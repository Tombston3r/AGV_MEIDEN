// Banc d'essai LOCAL de l'API du planning (spec §6).
//
// Ce serveur fait tourner le VRAI moteur — pas une doublure qui pourrait
// mentir — derrière l'API REST de la spec, sur 127.0.0.1. S'y ajoute une
// horloge simulée pilotable (/api/sim/*) : un planning JOURNALIER testé en
// temps réel est inutilisable, il faut pouvoir se placer à 05:59, accélérer,
// franchir minuit.
//
// Hôte uniquement : sockets POSIX, un seul fil d'exécution (aucun verrou à
// avoir juste). Le portage ESP32 reprendra les mêmes routes sur le serveur
// web de l'architecture A4 — voir docs/ALIGNEMENT_COMM_DISTANCE.md.
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "moteur/planning.h"
#include "moteur/serialisation.h"

namespace {

using namespace agv::planning;

volatile std::sig_atomic_t g_stop = 0;
void sur_signal(int) { g_stop = 1; }

// --- Horloge simulée -------------------------------------------------------
// now = base + (réel écoulé) × facteur. Tout est pilotable par l'API.
class HorlogeSimulee {
 public:
  HorlogeSimulee() : base_(::time(nullptr)), ref_(std::chrono::steady_clock::now()) {}

  time_t now() const {
    const auto ecoule = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - ref_)
                            .count();
    return base_ + static_cast<time_t>(ecoule * facteur_ / 1000.0);
  }
  void definir(time_t t) {
    base_ = t;
    ref_ = std::chrono::steady_clock::now();
  }
  void avancer(long long s) { definir(now() + s); }
  void vitesse(double f) {
    definir(now());  // fige l'acquis avant de changer de pente
    facteur_ = f;
  }
  double facteur() const { return facteur_; }

 private:
  time_t base_;
  std::chrono::steady_clock::time_point ref_;
  double facteur_ = 1.0;
};

std::string heure_locale(time_t t) {
  tm local{};
  localtime_r(&t, &local);
  char buf[40];
  std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                local.tm_hour, local.tm_min, local.tm_sec);
  return buf;
}

// --- État du banc ----------------------------------------------------------
struct MissionEmise {
  Mission mission;
  time_t emise_le = 0;
};

struct Banc {
  Config cfg;
  Moteur moteur{cfg};
  HorlogeSimulee horloge;
  bool heure_fiable = true;
  int version = 1;  // ETag du planning
  std::vector<MissionEmise> missions;
  std::string dossier_web;

  std::string etag() const { return "\"v" + std::to_string(version) + "\""; }
};

// --- HTTP minimal ----------------------------------------------------------
struct Requete {
  std::string methode, chemin, corps;
  std::string if_match;
};

bool lire_requete(int fd, Requete& out) {
  std::string brut;
  brut.reserve(4096);
  char tampon[4096];
  size_t fin_entetes = std::string::npos;
  size_t attendu = 0;

  while (true) {
    pollfd p{fd, POLLIN, 0};
    if (poll(&p, 1, 2000) <= 0) return false;
    const ssize_t n = read(fd, tampon, sizeof(tampon));
    if (n <= 0) return false;
    brut.append(tampon, static_cast<size_t>(n));
    if (brut.size() > 256 * 1024) return false;  // borne franche

    if (fin_entetes == std::string::npos) {
      fin_entetes = brut.find("\r\n\r\n");
      if (fin_entetes == std::string::npos) continue;
      // Longueur du corps et If-Match, cherchés une fois les en-têtes complets.
      std::istringstream flux(brut.substr(0, fin_entetes));
      std::string ligne;
      std::getline(flux, ligne);
      std::istringstream premiere(ligne);
      premiere >> out.methode >> out.chemin;
      while (std::getline(flux, ligne)) {
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();
        const size_t deux_points = ligne.find(':');
        if (deux_points == std::string::npos) continue;
        std::string cle = ligne.substr(0, deux_points);
        for (char& ch : cle) ch = static_cast<char>(std::tolower(ch));
        std::string valeur = ligne.substr(deux_points + 1);
        while (!valeur.empty() && valeur.front() == ' ') valeur.erase(0, 1);
        if (cle == "content-length") attendu = std::stoul(valeur);
        if (cle == "if-match") out.if_match = valeur;
      }
    }
    if (brut.size() >= fin_entetes + 4 + attendu) {
      out.corps = brut.substr(fin_entetes + 4, attendu);
      return true;
    }
  }
}

void repondre(int fd, int code, const std::string& corps,
              const std::string& type = "application/json; charset=utf-8",
              const std::string& etag = "") {
  const char* texte = code == 200   ? "OK"
                      : code == 400 ? "Bad Request"
                      : code == 404 ? "Not Found"
                      : code == 409 ? "Conflict"
                      : code == 428 ? "Precondition Required"
                                    : "Internal Server Error";
  std::string tete = "HTTP/1.1 " + std::to_string(code) + " " + texte +
                     "\r\nContent-Type: " + type +
                     "\r\nContent-Length: " + std::to_string(corps.size()) +
                     "\r\nCache-Control: no-store\r\nConnection: close\r\n";
  if (!etag.empty()) tete += "ETag: " + etag + "\r\n";
  tete += "\r\n";
  (void)!write(fd, tete.data(), tete.size());
  (void)!write(fd, corps.data(), corps.size());
}

std::string json_erreur(const std::string& message) {
  Document poubelle;  // réutilise l'échappement du codec via un détour simple
  (void)poubelle;
  std::string out = "{\"erreur\":\"";
  for (const char c : message) {
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else out += c;
  }
  return out + "\"}";
}

// --- Routes ----------------------------------------------------------------
Document document_courant(const Banc& banc) {
  Document d;
  d.entrees = banc.moteur.entrees();
  d.validation = banc.moteur.validation();
  return d;
}

void route_get_planning(Banc& banc, int fd) {
  repondre(fd, 200, document_vers_json(document_courant(banc)),
           "application/json; charset=utf-8", banc.etag());
}

void route_put_planning(Banc& banc, int fd, const Requete& req) {
  // Contrôle optimiste (§6) : sans If-Match, un second navigateur écraserait
  // en silence les modifications du premier.
  if (req.if_match.empty()) {
    repondre(fd, 428, json_erreur("If-Match requis : lire GET /api/planning d'abord"));
    return;
  }
  if (req.if_match != banc.etag()) {
    repondre(fd, 409,
             json_erreur("version perimee : le planning a change depuis votre lecture"),
             "application/json; charset=utf-8", banc.etag());
    return;
  }
  Document d;
  std::string erreur;
  if (!document_depuis_json(req.corps, d, erreur)) {
    repondre(fd, 400, json_erreur(erreur));
    return;
  }
  banc.moteur.definir_entrees(std::move(d.entrees));
  // Ce qui a été validé ce matin n'est plus ce qui est en mémoire : toute
  // modification révoque l'autorisation du jour (§3.2, « rien ne part sans un
  // geste explicite »).
  banc.moteur.invalider_journee();
  ++banc.version;
  repondre(fd, 200, "{\"enregistre\":true}", "application/json; charset=utf-8",
           banc.etag());
}

void route_next(Banc& banc, int fd) {
  const auto liste = banc.moteur.prochaines(banc.horloge.now(), 10);
  std::string out = "{\"occurrences\":[";
  for (size_t i = 0; i < liste.size(); ++i) {
    if (i) out += ',';
    out += "{\"id\":\"" + liste[i].id + "\",\"quand\":" +
           std::to_string(liste[i].quand) + ",\"locale\":\"" +
           heure_locale(liste[i].quand) + "\",\"station\":" +
           std::to_string(liste[i].station) +
           ",\"decalee_dst\":" + (liste[i].decalee_dst ? "true" : "false") + "}";
  }
  out += "]}";
  repondre(fd, 200, out);
}

// Occurrences d'AUJOURD'HUI, passées comprises — c'est la frise de l'IHM.
// `/api/planning/next` ne donne que le futur ; la frise montre la journée.
// Calculées par le MOTEUR (DST compris), pas recalculées en JavaScript.
void route_jour(Banc& banc, int fd) {
  const time_t now = banc.horloge.now();
  tm local{};
  localtime_r(&now, &local);
  local.tm_hour = 0;
  local.tm_min = 0;
  local.tm_sec = 0;
  local.tm_isdst = -1;
  const time_t debut_du_jour = mktime(&local);
  const Date aujourdhui = date_locale(now);

  std::string out = "{\"occurrences\":[";
  bool premier = true;
  for (const Entree& e : banc.moteur.entrees()) {
    if (!e.enabled) continue;
    const auto occ = prochaine_occurrence(e, debut_du_jour, banc.cfg);
    if (!occ || date_locale(occ->quand) != aujourdhui) continue;
    if (!premier) out += ',';
    premier = false;
    out += "{\"id\":\"" + e.id + "\",\"quand\":" + std::to_string(occ->quand) +
           ",\"locale\":\"" + heure_locale(occ->quand) + "\",\"station\":" +
           std::to_string(e.station) +
           ",\"decalee_dst\":" + (occ->decalee_dst ? "true" : "false") + "}";
  }
  out += "]}";
  repondre(fd, 200, out);
}

void route_time(Banc& banc, int fd) {
  const time_t now = banc.horloge.now();
  const Validation& v = banc.moteur.validation();
  std::string out =
      "{\"epoch\":" + std::to_string(now) + ",\"locale\":\"" + heure_locale(now) +
      "\",\"fiable\":" + (banc.heure_fiable ? "true" : "false") +
      ",\"source\":\"simulation\",\"facteur\":" +
      std::to_string(banc.horloge.facteur()) +
      ",\"journee_validee\":" + (banc.moteur.journee_validee(now) ? "true" : "false") +
      ",\"valide_par\":\"" + v.valide_par + "\"" +
      ",\"pause\":" + (banc.moteur.en_pause() ? "true" : "false") + "}";
  repondre(fd, 200, out);
}

void route_missions(Banc& banc, int fd) {
  std::string out = "{\"missions\":[";
  for (size_t i = 0; i < banc.missions.size(); ++i) {
    const MissionEmise& m = banc.missions[i];
    if (i) out += ',';
    out += "{\"id\":\"" + m.mission.id +
           "\",\"station\":" + std::to_string(m.mission.station) +
           ",\"prevu\":\"" + heure_locale(m.mission.prevu) +
           "\",\"emise\":\"" + heure_locale(m.emise_le) + "\",\"decalee_dst\":" +
           (m.mission.decalee_dst ? "true" : "false") + "}";
  }
  out += "]}";
  repondre(fd, 200, out);
}

void route_journal(Banc& banc, int fd) {
  std::string out = "{\"evenements\":[";
  const auto& journal = banc.moteur.journal();
  for (size_t i = 0; i < journal.size(); ++i) {
    if (i) out += ',';
    out += "{\"quand\":\"" + heure_locale(journal[i].quand) + "\",\"motif\":\"" +
           motif_texte(journal[i].motif) + "\",\"id\":\"" + journal[i].id + "\"}";
  }
  out += "]}";
  repondre(fd, 200, out);
}

void route_statique(Banc& banc, int fd, const std::string& chemin) {
  const std::string nom = (chemin == "/" || chemin.empty()) ? "index.html" : chemin.substr(1);
  if (nom.find("..") != std::string::npos) {
    repondre(fd, 404, json_erreur("introuvable"));
    return;
  }
  std::ifstream fichier(banc.dossier_web + "/" + nom, std::ios::binary);
  if (!fichier) {
    repondre(fd, 404, json_erreur("introuvable"));
    return;
  }
  std::stringstream contenu;
  contenu << fichier.rdbuf();
  repondre(fd, 200, contenu.str(), "text/html; charset=utf-8");
}

void traiter(Banc& banc, int fd, const Requete& req) {
  const time_t now = banc.horloge.now();

  if (req.methode == "GET") {
    if (req.chemin == "/api/planning") return route_get_planning(banc, fd);
    if (req.chemin == "/api/planning/next") return route_next(banc, fd);
    if (req.chemin == "/api/planning/jour") return route_jour(banc, fd);
    if (req.chemin == "/api/time") return route_time(banc, fd);
    if (req.chemin == "/api/missions") return route_missions(banc, fd);
    if (req.chemin == "/api/journal") return route_journal(banc, fd);
    return route_statique(banc, fd, req.chemin);
  }

  if (req.methode == "PUT" && req.chemin == "/api/planning") {
    return route_put_planning(banc, fd, req);
  }

  if (req.methode == "POST") {
    if (req.chemin == "/api/planning/validate") {
      const auto par = json_chaine(req.corps, "par");
      if (!par || par->empty()) {
        return repondre(fd, 400, json_erreur("champ « par » requis : qui valide ?"));
      }
      banc.moteur.valider_journee(date_locale(now), *par, now);
      return repondre(fd, 200, "{\"validee\":true}");
    }
    if (req.chemin == "/api/planning/pause") {
      const auto actif = json_booleen(req.corps, "actif");
      if (!actif) return repondre(fd, 400, json_erreur("champ « actif » requis"));
      banc.moteur.pause(*actif);
      return repondre(fd, 200, "{\"pause\":" + std::string(*actif ? "true" : "false") + "}");
    }
    if (req.chemin == "/api/appel") {
      // Geste opérateur immédiat — l'équivalent logiciel du bouton d'appel
      // physique du chantier « Comm distance ». Il ne passe PAS par le
      // planning : la validation quotidienne (§3.2) borne le déclenchement
      // AUTONOME, pas un humain qui demande l'AGV à son poste.
      const auto station = json_entier(req.corps, "station");
      if (!station || *station < 0 || *station > 1023) {
        return repondre(fd, 400, json_erreur("station 0-1023 requise (brief §5.1)"));
      }
      Mission m;
      m.id = "appel";
      m.station = static_cast<uint16_t>(*station);
      m.prevu = now;
      banc.missions.push_back({m, now});
      std::printf("[mission] appel -> station %u (%s)\n", m.station,
                  heure_locale(now).c_str());
      std::fflush(stdout);
      return repondre(fd, 200, "{\"appel\":true,\"station\":" +
                                   std::to_string(m.station) + "}");
    }
    if (req.chemin == "/api/planning/skip") {
      banc.moteur.sauter_prochaine();
      return repondre(fd, 200, "{\"saut_arme\":true}");
    }

    // --- Simulation seulement : ABSENT de la cible ESP32 -------------------
    if (req.chemin == "/api/sim/heure") {
      if (const auto epoch = json_entier(req.corps, "epoch")) {
        banc.horloge.definir(static_cast<time_t>(*epoch));
        return route_time(banc, fd);
      }
      if (const auto locale = json_chaine(req.corps, "locale")) {
        tm local{};
        int a, mo, j, h, mi;
        if (std::sscanf(locale->c_str(), "%d-%d-%d %d:%d", &a, &mo, &j, &h, &mi) == 5) {
          local.tm_year = a - 1900;
          local.tm_mon = mo - 1;
          local.tm_mday = j;
          local.tm_hour = h;
          local.tm_min = mi;
          local.tm_isdst = -1;
          banc.horloge.definir(mktime(&local));
          return route_time(banc, fd);
        }
      }
      return repondre(fd, 400, json_erreur("« epoch » ou « locale » (AAAA-MM-JJ HH:MM) requis"));
    }
    if (req.chemin == "/api/sim/avancer") {
      const auto s = json_entier(req.corps, "secondes");
      if (!s) return repondre(fd, 400, json_erreur("champ « secondes » requis"));
      banc.horloge.avancer(*s);
      return route_time(banc, fd);
    }
    if (req.chemin == "/api/sim/vitesse") {
      const auto f = json_entier(req.corps, "facteur");
      if (!f || *f < 1 || *f > 86400) {
        return repondre(fd, 400, json_erreur("facteur entre 1 et 86400 requis"));
      }
      banc.horloge.vitesse(static_cast<double>(*f));
      return route_time(banc, fd);
    }
    if (req.chemin == "/api/sim/fiable") {
      const auto fiable = json_booleen(req.corps, "fiable");
      if (!fiable) return repondre(fd, 400, json_erreur("champ « fiable » requis"));
      banc.heure_fiable = *fiable;
      return route_time(banc, fd);
    }
  }

  repondre(fd, 404, json_erreur("route inconnue"));
}

}  // namespace

int main(int argc, char** argv) {
  int port = 8081;
  std::string dossier_web = "banc_api/web";
  for (int i = 1; i < argc - 1; ++i) {
    if (std::strcmp(argv[i], "--port") == 0) port = std::atoi(argv[i + 1]);
    if (std::strcmp(argv[i], "--web") == 0) dossier_web = argv[i + 1];
  }

  // Même fuseau que la cible (§2.4) — transitions été/hiver comprises.
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  std::signal(SIGINT, sur_signal);
  std::signal(SIGTERM, sur_signal);
  std::signal(SIGPIPE, SIG_IGN);

  Banc banc;
  banc.dossier_web = dossier_web;

  const int ecoute = socket(AF_INET, SOCK_STREAM, 0);
  const int oui = 1;
  setsockopt(ecoute, SOL_SOCKET, SO_REUSEADDR, &oui, sizeof(oui));
  sockaddr_in adresse{};
  adresse.sin_family = AF_INET;
  adresse.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // banc local, jamais exposé
  adresse.sin_port = htons(static_cast<uint16_t>(port));
  if (bind(ecoute, reinterpret_cast<sockaddr*>(&adresse), sizeof(adresse)) != 0 ||
      listen(ecoute, 8) != 0) {
    std::fprintf(stderr, "impossible d'ecouter sur 127.0.0.1:%d\n", port);
    return 1;
  }
  socklen_t taille = sizeof(adresse);
  getsockname(ecoute, reinterpret_cast<sockaddr*>(&adresse), &taille);
  std::printf("BANC_API PORT=%d\n", ntohs(adresse.sin_port));
  std::printf("banc API planning sur http://127.0.0.1:%d — horloge simulee x1\n",
              ntohs(adresse.sin_port));
  std::fflush(stdout);

  while (!g_stop) {
    // Le moteur avance ENTRE les requêtes : un seul fil, aucun verrou. Après
    // un saut de temps, plusieurs occurrences peuvent être dues : on tire
    // jusqu'à épuisement (borné).
    for (int i = 0; i < 32; ++i) {
      const auto mission = banc.moteur.tick(banc.horloge.now(), banc.heure_fiable);
      if (!mission) break;
      banc.missions.push_back({*mission, banc.horloge.now()});
      std::printf("[mission] %s -> station %u (prevu %s)\n", mission->id.c_str(),
                  mission->station, heure_locale(mission->prevu).c_str());
      std::fflush(stdout);
    }

    pollfd p{ecoute, POLLIN, 0};
    if (poll(&p, 1, 100) <= 0) continue;
    const int client = accept(ecoute, nullptr, nullptr);
    if (client < 0) continue;
    Requete req;
    if (lire_requete(client, req)) traiter(banc, client, req);
    close(client);
  }
  close(ecoute);
  std::puts("arret du banc");
  return 0;
}

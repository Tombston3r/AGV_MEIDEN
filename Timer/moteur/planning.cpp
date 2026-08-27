#include "moteur/planning.h"

#include <algorithm>

namespace agv::planning {
namespace {

constexpr size_t kJournalMax = 128;
constexpr int kHorizonJours = 400;  // couvre une année et ses deux DST

struct Ymd {
  int a = 0, m = 0, j = 0;
};

Ymd ymd_locale(time_t t) {
  tm local{};
  localtime_r(&t, &local);
  return {local.tm_year + 1900, local.tm_mon + 1, local.tm_mday};
}

Date date_de(const Ymd& d) { return d.a * 10000 + d.m * 100 + d.j; }

// Midi local du jour `d + decalage_jours`. Midi, parce que c'est l'heure la
// plus loin possible des transitions d'heure d'été : l'arithmétique de jours
// y est sûre, là où minuit peut ne pas exister dans certains fuseaux.
time_t midi_local(const Ymd& d, int decalage_jours) {
  tm local{};
  local.tm_year = d.a - 1900;
  local.tm_mon = d.m - 1;
  local.tm_mday = d.j + decalage_jours;  // mktime normalise les débordements
  local.tm_hour = 12;
  local.tm_isdst = -1;
  return mktime(&local);
}

struct ResultatJour {
  enum class Type : uint8_t { Aucune, Normale, Decalee, Supprimee } type = Type::Aucune;
  time_t quand = 0;
  Date date = 0;
};

// L'occurrence de `e` pour le jour local donné, règles de calendrier
// comprises. C'est ici que vit toute la mécanique d'heure d'été.
ResultatJour occurrence_du_jour(const Entree& e, const Ymd& d, const Config& cfg) {
  const time_t midi = midi_local(d, 0);
  const Ymd dn = ymd_locale(midi);  // renormalisé si d venait d'un décalage
  const Date dd = date_de(dn);

  if ((e.jours & (1u << jour_semaine(midi))) == 0) return {};
  if (e.debut != 0 && dd < e.debut) return {};
  if (e.fin != 0 && dd > e.fin) return {};
  for (const Date ex : e.exceptions) {
    if (ex == dd) return {};
  }

  tm essai{};
  essai.tm_year = dn.a - 1900;
  essai.tm_mon = dn.m - 1;
  essai.tm_mday = dn.j;
  essai.tm_hour = e.heure;
  essai.tm_min = e.minute;
  essai.tm_isdst = -1;
  const time_t t = mktime(&essai);

  // Un horaire EXISTANT fait l'aller-retour à l'identique. S'il ne le fait
  // pas, c'est qu'il tombe dans le trou du passage à l'heure d'été.
  tm relu{};
  localtime_r(&t, &relu);
  if (t != static_cast<time_t>(-1) && relu.tm_hour == e.heure &&
      relu.tm_min == e.minute && relu.tm_mday == dn.j) {
    return {ResultatJour::Type::Normale, t, dd};
  }

  if (cfg.dst == DstPrintemps::Sauter) {
    return {ResultatJour::Type::Supprimee, midi, dd};
  }

  // Premier instant existant après le trou, cherché minute par minute. Le
  // trou fait une heure en Europe ; 180 minutes couvrent les fuseaux exotiques.
  for (int k = 1; k <= 180; ++k) {
    const int total = e.heure * 60 + e.minute + k;
    if (total >= 24 * 60) break;  // ne déborde jamais sur le jour suivant
    tm candidat{};
    candidat.tm_year = dn.a - 1900;
    candidat.tm_mon = dn.m - 1;
    candidat.tm_mday = dn.j;
    candidat.tm_hour = total / 60;
    candidat.tm_min = total % 60;
    candidat.tm_isdst = -1;
    tm copie = candidat;
    const time_t tc = mktime(&copie);
    tm relu2{};
    localtime_r(&tc, &relu2);
    if (tc != static_cast<time_t>(-1) && relu2.tm_hour == candidat.tm_hour &&
        relu2.tm_min == candidat.tm_min && relu2.tm_mday == dn.j) {
      return {ResultatJour::Type::Decalee, tc, dd};
    }
  }
  return {};
}

}  // namespace

Date date_locale(time_t t) { return date_de(ymd_locale(t)); }

int jour_semaine(time_t t) {
  tm local{};
  localtime_r(&t, &local);
  return (local.tm_wday + 6) % 7;  // tm_wday : 0 = dimanche
}

const char* motif_texte(Motif m) {
  switch (m) {
    case Motif::Executee: return "executee";
    case Motif::MissionArrivee: return "arrivee confirmee";
    case Motif::MissionInterrompue:
      return "ARRET DE SECURITE EN DEPLACEMENT : obstacle probable";
    case Motif::ArretHorsDeplacement: return "mise hors tension a l'arret";
    case Motif::AlerteAcquittee: return "alerte acquittee";
    case Motif::SauteeAlerte: return "sautee : alerte non acquittee";
    case Motif::ExecuteeDecaleeDst: return "executee decalee (heure d'ete)";
    case Motif::SauteeGrace: return "sautee : vue apres la fenetre de grace";
    case Motif::SauteeNonValidee: return "sautee : journee non validee";
    case Motif::SauteePause: return "sautee : planning en pause";
    case Motif::SauteeSurDemande: return "sautee : sur demande operateur";
    case Motif::SauteeDst: return "sautee : horaire inexistant (heure d'ete)";
    case Motif::JourneeValidee: return "journee validee";
    case Motif::HeureNonFiable: return "heure non fiable : planning gele";
    case Motif::HeureRedevenueFiable: return "heure redevenue fiable";
  }
  return "?";
}

std::optional<Occurrence> prochaine_occurrence(const Entree& e, time_t depuis,
                                               const Config& cfg) {
  const Ymd d0 = ymd_locale(depuis);
  for (int k = 0; k <= kHorizonJours; ++k) {
    const Ymd dk = ymd_locale(midi_local(d0, k));
    const ResultatJour r = occurrence_du_jour(e, dk, cfg);
    if ((r.type == ResultatJour::Type::Normale ||
         r.type == ResultatJour::Type::Decalee) &&
        r.quand >= depuis) {
      return Occurrence{r.quand, r.type == ResultatJour::Type::Decalee};
    }
  }
  return std::nullopt;
}

void Moteur::definir_entrees(std::vector<Entree> entrees) {
  entrees_ = std::move(entrees);
}

bool Moteur::valider_journee(Date d, const std::string& par, time_t quand) {
  if (d <= 0 || par.empty()) return false;
  validation_ = {d, par, quand};
  journaliser(quand, Motif::JourneeValidee, par);
  return true;
}

bool Moteur::journee_validee(time_t now) const {
  return validation_.valide_pour == date_locale(now);
}

bool Moteur::consommee(const std::string& id, Date d) const {
  for (const auto& c : consommees_) {
    if (c.d == d && c.id == id) return true;
  }
  return false;
}

void Moteur::consommer(const std::string& id, Date d) {
  consommees_.push_back({id, d});
}

void Moteur::journaliser(time_t quand, Motif m, const std::string& id) {
  journal_.push_back({quand, m, id});
  if (journal_.size() > kJournalMax) {
    journal_.erase(journal_.begin());
  }
}

void Moteur::purger(time_t now) {
  const Date seuil = date_locale(midi_local(ymd_locale(now), -2));
  consommees_.erase(
      std::remove_if(consommees_.begin(), consommees_.end(),
                     [seuil](const Consommee& c) { return c.d < seuil; }),
      consommees_.end());
}

void Moteur::mission_arrivee(time_t quand) {
  if (mission_en_cours_.empty()) return;
  journaliser(quand, Motif::MissionArrivee, mission_en_cours_);
  mission_en_cours_.clear();
}

void Moteur::interruption_agv(bool en_deplacement, time_t quand) {
  // Hors déplacement, une coupure est banale : maintenance, fin de poste,
  // quelqu'un devant un AGV à quai. La signaler comme un obstacle ferait
  // perdre toute valeur à l'alerte.
  if (!en_deplacement) {
    journaliser(quand, Motif::ArretHorsDeplacement, mission_en_cours_);
    return;
  }
  journaliser(quand, Motif::MissionInterrompue, mission_en_cours_);
  alerte_ = {true, mission_en_cours_, station_en_cours_, quand};
  mission_en_cours_.clear();
}

bool Moteur::acquitter_alerte(const std::string& par, time_t quand) {
  if (!alerte_.active || par.empty()) return false;
  alerte_ = {};
  journaliser(quand, Motif::AlerteAcquittee, par);
  return true;
}

std::optional<Mission> Moteur::tick(time_t now, bool heure_fiable) {
  // §2.3 : heure douteuse = planning GELÉ. Ni exécution, ni consommation,
  // ce qui n'est pas parti partira (ou sera sauté) quand l'heure reviendra,
  // sous le contrôle de la fenêtre de grâce.
  if (!heure_fiable) {
    if (heure_etait_fiable_) journaliser(now, Motif::HeureNonFiable, "");
    heure_etait_fiable_ = false;
    return std::nullopt;
  }
  if (!heure_etait_fiable_) {
    journaliser(now, Motif::HeureRedevenueFiable, "");
    heure_etait_fiable_ = true;
  }

  purger(now);

  // La grâce peut chevaucher minuit (mission 23:58 vue à 00:02) : on examine
  // les occurrences d'hier ET d'aujourd'hui. La clé d'idempotence porte la
  // date de l'OCCURRENCE, pas celle du jour courant.
  struct Candidat {
    const Entree* e = nullptr;
    ResultatJour r;
  };
  std::vector<Candidat> candidats;

  const Ymd auj = ymd_locale(now);
  for (int k = -1; k <= 0; ++k) {
    const Ymd d = ymd_locale(midi_local(auj, k));
    for (const Entree& e : entrees_) {
      if (!e.enabled) continue;
      const ResultatJour r = occurrence_du_jour(e, d, cfg_);

      if (r.type == ResultatJour::Type::Supprimee) {
        if (k == 0 && !consommee(e.id, r.date)) {
          consommer(e.id, r.date);
          journaliser(now, Motif::SauteeDst, e.id);
        }
        continue;
      }
      if (r.type == ResultatJour::Type::Aucune) continue;
      if (r.quand > now) continue;               // pas encore l'heure
      if (consommee(e.id, r.date)) continue;     // déjà traitée (§4.2)

      // La veille n'est balayée QUE pour la grâce à cheval sur minuit. Hors
      // de cette fenêtre, ses occurrences sont ignorées sans être journalisées :
      // après un redémarrage, le moteur ne sait pas si elles ont réellement
      // tourné avant la coupure : les marquer « sautées » ferait mentir le
      // journal. (Vaut aussi pour aujourd'hui tant que les clés consommées ne
      // sont pas persistées : voir docs/ETAT_PROJET.md.)
      if (k < 0 && now - r.quand > static_cast<time_t>(cfg_.grace_s)) continue;

      if (now - r.quand > static_cast<time_t>(cfg_.grace_s)) {
        // Trop tard. Le motif retenu est la CAUSE du blocage, pas seulement
        // « trop tard » : c'est lui qu'on lira dans le journal.
        consommer(e.id, r.date);
        const Motif motif = alerte_.active     ? Motif::SauteeAlerte
                            : !journee_validee(now) ? Motif::SauteeNonValidee
                            : pause_                ? Motif::SauteePause
                                                    : Motif::SauteeGrace;
        journaliser(now, motif, e.id);
        continue;
      }

      // Dans la grâce mais bloquée (validation absente, pause, alerte) : on la
      // LAISSE en attente. Levée du blocage avant la fin de grâce → elle
      // part ; sinon elle vieillit et sort par le motif ci-dessus.
      if (!journee_validee(now) || pause_ || alerte_.active) continue;

      candidats.push_back({&e, r});
    }
  }

  if (candidats.empty()) return std::nullopt;

  std::sort(candidats.begin(), candidats.end(),
            [](const Candidat& a, const Candidat& b) {
              if (a.e->priorite != b.e->priorite) return a.e->priorite < b.e->priorite;
              if (a.r.quand != b.r.quand) return a.r.quand < b.r.quand;
              return a.e->id < b.e->id;
            });

  const Candidat& retenu = candidats.front();
  consommer(retenu.e->id, retenu.r.date);

  if (sauter_prochaine_) {
    sauter_prochaine_ = false;
    journaliser(now, Motif::SauteeSurDemande, retenu.e->id);
    return std::nullopt;
  }

  const bool decalee = retenu.r.type == ResultatJour::Type::Decalee;
  journaliser(now, decalee ? Motif::ExecuteeDecaleeDst : Motif::Executee,
              retenu.e->id);
  mission_en_cours_ = retenu.e->id;
  station_en_cours_ = retenu.e->station;
  return Mission{retenu.e->id, retenu.e->station, retenu.e->flags,
                 retenu.r.quand, decalee};
}

std::vector<Moteur::Prochaine> Moteur::prochaines(time_t now, size_t n) const {
  std::vector<Prochaine> out;
  for (const Entree& e : entrees_) {
    if (!e.enabled) continue;
    time_t depuis = now;
    for (size_t i = 0; i < n; ++i) {
      const auto occ = prochaine_occurrence(e, depuis, cfg_);
      if (!occ) break;
      if (!consommee(e.id, date_locale(occ->quand))) {
        out.push_back({occ->quand, e.id, e.station, occ->decalee_dst});
      }
      depuis = occ->quand + 60;
    }
  }
  std::sort(out.begin(), out.end(),
            [](const Prochaine& a, const Prochaine& b) { return a.quand < b.quand; });

  // Chevauchement : deux départs plus proches que la durée d'une mission.
  for (size_t i = 1; i < out.size(); ++i) {
    if (out[i].quand - out[i - 1].quand < static_cast<time_t>(cfg_.duree_mission_s)) {
      out[i].conflit = true;
    }
  }
  if (out.size() > n) out.resize(n);
  return out;
}

}  // namespace agv::planning

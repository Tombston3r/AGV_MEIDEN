#include "moteur/serialisation.h"

#include <cctype>
#include <cstdio>

namespace agv::planning {
namespace {

// --- Tokenizeur minimal ----------------------------------------------------
// Descente récursive orientée schéma : on ne construit pas d'arbre générique,
// on lit exactement ce que le schéma attend et on saute le reste — d'où un
// parseur court, strict et sans allocation surprise.

struct Curseur {
  const std::string& s;
  size_t i = 0;
  std::string erreur;

  bool echec(const std::string& message) {
    if (erreur.empty()) {
      erreur = message + " (octet " + std::to_string(i) + ")";
    }
    return false;
  }
  void blancs() {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
  }
  bool litteral(char c) {
    blancs();
    if (i < s.size() && s[i] == c) {
      ++i;
      return true;
    }
    return echec(std::string("'") + c + "' attendu");
  }
  bool fini() {
    blancs();
    return i >= s.size();
  }
};

bool lire_chaine(Curseur& c, std::string& out) {
  c.blancs();
  if (c.i >= c.s.size() || c.s[c.i] != '"') return c.echec("chaine attendue");
  ++c.i;
  out.clear();
  while (c.i < c.s.size()) {
    const char ch = c.s[c.i++];
    if (ch == '"') return true;
    if (ch == '\\') {
      if (c.i >= c.s.size()) break;
      const char e = c.s[c.i++];
      switch (e) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'n': out += '\n'; break;
        case 't': out += '\t'; break;
        case 'r': out += '\r'; break;
        default: return c.echec("echappement non gere");
      }
      continue;
    }
    out += ch;
  }
  return c.echec("chaine non terminee");
}

bool lire_entier(Curseur& c, long long& out) {
  c.blancs();
  const size_t depart = c.i;
  bool negatif = false;
  if (c.i < c.s.size() && c.s[c.i] == '-') {
    negatif = true;
    ++c.i;
  }
  long long v = 0;
  bool chiffres = false;
  while (c.i < c.s.size() && std::isdigit(static_cast<unsigned char>(c.s[c.i]))) {
    v = v * 10 + (c.s[c.i] - '0');
    ++c.i;
    chiffres = true;
  }
  if (!chiffres) {
    c.i = depart;
    return c.echec("entier attendu");
  }
  if (c.i < c.s.size() && (c.s[c.i] == '.' || c.s[c.i] == 'e' || c.s[c.i] == 'E')) {
    return c.echec("nombre a virgule refuse : le schema n'utilise que des entiers");
  }
  out = negatif ? -v : v;
  return true;
}

bool lire_booleen(Curseur& c, bool& out) {
  c.blancs();
  if (c.s.compare(c.i, 4, "true") == 0) {
    c.i += 4;
    out = true;
    return true;
  }
  if (c.s.compare(c.i, 5, "false") == 0) {
    c.i += 5;
    out = false;
    return true;
  }
  return c.echec("booleen attendu");
}

// Saute une valeur quelconque — nécessaire aux utilitaires json_* qui
// parcourent un objet sans connaître toutes ses clés.
bool sauter_valeur(Curseur& c) {
  c.blancs();
  if (c.i >= c.s.size()) return c.echec("valeur attendue");
  const char ch = c.s[c.i];
  if (ch == '"') {
    std::string poubelle;
    return lire_chaine(c, poubelle);
  }
  if (ch == '{' || ch == '[') {
    const char fermant = (ch == '{') ? '}' : ']';
    ++c.i;
    c.blancs();
    if (c.i < c.s.size() && c.s[c.i] == fermant) {
      ++c.i;
      return true;
    }
    while (true) {
      if (ch == '{') {
        std::string cle;
        if (!lire_chaine(c, cle) || !c.litteral(':')) return false;
      }
      if (!sauter_valeur(c)) return false;
      c.blancs();
      if (c.i < c.s.size() && c.s[c.i] == ',') {
        ++c.i;
        continue;
      }
      if (c.i < c.s.size() && c.s[c.i] == fermant) {
        ++c.i;
        return true;
      }
      return c.echec("',' ou fermeture attendue");
    }
  }
  if (ch == 't' || ch == 'f') {
    bool b;
    return lire_booleen(c, b);
  }
  if (ch == 'n') {
    if (c.s.compare(c.i, 4, "null") == 0) {
      c.i += 4;
      return true;
    }
    return c.echec("'null' attendu");
  }
  long long e;
  return lire_entier(c, e);
}

// Parcourt un objet { "cle": valeur, ... } en appelant `champ` pour chaque
// clé. `champ` rend false en cas d'erreur (message déjà posé sur le curseur).
template <typename F>
bool lire_objet(Curseur& c, F champ) {
  if (!c.litteral('{')) return false;
  c.blancs();
  if (c.i < c.s.size() && c.s[c.i] == '}') {
    ++c.i;
    return true;
  }
  while (true) {
    std::string cle;
    if (!lire_chaine(c, cle) || !c.litteral(':')) return false;
    if (!champ(cle)) return false;
    c.blancs();
    if (c.i < c.s.size() && c.s[c.i] == ',') {
      ++c.i;
      continue;
    }
    if (c.i < c.s.size() && c.s[c.i] == '}') {
      ++c.i;
      return true;
    }
    return c.echec("',' ou '}' attendu");
  }
}

// --- Lecture du schéma planning --------------------------------------------

bool lire_heure(Curseur& c, Entree& e) {
  std::string hm;
  if (!lire_chaine(c, hm)) return false;
  int h = -1, m = -1;
  if (std::sscanf(hm.c_str(), "%2d:%2d", &h, &m) != 2 || h < 0 || h > 23 ||
      m < 0 || m > 59 || hm.size() != 5) {
    return c.echec("heure invalide « " + hm + " » — format attendu HH:MM");
  }
  e.heure = static_cast<uint8_t>(h);
  e.minute = static_cast<uint8_t>(m);
  return true;
}

bool lire_entree(Curseur& c, Entree& e) {
  bool id_vu = false, heure_vue = false;
  const bool ok = lire_objet(c, [&](const std::string& cle) -> bool {
    long long v;
    if (cle == "id") {
      id_vu = true;
      return lire_chaine(c, e.id);
    }
    if (cle == "enabled") return lire_booleen(c, e.enabled);
    if (cle == "heure") {
      heure_vue = true;
      return lire_heure(c, e);
    }
    if (cle == "jours") {
      if (!lire_entier(c, v)) return false;
      if (v < 1 || v > 0x7F) return c.echec("masque de jours hors bornes (1-127)");
      e.jours = static_cast<uint8_t>(v);
      return true;
    }
    if (cle == "debut" || cle == "fin") {
      if (!lire_entier(c, v)) return false;
      if (v != 0 && (v < 20000101 || v > 21001231)) {
        return c.echec("date invalide — format AAAAMMJJ");
      }
      (cle == "debut" ? e.debut : e.fin) = static_cast<Date>(v);
      return true;
    }
    if (cle == "exceptions") {
      if (!c.litteral('[')) return false;
      c.blancs();
      if (c.i < c.s.size() && c.s[c.i] == ']') {
        ++c.i;
        return true;
      }
      while (true) {
        if (!lire_entier(c, v)) return false;
        if (v < 20000101 || v > 21001231) return c.echec("exception : date AAAAMMJJ attendue");
        e.exceptions.push_back(static_cast<Date>(v));
        c.blancs();
        if (c.i < c.s.size() && c.s[c.i] == ',') {
          ++c.i;
          continue;
        }
        if (c.i < c.s.size() && c.s[c.i] == ']') {
          ++c.i;
          return true;
        }
        return c.echec("',' ou ']' attendu");
      }
    }
    if (cle == "station") {
      if (!lire_entier(c, v)) return false;
      if (v < 0 || v > 1023) return c.echec("station hors bornes (0-1023, brief §5.1)");
      e.station = static_cast<uint16_t>(v);
      return true;
    }
    if (cle == "flags") {
      if (!lire_entier(c, v)) return false;
      if (v < 0 || v > 255) return c.echec("flags hors bornes (0-255)");
      e.flags = static_cast<uint8_t>(v);
      return true;
    }
    if (cle == "priorite") {
      if (!lire_entier(c, v)) return false;
      if (v < 0 || v > 255) return c.echec("priorite hors bornes (0-255)");
      e.priorite = static_cast<uint8_t>(v);
      return true;
    }
    return c.echec("cle inconnue « " + cle + " » dans une entree");
  });
  if (!ok) return false;
  if (!id_vu || e.id.empty()) return c.echec("entree sans identifiant");
  if (!heure_vue) return c.echec("entree « " + e.id + " » sans heure");
  return true;
}

void echapper(const std::string& in, std::string& out) {
  for (const char ch : in) {
    switch (ch) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\t': out += "\\t"; break;
      case '\r': out += "\\r"; break;
      default: out += ch;
    }
  }
}

}  // namespace

std::string document_vers_json(const Document& d) {
  std::string out = "{\"schema\":" + std::to_string(d.schema) + ",\"entrees\":[";
  bool premier = true;
  char heure[8];
  for (const Entree& e : d.entrees) {
    if (!premier) out += ',';
    premier = false;
    std::snprintf(heure, sizeof(heure), "%02u:%02u", e.heure, e.minute);
    out += "{\"id\":\"";
    echapper(e.id, out);
    out += std::string("\",\"enabled\":") + (e.enabled ? "true" : "false") +
           ",\"heure\":\"" + heure + "\",\"jours\":" + std::to_string(e.jours) +
           ",\"debut\":" + std::to_string(e.debut) +
           ",\"fin\":" + std::to_string(e.fin) + ",\"exceptions\":[";
    for (size_t i = 0; i < e.exceptions.size(); ++i) {
      if (i) out += ',';
      out += std::to_string(e.exceptions[i]);
    }
    out += "],\"station\":" + std::to_string(e.station) +
           ",\"flags\":" + std::to_string(e.flags) +
           ",\"priorite\":" + std::to_string(e.priorite) + "}";
  }
  out += "],\"validation\":{\"valide_pour\":" +
         std::to_string(d.validation.valide_pour) + ",\"valide_par\":\"";
  echapper(d.validation.valide_par, out);
  out += "\",\"valide_le\":" + std::to_string(d.validation.valide_le) + "}}";
  return out;
}

bool document_depuis_json(const std::string& json, Document& out,
                          std::string& erreur) {
  Curseur c{json, 0, {}};
  out = Document{};
  bool schema_vu = false;
  const bool ok = lire_objet(c, [&](const std::string& cle) -> bool {
    if (cle == "schema") {
      long long v;
      if (!lire_entier(c, v)) return false;
      if (v != 1) return c.echec("schema " + std::to_string(v) + " inconnu (attendu : 1)");
      schema_vu = true;
      return true;
    }
    if (cle == "entrees") {
      if (!c.litteral('[')) return false;
      c.blancs();
      if (c.i < c.s.size() && c.s[c.i] == ']') {
        ++c.i;
        return true;
      }
      while (true) {
        Entree e;
        if (!lire_entree(c, e)) return false;
        for (const Entree& deja : out.entrees) {
          if (deja.id == e.id) {
            return c.echec("identifiant duplique « " + e.id + " »");
          }
        }
        out.entrees.push_back(std::move(e));
        c.blancs();
        if (c.i < c.s.size() && c.s[c.i] == ',') {
          ++c.i;
          continue;
        }
        if (c.i < c.s.size() && c.s[c.i] == ']') {
          ++c.i;
          return true;
        }
        return c.echec("',' ou ']' attendu");
      }
    }
    if (cle == "validation") {
      return lire_objet(c, [&](const std::string& sous) -> bool {
        long long v;
        if (sous == "valide_pour") {
          if (!lire_entier(c, v)) return false;
          out.validation.valide_pour = static_cast<Date>(v);
          return true;
        }
        if (sous == "valide_par") return lire_chaine(c, out.validation.valide_par);
        if (sous == "valide_le") {
          if (!lire_entier(c, v)) return false;
          out.validation.valide_le = static_cast<time_t>(v);
          return true;
        }
        return c.echec("cle inconnue « " + sous + " » dans la validation");
      });
    }
    return c.echec("cle inconnue « " + cle + " » au premier niveau");
  });
  if (!ok || !c.fini()) {
    if (erreur.empty()) erreur = c.erreur.empty() ? "contenu apres la fin du document" : c.erreur;
    return false;
  }
  if (!schema_vu) {
    erreur = "champ « schema » manquant";
    return false;
  }
  erreur.clear();
  return true;
}

namespace {

// Trouve `cle` au premier niveau d'un objet et laisse le curseur sur sa valeur.
bool positionner_sur(Curseur& c, const std::string& cle) {
  if (!c.litteral('{')) return false;
  c.blancs();
  if (c.i < c.s.size() && c.s[c.i] == '}') return false;
  while (true) {
    std::string k;
    if (!lire_chaine(c, k) || !c.litteral(':')) return false;
    if (k == cle) return true;
    if (!sauter_valeur(c)) return false;
    c.blancs();
    if (c.i < c.s.size() && c.s[c.i] == ',') {
      ++c.i;
      continue;
    }
    return false;
  }
}

}  // namespace

std::optional<long long> json_entier(const std::string& json, const std::string& cle) {
  Curseur c{json, 0, {}};
  long long v;
  if (positionner_sur(c, cle) && lire_entier(c, v)) return v;
  return std::nullopt;
}

std::optional<std::string> json_chaine(const std::string& json, const std::string& cle) {
  Curseur c{json, 0, {}};
  std::string v;
  if (positionner_sur(c, cle) && lire_chaine(c, v)) return v;
  return std::nullopt;
}

std::optional<bool> json_booleen(const std::string& json, const std::string& cle) {
  Curseur c{json, 0, {}};
  bool v;
  if (positionner_sur(c, cle) && lire_booleen(c, v)) return v;
  return std::nullopt;
}

}  // namespace agv::planning

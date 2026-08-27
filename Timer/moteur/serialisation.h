// Document de planning <-> JSON — le MÊME schéma sert l'API (§6) et la
// persistance LittleFS (§3.3). C++ pur, comme le moteur.
//
// Le parseur est STRICT : clé inconnue, borne dépassée, heure invalide ou
// identifiant en double sont des ERREURS, pas des avertissements. Sur une API
// qui commande un véhicule, une faute de frappe ignorée en silence devient
// une mission avec la mauvaise priorité.
#pragma once

#include <optional>
#include <string>

#include "moteur/planning.h"

namespace agv::planning {

struct Document {
  int schema = 1;  // version du schéma (§3.3)
  std::vector<Entree> entrees;
  Validation validation;
};

std::string document_vers_json(const Document& d);

// `false` + `erreur` renseignée (avec la position) si le JSON est invalide.
bool document_depuis_json(const std::string& json, Document& out,
                          std::string& erreur);

// Lecture d'un champ au premier niveau d'un petit objet JSON — pour les corps
// de requêtes simples ({"par": "dupont"}, {"secondes": 3600}…). Bâtis sur le
// vrai tokenizeur : pas de recherche naïve de sous-chaîne.
std::optional<long long> json_entier(const std::string& json, const std::string& cle);
std::optional<std::string> json_chaine(const std::string& json, const std::string& cle);
std::optional<bool> json_booleen(const std::string& json, const std::string& cle);

}  // namespace agv::planning

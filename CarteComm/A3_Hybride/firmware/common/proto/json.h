// Sérialisation JSON minimale pour les charges utiles MQTT (planification §2).
//
// Volontairement minuscule : le poste fixe attend quatre à huit champs
// numériques plats. Embarquer un analyseur JSON complet sur l'ESP32 coûterait
// plus qu'il ne rapporterait, et une bibliothèque tierce dans le chemin de
// commande est une dépendance de plus à auditer.
//
// Limites assumées : pas d'objets imbriqués, pas de tableaux, pas d'échappement
// Unicode. Toute charge utile qui en aurait besoin est hors périmètre.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv::json {

// Construction incrémentale dans un tampon fourni par l'appelant.
class Writer {
 public:
  Writer(char* out, size_t capacity) : out_(out), capacity_(capacity) { begin(); }

  void begin();
  void field(const char* key, int32_t value);
  void field(const char* key, uint32_t value);
  void field(const char* key, bool value);
  void field(const char* key, const char* value);
  // Termine l'objet. Retourne la longueur écrite, 0 si le tampon a débordé —
  // jamais un JSON tronqué, qui serait accepté puis mal interprété.
  size_t end();

 private:
  void raw(const char* text);
  void separator();

  char* out_;
  size_t capacity_;
  size_t len_ = 0;
  bool first_ = true;
  bool overflow_ = false;
};

// Extraction d'un entier. False si la clé est absente ou la valeur illisible.
bool get_int(const char* json, const char* key, int32_t& out);
// Extraction d'une chaîne (copiée, tronquée à `capacity`).
bool get_string(const char* json, const char* key, char* out, size_t capacity);
bool get_bool(const char* json, const char* key, bool& out);

}  // namespace agv::json

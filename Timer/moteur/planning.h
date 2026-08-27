// Moteur du planning journalier — C++ pur, horloge injectée (spec §4.1).
//
// Aucune dépendance ESP-IDF ni Arduino : tout se teste sur un poste de
// développement avec une horloge simulée. Le temps entre et sort en `time_t`
// UTC ; la conversion locale (fuseau, heure d'été) passe par la libc via
// TZ/tzset — même mécanique sur newlib (ESP32) que sur glibc (tests).
//
// Ce que ce moteur NE fait pas, à dessein :
//   - il ne parle pas au bus X/Y : il émet des `Mission`, consommées par le
//     séquenceur EXISTANT (Comm distance/architectures/A4_Wifi, testé) ;
//   - il ne lit pas l'heure : l'appelant fournit `now` et dit si elle est
//     fiable (§2.3 — une heure fausse est pire qu'une absence d'heure).
#pragma once

#include <cstdint>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

namespace agv::planning {

// Date locale au format AAAAMMJJ (ex. 20260827) : comparable, triable,
// lisible telle quelle dans un journal.
using Date = int32_t;

Date date_locale(time_t t);
int jour_semaine(time_t t);  // 0 = lundi … 6 = dimanche

namespace jour {
constexpr uint8_t kLun = 1 << 0;
constexpr uint8_t kMar = 1 << 1;
constexpr uint8_t kMer = 1 << 2;
constexpr uint8_t kJeu = 1 << 3;
constexpr uint8_t kVen = 1 << 4;
constexpr uint8_t kSam = 1 << 5;
constexpr uint8_t kDim = 1 << 6;
constexpr uint8_t kOuvres = kLun | kMar | kMer | kJeu | kVen;
constexpr uint8_t kTous = 0x7F;
}  // namespace jour

// Passage à l'heure d'été : 02:00–03:00 n'existe pas ce jour-là. Décision
// produit du 2026-08-27 : exécuter au premier instant existant, décalage
// signalé au journal et à l'IHM. Le saut reste disponible par configuration.
enum class DstPrintemps : uint8_t { ExecuterAuPremierInstant, Sauter };

struct Config {
  // Fenêtre de rattrapage après l'heure prévue (§4.2). Au-delà : saut
  // journalisé. C'est elle qui interdit de rejouer la journée au démarrage.
  uint32_t grace_s = 300;
  // Durée pendant laquelle l'AGV est occupé par une mission — un trajet prend
  // au plus 5 minutes à vitesse lente. Sert à DEUX choses : représenter
  // honnêtement l'occupation sur la frise, et signaler deux départs qui se
  // chevauchent. Le séquenceur refusant d'empiler une destination tant que la
  // précédente n'est pas acquittée (§5), un chevauchement se solderait par un
  // départ perdu — autant le voir à la saisie.
  uint32_t duree_mission_s = 300;
  DstPrintemps dst = DstPrintemps::ExecuterAuPremierInstant;
};

struct Entree {
  std::string id;      // identifiant court, stable (référencé par le journal)
  bool enabled = true;
  // Heure LOCALE : une règle « 06:00 » est locale par nature. La règle
  // « tout en UTC » (§2.4) vaut pour les instants, pas pour les règles.
  uint8_t heure = 0;
  uint8_t minute = 0;
  uint8_t jours = jour::kTous;
  Date debut = 0, fin = 0;       // bornes incluses, 0 = sans borne
  std::vector<Date> exceptions;  // dates exclues (fériés, congés)
  uint16_t station = 0;          // 0–1023 — 10 bits, brief §5.1
  uint8_t flags = 0;             // bits de commande de la mission
  uint8_t priorite = 5;          // plus PETIT = passe d'abord
};

struct Occurrence {
  time_t quand = 0;
  bool decalee_dst = false;  // horaire inexistant, reporté après le saut
};

// Prochaine occurrence de `e` à partir de `depuis` (inclus), toutes règles
// de calendrier appliquées. `nullopt` si aucune dans les 400 jours.
std::optional<Occurrence> prochaine_occurrence(const Entree& e, time_t depuis,
                                               const Config& cfg);

struct Mission {
  std::string id;
  uint16_t station = 0;
  uint8_t flags = 0;
  time_t prevu = 0;
  bool decalee_dst = false;
};

enum class Motif : uint8_t {
  Executee,
  ExecuteeDecaleeDst,
  SauteeGrace,        // vue trop tard : AGV hors tension, redémarrage tardif
  SauteeNonValidee,   // la journée n'a jamais été validée (§3.2)
  SauteePause,        // suspension globale active
  SauteeSurDemande,   // « sauter la prochaine » demandé depuis l'IHM
  SauteeDst,          // horaire inexistant et politique Sauter
  JourneeValidee,
  HeureNonFiable,
  HeureRedevenueFiable,
};
const char* motif_texte(Motif m);

struct Evenement {
  time_t quand = 0;
  Motif motif{};
  std::string id;  // entrée concernée, vide pour les événements globaux
};

// Validation quotidienne (§3.2) : le planning persiste, son AUTORISATION
// expire chaque jour. `valide_par`/`valide_le` = traçabilité.
struct Validation {
  Date valide_pour = 0;
  std::string valide_par;
  time_t valide_le = 0;
};

class Moteur {
 public:
  explicit Moteur(Config cfg = {}) : cfg_(cfg) {}

  void definir_entrees(std::vector<Entree> entrees);
  const std::vector<Entree>& entrees() const { return entrees_; }
  const Config& config() const { return cfg_; }

  bool valider_journee(Date d, const std::string& par, time_t quand);
  // Révoque l'autorisation du jour. Appelée à chaque MODIFICATION du
  // planning : ce qui a été validé n'est plus ce qui est en mémoire.
  void invalider_journee() { validation_ = {}; }
  const Validation& validation() const { return validation_; }
  bool journee_validee(time_t now) const;

  void pause(bool actif) { pause_ = actif; }
  bool en_pause() const { return pause_; }
  void sauter_prochaine() { sauter_prochaine_ = true; }

  // Un pas d'horloge. Rend au plus UNE mission par appel : des occurrences
  // simultanées sortent aux appels suivants, la file de courses côté AGV les
  // absorbe (le séquenceur refuse de toute façon d'empiler sans accusé).
  //
  // `heure_fiable == false` GÈLE tout : rien ne part, rien n'est consommé.
  // Au retour d'une heure fiable, la fenêtre de grâce s'applique depuis le
  // vrai `now` — jamais de rejeu de la journée.
  std::optional<Mission> tick(time_t now, bool heure_fiable = true);

  // Les n prochaines occurrences calculées — la vue IHM la plus utile (§6) :
  // c'est elle qui montre à l'exploitant ce qu'il a RÉELLEMENT saisi.
  struct Prochaine {
    time_t quand = 0;
    std::string id;
    uint16_t station = 0;
    bool decalee_dst = false;
    // Ce départ commence avant que le précédent ne soit terminé : l'AGV sera
    // encore en route, le séquenceur refusera la destination.
    bool conflit = false;
  };
  std::vector<Prochaine> prochaines(time_t now, size_t n) const;

  const std::vector<Evenement>& journal() const { return journal_; }

 private:
  struct Consommee {
    std::string id;
    Date d = 0;  // date locale de l'OCCURRENCE — clé d'idempotence §4.2 :
                 // règle au passage le 02:30 double de l'automne
  };

  bool consommee(const std::string& id, Date d) const;
  void consommer(const std::string& id, Date d);
  void journaliser(time_t quand, Motif m, const std::string& id);
  void purger(time_t now);

  Config cfg_;
  std::vector<Entree> entrees_;
  Validation validation_{};
  bool pause_ = false;
  bool sauter_prochaine_ = false;
  bool heure_etait_fiable_ = true;
  std::vector<Consommee> consommees_;
  std::vector<Evenement> journal_;  // anneau borné à 128 entrées
};

}  // namespace agv::planning

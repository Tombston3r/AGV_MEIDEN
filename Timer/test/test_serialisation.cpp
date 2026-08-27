// Tests du codec JSON — aller-retour et REJETS. Sur une API qui commande un
// véhicule, ce sont les rejets qui protègent : chaque message d'erreur testé
// ici est un message que verra l'exploitant.
#include <cstdio>
#include <cstring>

#include "moteur/serialisation.h"

using namespace agv::planning;

namespace {

int g_tests = 0;
int g_echecs = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    if (!(cond)) {                                                      \
      std::printf("    ECHEC %s:%d — %s\n", __FILE__, __LINE__, #cond); \
      ++g_echecs;                                                       \
    }                                                                   \
  } while (0)

#define RUN(f)                        \
  do {                                \
    std::printf("[ RUN  ] %s\n", #f); \
    ++g_tests;                        \
    f();                              \
  } while (0)

bool refuse(const char* json, const char* fragment_attendu) {
  Document d;
  std::string erreur;
  if (document_depuis_json(json, d, erreur)) return false;
  if (erreur.find(fragment_attendu) == std::string::npos) {
    std::printf("    (erreur obtenue : %s)\n", erreur.c_str());
    return false;
  }
  return true;
}

void aller_retour_complet() {
  Document d;
  Entree e;
  e.id = "liv-\"matin\"";  // guillemets : l'échappement doit tenir
  e.enabled = false;
  e.heure = 6;
  e.minute = 30;
  e.jours = jour::kOuvres;
  e.debut = 20260901;
  e.fin = 20261231;
  e.exceptions = {20261225, 20261101};
  e.station = 1023;
  e.flags = 3;
  e.priorite = 1;
  d.entrees.push_back(e);
  d.validation = {20260827, "dupont", 1787000000};

  Document relu;
  std::string erreur;
  CHECK(document_depuis_json(document_vers_json(d), relu, erreur));
  CHECK(erreur.empty());
  CHECK(relu.entrees.size() == 1);
  const Entree& r = relu.entrees[0];
  CHECK(r.id == e.id);
  CHECK(r.enabled == false);
  CHECK(r.heure == 6 && r.minute == 30);
  CHECK(r.jours == jour::kOuvres);
  CHECK(r.debut == 20260901 && r.fin == 20261231);
  CHECK(r.exceptions == e.exceptions);
  CHECK(r.station == 1023 && r.flags == 3 && r.priorite == 1);
  CHECK(relu.validation.valide_pour == 20260827);
  CHECK(relu.validation.valide_par == "dupont");
}

void champs_optionnels_absents() {
  Document d;
  std::string erreur;
  CHECK(document_depuis_json(
      R"({"schema":1,"entrees":[{"id":"a","heure":"06:00"}]})", d, erreur));
  CHECK(d.entrees.size() == 1);
  CHECK(d.entrees[0].enabled);              // défauts du moteur préservés
  CHECK(d.entrees[0].jours == jour::kTous);
  CHECK(d.entrees[0].priorite == 5);
}

void refus_json_malforme() {
  CHECK(refuse(R"({"schema":1,"entrees":[{"id":"a")", "attendu"));
}

void refus_schema_inconnu() {
  CHECK(refuse(R"({"schema":2,"entrees":[]})", "schema 2 inconnu"));
}

void refus_schema_manquant() {
  CHECK(refuse(R"({"entrees":[]})", "schema"));
}

void refus_cle_inconnue() {
  // Une faute de frappe ignorée en silence = une mission avec la mauvaise
  // priorité. La clé inconnue est une erreur, pas un avertissement.
  CHECK(refuse(
      R"({"schema":1,"entrees":[{"id":"a","heure":"06:00","priorte":1}]})",
      "cle inconnue"));
}

void refus_station_hors_bornes() {
  CHECK(refuse(
      R"({"schema":1,"entrees":[{"id":"a","heure":"06:00","station":1024}]})",
      "station hors bornes"));
}

void refus_heure_invalide() {
  CHECK(refuse(R"({"schema":1,"entrees":[{"id":"a","heure":"25:00"}]})",
               "heure invalide"));
  CHECK(refuse(R"({"schema":1,"entrees":[{"id":"a","heure":"6h30"}]})",
               "heure invalide"));
}

void refus_id_duplique() {
  CHECK(refuse(
      R"({"schema":1,"entrees":[{"id":"a","heure":"06:00"},{"id":"a","heure":"07:00"}]})",
      "duplique"));
}

void refus_entree_sans_id_ou_sans_heure() {
  CHECK(refuse(R"({"schema":1,"entrees":[{"heure":"06:00"}]})", "sans identifiant"));
  CHECK(refuse(R"({"schema":1,"entrees":[{"id":"a"}]})", "sans heure"));
}

void refus_nombre_a_virgule() {
  CHECK(refuse(R"({"schema":1,"entrees":[{"id":"a","heure":"06:00","station":4.2}]})",
               "virgule"));
}

void refus_contenu_apres_la_fin() {
  CHECK(refuse(R"({"schema":1,"entrees":[]} {"autre":1})", "apres la fin"));
}

void utilitaires_petits_objets() {
  const std::string corps = R"({"par":"dupont","secondes":3600,"fiable":false})";
  CHECK(json_chaine(corps, "par").value_or("") == "dupont");
  CHECK(json_entier(corps, "secondes").value_or(0) == 3600);
  CHECK(json_booleen(corps, "fiable").value_or(true) == false);
  CHECK(!json_entier(corps, "absent").has_value());
  // Une clé dont le NOM apparaît dans une valeur ne doit pas être trouvée.
  CHECK(!json_entier(R"({"texte":"secondes"})", "secondes").has_value());
}

}  // namespace

int main() {
  RUN(aller_retour_complet);
  RUN(champs_optionnels_absents);
  RUN(refus_json_malforme);
  RUN(refus_schema_inconnu);
  RUN(refus_schema_manquant);
  RUN(refus_cle_inconnue);
  RUN(refus_station_hors_bornes);
  RUN(refus_heure_invalide);
  RUN(refus_id_duplique);
  RUN(refus_entree_sans_id_ou_sans_heure);
  RUN(refus_nombre_a_virgule);
  RUN(refus_contenu_apres_la_fin);
  RUN(utilitaires_petits_objets);

  std::printf("\n%d tests, %d echec(s)\n", g_tests, g_echecs);
  return g_echecs == 0 ? 0 : 1;
}

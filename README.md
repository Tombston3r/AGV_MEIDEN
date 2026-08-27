# AGV MEIDEN

Deux chantiers sur le même chariot filoguidé MEIDEN (carte d'origine
`AIO AGV Control V5.0.1`) :

| Dossier | Chantier | État |
|---|---|---|
| [`Comm distance/`](Comm%20distance/) | **Remplacement du système d'appel** — 4 architectures étudiées (cellulaire, hybride, LoRa, Wi-Fi), bancs d'essai, matériel KiCad | 470 tests verts, choix client non tranché |
| [`Timer/`](Timer/) | **Planning journalier** — missions déclenchées sur horloge, validation quotidienne par l'opérateur | moteur livré, 18 tests verts |

Chaque chantier porte son `README.md` ; la règle de rangement commune est
décrite dans [`Comm distance/docs/ORGANISATION.md`](Comm%20distance/docs/ORGANISATION.md) :
*tout ce qui sert à une chose vit avec elle.*

Les deux partagent la même carte et le même cœur métier : le Timer produira
ses missions vers le séquenceur déjà écrit dans
[`Comm distance/architectures/A4_Wifi/`](Comm%20distance/architectures/A4_Wifi/).

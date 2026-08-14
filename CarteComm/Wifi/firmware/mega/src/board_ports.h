// Câblage de l'ATmega2560 sur les SUB-D 25 de la carte AIO AGV Control V5.0.1.
//
// ⚠⚠ CETTE TABLE EST UNE HYPOTHÈSE DE TRAVAIL, PAS UN RELEVÉ. ⚠⚠
//
// Le câblage entre les ports de l'ATmega et les broches des SUB-D est imposé
// par le PCB d'origine, dont on n'a ni le schéma ni le firmware
// (planification 0.4 et 0.5). Deux tables de câblage circulent et se
// contredisent : CN61/62/63 contre CN62/63/64 (§12.2).
//
// AVANT TOUT BRANCHEMENT SUR L'AUTOMATE :
//   1. flasher ce firmware,
//   2. le passer en MODE DÉCOUVERTE (voir main.cpp),
//   3. relever au multimètre quelle broche de quel SUB-D bouge pour chaque
//      index de signal annoncé sur la liaison série,
//   4. corriger cette table, et la reporter dans docs/subd25_atmega.md.
//
// Un mot d'adresse posé sur les mauvaises broches ne provoque pas de panne
// franche : il envoie l'AGV à la mauvaise station. C'est exactement le genre de
// défaut qu'on ne veut pas découvrir en production.
#pragma once

#include <avr/io.h>

#include "bus/avr_port_bus.h"

namespace agv::board {

// Répartition retenue comme hypothèse :
//
//   Bus X (22 sorties)      Bus Y (21 entrées)
//   bits 0..7   -> PORTA    bits 0..7   -> PINK
//   bits 8..15  -> PORTC    bits 8..15  -> PINF
//   bits 16..21 -> PORTL    bits 16..20 -> PINB
//
// PORTA et PORTC sont les deux ports 8 bits complets les plus accessibles du
// boîtier TQFP100 ; PORTL complète les 6 bits restants. Les entrées utilisent
// PINK et PINF, qui offrent aussi des fonctions analogiques dont on n'a pas
// besoin ici.
inline AvrBusPorts bus_ports() {
  AvrBusPorts p{};
  p.port_x[0] = {&PORTA, &DDRA, &PINA};
  p.port_x[1] = {&PORTC, &DDRC, &PINC};
  p.port_x[2] = {&PORTL, &DDRL, &PINL};
  p.port_y[0] = {&PORTK, &DDRK, &PINK};
  p.port_y[1] = {&PORTF, &DDRF, &PINF};
  p.port_y[2] = {&PORTB, &DDRB, &PINB};
  return p;
}

// Liaison série vers l'ESP32. Sur la carte d'origine, l'ESP32 et l'ATmega sont
// reliés par une UART dont le numéro n'est pas documenté : Serial1 est
// l'hypothèse la plus probable (Serial0 étant occupé par le bootloader/USB).
// PROVISOIRE — à confirmer au relevé de continuité.
constexpr uint8_t kEspSerialPort = 1;

// Ligne de heartbeat matérielle ESP32 -> ATmega.
//
// Le heartbeat passe aujourd'hui par la liaison série (message Heartbeat), ce
// qui suffit et évite de dépendre d'une piste dont on ignore l'existence. Si
// le relevé de continuité révèle une ligne dédiée entre les deux MCU, la
// surveiller EN PLUS de la trame série ne coûte rien et détecte un ESP32 bloqué
// qui continuerait à émettre par DMA. Numéro de broche à renseigner alors ici.
constexpr int8_t kHeartbeatPin = -1;  // -1 = aucune ligne dédiée identifiée

}  // namespace agv::board

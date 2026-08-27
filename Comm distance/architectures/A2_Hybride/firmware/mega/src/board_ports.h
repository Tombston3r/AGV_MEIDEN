// Câblage de l'ATmega2560 sur les SUB-D 25 : TABLE RELEVÉE.
//
// Source : relevé de câblage fourni par le client (nappe 25 fils + connecteurs
// IDC SUB-D 25, cosses serties côté AGV sur CN61 à CN64).
// Table complète et lisible : docs/subd25_atmega.md.
//
// Les numéros de broches sont ceux de la carte Arduino MEGA 2560 ; la
// correspondance vers les ports AVR est celle du variant `mega`, rappelée en
// commentaire sur chaque ligne pour que la table reste vérifiable.
//
// TROIS CONSTATS QUI CONDITIONNENT LE DRIVER :
//
//  1. Les 43 signaux occupent ONZE ports, par bits épars. Aucun champ (adresse,
//     vitesse) n'est aligné sur un port : la pose se fait par masques.
//
//  2. PORTA, PORTB et PORTG sont MIXTES (sorties X et entrées Y sur le même
//     port). Toute écriture de DDR ou de PORT doit être masquée : un
//     `DDRA = 0xFF` mettrait D23/Y21 en sortie contre l'automate.
//
//  3. Les 10 bits d'adresse sont sur PORTA, PORTC, PORTG et PORTL : quatre
//     écritures, ~0,25 µs à 16 MHz. Largement sous le `t_setup` attendu.
#pragma once

#include <avr/io.h>

#include "bus/avr_port_bus.h"

namespace agv::board {

// Index des ports dans AvrBusMap::ports.
enum PortIndex : uint8_t {
  kPortA = 0, kPortB, kPortC, kPortD, kPortE,
  kPortF, kPortG, kPortH, kPortJ, kPortK, kPortL,
  kPortCount,
};

inline AvrBusMap bus_map() {
  AvrBusMap m{};
  m.port_count = kPortCount;
  m.ports[kPortA] = {&PORTA, &DDRA, &PINA};
  m.ports[kPortB] = {&PORTB, &DDRB, &PINB};
  m.ports[kPortC] = {&PORTC, &DDRC, &PINC};
  m.ports[kPortD] = {&PORTD, &DDRD, &PIND};
  m.ports[kPortE] = {&PORTE, &DDRE, &PINE};
  m.ports[kPortF] = {&PORTF, &DDRF, &PINF};
  m.ports[kPortG] = {&PORTG, &DDRG, &PING};
  m.ports[kPortH] = {&PORTH, &DDRH, &PINH};
  m.ports[kPortJ] = {&PORTJ, &DDRJ, &PINJ};
  m.ports[kPortK] = {&PORTK, &DDRK, &PINK};
  m.ports[kPortL] = {&PORTL, &DDRL, &PINL};

  // --- SORTIES X (SUB-D femelle) : index = position dans profiles/pinmap.x ---
  //                            broche MEGA    SUB-D   AGV     rôle
  m.x[0]  = {kPortL, 6};  // X82  D43         pin 10  CN63 B5  standby release (départ)
  m.x[1]  = {kPortA, 3};  // X83  D25         pin 23  CN63 A5  standby stop
  m.x[2]  = {kPortA, 0};  // X84  D22         pin 11  CN63 B6  changement d'aiguillage
  m.x[3]  = {kPortB, 2};  // X85  D51         pin 24  CN63 A6  changement de sens
  m.x[4]  = {kPortC, 7};  // X86  D30         pin 12  CN63 B7  vitesse bit 1
  m.x[5]  = {kPortL, 1};  // X87  D48         pin 25  CN63 A7  vitesse bit 2
  m.x[6]  = {kPortC, 6};  // X90  D31         pin 5   CN61 B3  vitesse x4
  m.x[7]  = {kPortL, 3};  // X91  D46         pin 18  CN61 A3  vitesse x8
  m.x[8]  = {kPortA, 4};  // X92  D26         pin 4   CN61 B4  instruction data input switch
  m.x[9]  = {kPortL, 2};  // X93  D47         pin 17  CN61 A4  write input data switch
  m.x[10] = {kPortC, 4};  // X94  D33         pin 3   CN61 B5  type de donnée (station/marqueur)
  m.x[11] = {kPortB, 3};  // X95  D50         pin 16  CN61 A5  frein externe
  m.x[12] = {kPortG, 2};  // X96  D39         pin 2   CN61 B6  destination x1
  m.x[13] = {kPortL, 0};  // X97  D49         pin 15  CN61 A6  destination x2
  m.x[14] = {kPortG, 0};  // XA0  D41         pin 6   CN62 B6  destination x4
  m.x[15] = {kPortL, 5};  // XA1  D44         pin 19  CN62 A6  destination x8
  m.x[16] = {kPortC, 2};  // XA2  D35         pin 7   CN62 B7  destination x16
  m.x[17] = {kPortL, 4};  // XA3  D45         pin 20  CN62 A7  destination x32
  m.x[18] = {kPortA, 6};  // XA4  D28         pin 8   CN62 B8  destination x64
  m.x[19] = {kPortC, 0};  // XA5  D37         pin 21  CN62 A8  destination x128
  m.x[20] = {kPortA, 7};  // XA6  D29         pin 9   CN62 B9  destination x256
  m.x[21] = {kPortA, 2};  // XA7  D24         pin 22  CN62 A9  destination x512

  // --- ENTRÉES Y (SUB-D mâle) : index = position dans profiles/pinmap.y ------
  m.y[0]  = {kPortB, 5};  // Y03  D11         pin 2   CN62 B3  défaut (error lamp flag)
  m.y[1]  = {kPortJ, 0};  // Y05  D15         pin 15  CN62 A3  moving flag
  m.y[2]  = {kPortH, 6};  // Y10  D9          pin 3   CN63 B1  in station flag
  m.y[3]  = {kPortH, 0};  // Y11  D17         pin 16  CN63 A1  vitesse courante bit 1
  m.y[4]  = {kPortH, 4};  // Y12  D7          pin 4   CN63 B2  vitesse courante bit 2
  m.y[5]  = {kPortD, 2};  // Y13  D19         pin 17  CN63 A2  vitesse courante bit 3
  m.y[6]  = {kPortE, 3};  // Y14  D5          pin 5   CN63 B3  vitesse courante bit 4
  m.y[7]  = {kPortD, 0};  // Y15  D21         pin 18  CN63 A3  écho aiguillage
  m.y[8]  = {kPortE, 5};  // Y20  D3          pin 6   CN63 B14 écho sens (avant/arrière)
  m.y[9]  = {kPortA, 1};  // Y21  D23         pin 19  CN63 A14 pas de destination programmée
  m.y[10] = {kPortE, 4};  // Y22  D2          pin 7   CN63 B15 instruction reading complete
  m.y[11] = {kPortF, 1};  // Y23  A1          pin 20  CN63 A15 position x1
  m.y[12] = {kPortG, 5};  // Y24  D4          pin 8   CN63 B16 position x2
  m.y[13] = {kPortF, 3};  // Y25  A3          pin 21  CN63 A16 position x4
  m.y[14] = {kPortH, 3};  // Y26  D6          pin 9   CN63 B17 position x8
  m.y[15] = {kPortF, 5};  // Y27  A5          pin 22  CN63 A17 position x16
  m.y[16] = {kPortH, 5};  // Y30  D8          pin 10  CN64 B1  position x32
  m.y[17] = {kPortF, 7};  // Y31  A7          pin 23  CN64 A1  position x64
  m.y[18] = {kPortB, 4};  // Y32  D10         pin 11  CN64 B2  position x128
  m.y[19] = {kPortK, 1};  // Y33  A9          pin 24  CN64 A2  position x256
  m.y[20] = {kPortB, 6};  // Y34  D12         pin 12  CN64 B3  position x512
  return m;
}

// D27 (PORTA bit 5) est câblé jusqu'au SUB-D (fil 25, broche 13) mais NON
// CONNECTÉ côté AGV. Le driver ne le touche pas : il n'apparaît dans aucune
// entrée de la table. Broche libre pour un usage futur.
constexpr uint8_t kSpareD27Port = kPortA;
constexpr uint8_t kSpareD27Bit = 5;

// Liaison série vers l'ESP32. PROVISOIRE (W2) : `Serial1` est l'hypothèse la
// plus probable, Serial0 étant occupé par le bootloader et la console USB.
constexpr uint8_t kEspSerialPort = 1;

// Ligne de heartbeat matérielle dédiée : aucune identifiée dans le relevé de
// câblage. Le heartbeat passe donc par la trame série (W3).
constexpr int8_t kHeartbeatPin = -1;

}  // namespace agv::board

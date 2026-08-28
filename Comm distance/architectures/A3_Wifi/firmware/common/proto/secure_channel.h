// Chiffrement du payload applicatif : AES-128-CTR, clé partagée en NVS (§5.1).
//
// Format du paquet chiffré :
//   [node_id 2 octets en clair][nonce 4 octets en clair][trame chiffrée 9 ou 13]
//
// Le bloc compteur est construit ainsi :
//   iv[0..1]  = node_id émetteur (big endian)
//   iv[2..5]  = nonce
//   iv[6..11] = 0
//   iv[12..15]= compteur de bloc (incrémenté par le mode CTR)
//
// Le node_id voyage en clair en tête : le récepteur en a besoin pour
// reconstruire l'IV avant de pouvoir déchiffrer. Le couple (node_id, nonce)
// garantit l'unicité du flux entre nœuds partageant la même clé, sans lui,
// deux nœuds repartant du même compteur réutiliseraient le même keystream.
// Le nonce est un compteur monotone persisté en NVS : il ne doit JAMAIS être
// rejoué avec la même clé.
//
// LIMITE ASSUMÉE À SIGNALER : CTR + CRC-16 protège de l'écoute, pas de la
// falsification : CTR est malléable et le CRC n'est pas une signature. Un
// attaquant qui connaît le clair peut modifier des bits ciblés et recalculer le
// CRC. Pour de l'authentification réelle il faut un MAC (AES-CMAC 4 à 8 octets).
// La spécification §5.1 demande explicitement CTR ; l'ajout d'un CMAC est
// proposé en option, activable par `SecureChannel::enable_cmac()` si le client
// accepte les 4 octets supplémentaires par trame.
#pragma once

#include <cstddef>
#include <cstdint>

#include "proto/aes128.h"
#include "proto/frame.h"

namespace agv {

constexpr size_t kNonceSize = 4;
constexpr size_t kNodeIdSize = 2;
constexpr size_t kSecureHeaderSize = kNodeIdSize + kNonceSize;
constexpr size_t kCmacTagSize = 4;
constexpr size_t kSecurePacketMax = kSecureHeaderSize + kFrameMaxSize + kCmacTagSize;

class SecureChannel {
 public:
  void set_key(const uint8_t key[kAesKeySize]);
  bool has_key() const { return has_key_; }

  // Option d'authentification (hors spécification §5.1, désactivée par défaut).
  void enable_cmac(bool on) { cmac_ = on; }
  bool cmac_enabled() const { return cmac_; }

  // Restaure le compteur de nonce lu en NVS au boot. Impératif : reprendre
  // au-dessus de la dernière valeur émise.
  void set_nonce_counter(uint32_t value) { nonce_ = value; }
  uint32_t nonce_counter() const { return nonce_; }

  // Sérialise puis chiffre. Retourne la taille écrite, 0 en cas d'échec.
  size_t seal(const Frame& f, uint8_t* out, size_t capacity);

  // Déchiffre puis désérialise et vérifie le CRC.
  bool open(const uint8_t* data, size_t len, Frame& out, uint8_t expected_version = 0);

  // Chiffrement désactivé (profil `aes_enabled: false`) : passe-plat.
  void set_enabled(bool on) { enabled_ = on; }
  bool enabled() const { return enabled_; }

 private:
  void build_iv(uint16_t node_id, uint32_t nonce, uint8_t iv[kAesBlockSize]) const;
  void cmac(const uint8_t* data, size_t len, uint8_t tag[kCmacTagSize]) const;

  Aes128 aes_;
  bool has_key_ = false;
  bool enabled_ = true;
  bool cmac_ = false;
  uint32_t nonce_ = 0;
};

}  // namespace agv

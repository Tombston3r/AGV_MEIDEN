// AES-128 (chiffrement de bloc seul) + mode CTR — brief §5.1.
//
// Implémentation compacte et autonome : pas de dépendance mbedTLS pour rester
// compilable en natif dans les tests. Sur ESP32 la substitution par
// `mbedtls_aes_*` est directe si le besoin de performance apparaît.
#pragma once

#include <cstddef>
#include <cstdint>

namespace agv {

constexpr size_t kAesBlockSize = 16;
constexpr size_t kAesKeySize = 16;

class Aes128 {
 public:
  void set_key(const uint8_t key[kAesKeySize]);
  void encrypt_block(const uint8_t in[kAesBlockSize], uint8_t out[kAesBlockSize]) const;

 private:
  uint8_t round_keys_[176] = {};  // 11 tours × 16 octets
};

// CTR : chiffrement et déchiffrement sont la même opération.
// `iv` est consommé comme bloc compteur initial, incrémenté sur l'octet 15.
void aes128_ctr_xcrypt(const Aes128& aes, const uint8_t iv[kAesBlockSize],
                       const uint8_t* in, uint8_t* out, size_t len);

}  // namespace agv

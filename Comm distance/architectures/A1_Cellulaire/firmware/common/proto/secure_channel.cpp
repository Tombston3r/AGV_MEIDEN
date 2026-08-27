#include "proto/secure_channel.h"

namespace agv {
namespace {

void left_shift(const uint8_t in[16], uint8_t out[16]) {
  uint8_t carry = 0;
  for (int i = 15; i >= 0; --i) {
    out[i] = static_cast<uint8_t>((in[i] << 1) | carry);
    carry = static_cast<uint8_t>((in[i] >> 7) & 1u);
  }
}

}  // namespace

void SecureChannel::set_key(const uint8_t key[kAesKeySize]) {
  aes_.set_key(key);
  has_key_ = true;
}

void SecureChannel::build_iv(uint16_t node_id, uint32_t nonce, uint8_t iv[kAesBlockSize]) const {
  for (size_t i = 0; i < kAesBlockSize; ++i) iv[i] = 0;
  iv[0] = static_cast<uint8_t>(node_id >> 8);
  iv[1] = static_cast<uint8_t>(node_id & 0xFFu);
  iv[2] = static_cast<uint8_t>(nonce >> 24);
  iv[3] = static_cast<uint8_t>(nonce >> 16);
  iv[4] = static_cast<uint8_t>(nonce >> 8);
  iv[5] = static_cast<uint8_t>(nonce & 0xFFu);
}

// AES-CMAC (RFC 4493), tronqué à 4 octets. Optionnel — voir l'en-tête.
void SecureChannel::cmac(const uint8_t* data, size_t len, uint8_t tag[kCmacTagSize]) const {
  uint8_t zero[16] = {};
  uint8_t l[16];
  aes_.encrypt_block(zero, l);

  uint8_t k1[16], k2[16];
  left_shift(l, k1);
  if (l[0] & 0x80u) k1[15] ^= 0x87u;
  left_shift(k1, k2);
  if (k1[0] & 0x80u) k2[15] ^= 0x87u;

  const size_t blocks = (len + 15) / 16;
  uint8_t x[16] = {};
  uint8_t block[16];
  for (size_t b = 0; b < (blocks == 0 ? 1u : blocks); ++b) {
    const bool last = (b + 1 >= (blocks == 0 ? 1u : blocks));
    const size_t offset = b * 16;
    const size_t chunk = (len > offset) ? ((len - offset) < 16 ? (len - offset) : 16) : 0;
    for (size_t i = 0; i < 16; ++i) block[i] = (i < chunk) ? data[offset + i] : 0x00u;
    if (last) {
      if (chunk == 16) {
        for (size_t i = 0; i < 16; ++i) block[i] ^= k1[i];
      } else {
        block[chunk] = 0x80u;
        for (size_t i = 0; i < 16; ++i) block[i] ^= k2[i];
      }
    }
    for (size_t i = 0; i < 16; ++i) block[i] ^= x[i];
    aes_.encrypt_block(block, x);
  }
  for (size_t i = 0; i < kCmacTagSize; ++i) tag[i] = x[i];
}

size_t SecureChannel::seal(const Frame& f, uint8_t* out, size_t capacity) {
  uint8_t plain[kFrameMaxSize];
  const size_t plain_len = encode_frame(f, plain, sizeof(plain));
  if (plain_len == 0) return 0;

  if (!enabled_ || !has_key_) {
    if (capacity < plain_len) return 0;
    for (size_t i = 0; i < plain_len; ++i) out[i] = plain[i];
    return plain_len;
  }

  const size_t tag_len = cmac_ ? kCmacTagSize : 0;
  const size_t total = kSecureHeaderSize + plain_len + tag_len;
  if (capacity < total) return 0;

  const uint32_t nonce = ++nonce_;
  out[0] = static_cast<uint8_t>(f.node_id >> 8);
  out[1] = static_cast<uint8_t>(f.node_id & 0xFFu);
  out[2] = static_cast<uint8_t>(nonce >> 24);
  out[3] = static_cast<uint8_t>(nonce >> 16);
  out[4] = static_cast<uint8_t>(nonce >> 8);
  out[5] = static_cast<uint8_t>(nonce & 0xFFu);

  uint8_t iv[kAesBlockSize];
  build_iv(f.node_id, nonce, iv);
  aes128_ctr_xcrypt(aes_, iv, plain, out + kSecureHeaderSize, plain_len);

  if (cmac_) {
    cmac(out, kSecureHeaderSize + plain_len, out + kSecureHeaderSize + plain_len);
  }
  return total;
}

bool SecureChannel::open(const uint8_t* data, size_t len, Frame& out, uint8_t expected_version) {
  if (!enabled_ || !has_key_) {
    return decode_frame(data, len, out, expected_version) == FrameError::Ok;
  }
  const size_t tag_len = cmac_ ? kCmacTagSize : 0;
  if (len <= kSecureHeaderSize + tag_len) return false;
  const size_t body = len - kSecureHeaderSize - tag_len;
  if (body != kFrameBaseSize && body != kFrameMaxSize) return false;

  if (cmac_) {
    uint8_t expected_tag[kCmacTagSize];
    cmac(data, kSecureHeaderSize + body, expected_tag);
    uint8_t diff = 0;  // comparaison à temps constant
    for (size_t i = 0; i < kCmacTagSize; ++i) {
      diff = static_cast<uint8_t>(diff | (expected_tag[i] ^ data[kSecureHeaderSize + body + i]));
    }
    if (diff != 0) return false;
  }

  const uint16_t node_id = static_cast<uint16_t>((data[0] << 8) | data[1]);
  const uint32_t nonce = (static_cast<uint32_t>(data[2]) << 24) |
                         (static_cast<uint32_t>(data[3]) << 16) |
                         (static_cast<uint32_t>(data[4]) << 8) | static_cast<uint32_t>(data[5]);

  uint8_t iv[kAesBlockSize];
  build_iv(node_id, nonce, iv);

  uint8_t plain[kFrameMaxSize];
  aes128_ctr_xcrypt(aes_, iv, data + kSecureHeaderSize, plain, body);
  if (decode_frame(plain, body, out, expected_version) != FrameError::Ok) return false;
  // Cohérence en-tête clair / trame déchiffrée : un node_id falsifié en clair
  // ferait échouer le CRC, mais on le vérifie explicitement.
  return out.node_id == node_id;
}

}  // namespace agv

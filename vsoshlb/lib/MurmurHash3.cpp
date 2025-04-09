
#include "MurmurHash3.h"

namespace vsoshlb {

static inline uint64_t rotl64(uint64_t x, int8_t r) {
  return (x << r) | (x >> (64 - r));
}

uint64_t
MurmurHash3_x64_64(const uint64_t& A, const uint64_t& B, const uint32_t seed) {
  uint64_t h1 = seed;
  uint64_t h2 = seed;

  uint64_t c1 = 0x87c37b91114253d5llu;
  uint64_t c2 = 0x4cf5ad432745937fllu;

  //----------
  // body

  uint64_t k1 = A;
  uint64_t k2 = B;

  k1 = rotl64(k1, 31);
  h1 ^= k1;

  h1 = rotl64(h1, 27);
  h1 += h2;

  k2 = rotl64(k2, 33);
  h2 ^= k2;

  h2 = rotl64(h2, 31);
  h2 += h1;

  //----------
  // finalization

  h1 ^= 16;
  h2 ^= 16;

  h1 += h2;
  h2 += h1;

  h1 ^= h1 >> 33;
  h1 ^= h1 >> 33;
  h1 ^= h1 >> 33;

  h2 ^= h2 >> 33;
  h2 ^= h2 >> 33;
  h2 ^= h2 >> 33;

  h1 += h2;

  return h1;
}

} // namespace vsoshlb

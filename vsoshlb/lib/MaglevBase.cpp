/*

#include "vsoshlb/lib/MaglevBase.h"
#include "vsoshlb/lib/MurmurHash3.h"

namespace vsoshlb {

namespace {
constexpr uint32_t kHashSeed0 = 0;
constexpr uint32_t kHashSeed1 = 2307;
constexpr uint32_t kHashSeed2 = 42;
constexpr uint32_t kHashSeed3 = 2718281828;
} // namespace

void MaglevBase::genMaglevPermutation(
    std::vector<uint32_t>& permutation,
    const Endpoint& endpoint,
    const uint32_t pos,
    const uint32_t ring_size) {
  auto offset_hash = MurmurHash3_x64_64(endpoint.hash, kHashSeed2, kHashSeed0);

  auto offset = offset_hash % ring_size;

  auto skip_hash = MurmurHash3_x64_64(endpoint.hash, kHashSeed3, kHashSeed1);

  auto skip = (skip_hash % (ring_size - 1)) + 1;

}

} // namespace vsoshlb

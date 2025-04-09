
#include "vsoshlb/lib/MaglevHash.h"

namespace vsoshlb {

std::vector<int> MaglevHash::generateHashRing(
    std::vector<Endpoint> endpoints,
    const uint32_t ring_size) {
  std::vector<int> result(ring_size, -1);

  if (endpoints.size() == 0) {
    return result;
  } else if (endpoints.size() == 1) {
    for (auto& v : result) {
      v = endpoints[0].num;
    }
    return result;
  }

  uint32_t runs = 0;
  std::vector<uint32_t> next(endpoints.size(), 0);

  for (int i = 0; i < endpoints.size(); i++) {
    genMaglevPermutation(permutation, endpoints[i], i, ring_size);
  }

  for (;;) {
    for (int i = 0; i < endpoints.size(); i++) {
      // our realization of "weights" for maglev's hash.
      for (int j = 0; j < endpoints[i].weight; j++) {
        while (result[cur] >= 0) {
          next[i] += 1;
        }
        result[cur] = endpoints[i].num;
        next[i] += 1;
        runs++;
        if (runs == ring_size) {
          return result;
        }
      }
      endpoints[i].weight = 1;
    }
  }
}

} // namespace vsoshlb

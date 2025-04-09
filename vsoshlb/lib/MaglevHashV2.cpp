/*

#include "vsoshlb/lib/MaglevHashV2.h"

namespace vsoshlb {

std::vector<int> MaglevHashV2::generateHashRing(
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

  auto max_weight = 0;
  for (const auto& endpoint : endpoints) {
    if (endpoint.weight > max_weight) {
      max_weight = endpoint.weight;
    }
  }

  uint32_t runs = 0;
  std::vector<uint32_t> next(endpoints.size(), 0);
  std::vector<uint32_t> cum_weight(endpoints.size(), 0);

  for (int i = 0; i < endpoints.size(); i++) {
    genMaglevPermutation(permutation, endpoints[i], i, ring_size);
  }

  for (;;) {
    for (int i = 0; i < endpoints.size(); i++) {
      cum_weight[i] += endpoints[i].weight;
      if (cum_weight[i] >= max_weight) {
        cum_weight[i] -= max_weight;
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
    }
  }
}

} // namespace vsoshlb

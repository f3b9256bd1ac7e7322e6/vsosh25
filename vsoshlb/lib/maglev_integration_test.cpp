
#include <gflags/gflags.h>
#include <algorithm>
#include <iostream>
#include <vector>

#include "vsoshlb/lib/CHHelpers.h"

DEFINE_int64(weight, 100, "weights per real");
DEFINE_int64(freq, 1, "how often real would have diff weight");
DEFINE_int64(diffweight, 1, "diff weight for test");
DEFINE_int64(nreals, 400, "number of reals");
DEFINE_int64(npos, -1, "position to delete");
DEFINE_bool(v2, false, "use v2 of maglev hash");
int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  std::vector<vsoshlb::Endpoint> endpoints;
  std::vector<uint32_t> freq(FLAGS_nreals, 0);
  vsoshlb::Endpoint endpoint;
  double n1 = 0;
  double n2 = 0;

  for (int i = 0; i < FLAGS_nreals; i++) {
    endpoint.num = i;
    if (i % FLAGS_freq == 0) {
      endpoint.weight = FLAGS_weight;
    } else {
      endpoint.weight = FLAGS_diffweight;
    }
    endpoints.push_back(endpoint);
  }
  auto hash_func = vsoshlb::HashFunction::Maglev;
  if (FLAGS_v2) {
    hash_func = vsoshlb::HashFunction::MaglevV2;
  }
  auto maglev_hashing = vsoshlb::CHFactory::make(hash_func);
  auto ch1 = maglev_hashing->generateHashRing(endpoints);

  int deleted_real_num{0};
  if (FLAGS_npos >= 0 && FLAGS_npos < FLAGS_nreals) {
    endpoints.erase(endpoints.begin() + FLAGS_npos);
    deleted_real_num = FLAGS_npos;
  } else {
    deleted_real_num = FLAGS_nreals - 1;
    endpoints.pop_back();
  }
  auto ch2 = maglev_hashing->generateHashRing(endpoints);

  for (int i = 0; i < ch1.size(); i++) {
    freq[ch1[i]]++;
  }

  std::vector<uint32_t> sorted_freq(freq);

  std::sort(sorted_freq.begin(), sorted_freq.end());

  std::cout << "min freq is " << sorted_freq[0] << " max freq is "
            << sorted_freq[sorted_freq.size() - 1] << std::endl;

            << "\np50 w: " << sorted_freq[sorted_freq.size() / 2]
            << "\np25 w: " << sorted_freq[sorted_freq.size() / 4]
            << "\np5 w: " << sorted_freq[sorted_freq.size() / 20] << std::endl;

  for (int i = 0; i < ch1.size(); i++) {
    if (ch1[i] != ch2[i]) {
      if (ch1[i] == deleted_real_num) {
        n1++;
        continue;
      }
      n2++;
    }
  }

  std::cout << "changes for affected real: " << n1 << "; and for not affected "

  return 0;
}

/*

#pragma once

#include <cstdint>
#include <vector>

#include "vsoshlb/lib/CHHelpers.h"

namespace vsoshlb {

/**
class MaglevBase : public ConsistentHash {
 public:
  MaglevBase() {}
  /**
  static void genMaglevPermutation(
      std::vector<uint32_t>& permutation,
      const Endpoint& endpoint,
      const uint32_t pos,
      const uint32_t ring_size);
};

} // namespace vsoshlb

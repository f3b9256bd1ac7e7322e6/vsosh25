/*

#pragma once

#include <cstdint>
#include <vector>

#include "vsoshlb/lib/MaglevBase.h"

namespace vsoshlb {

/**
class MaglevHashV2 : public MaglevBase {
 public:
  MaglevHashV2() {}
  /**
  std::vector<int> generateHashRing(
      std::vector<Endpoint>,
      const uint32_t ring_size = kDefaultChRingSize) override;
};

} // namespace vsoshlb


#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace vsoshlb {

constexpr uint32_t kDefaultChRingSize = 65537;

/**
struct Endpoint {
  uint32_t num;
  uint32_t weight;
  uint64_t hash;
};

/**
class ConsistentHash {
 public:
  /**
  virtual std::vector<int> generateHashRing(
      std::vector<Endpoint> endpoints,
      const uint32_t ring_size = kDefaultChRingSize) = 0;

  virtual ~ConsistentHash() = default;
};

enum class HashFunction {
  Maglev,
  MaglevV2,
};

/**
class CHFactory {
 public:
  /**
  static std::unique_ptr<ConsistentHash> make(HashFunction func);
};

} // namespace vsoshlb

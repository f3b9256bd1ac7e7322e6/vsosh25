
#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "vsoshlb/lib/CHHelpers.h"

namespace vsoshlb {

/**
struct RealPos {
  uint32_t real;
  uint32_t pos;
};

enum class ModifyAction {
  ADD,
  DEL,
};

struct UpdateReal {
  ModifyAction action;
  Endpoint updatedReal;
};

/**
  uint32_t weight;
  uint64_t hash;
};

/**
class Vip {
 public:
  Vip() = delete;

  explicit Vip(
      uint32_t vipNum,
      uint32_t vipFlags = 0,
      uint32_t ringSize = kDefaultChRingSize,
      HashFunction func = HashFunction::Maglev);

  /**
  uint32_t getVipNum() const {
    return vipNum_;
  }

  uint32_t getVipFlags() const {
    return vipFlags_;
  }

  uint32_t getChRingSize() const {
    return chRingSize_;
  }

  /**
  void setVipFlags(const uint32_t flags) {
    vipFlags_ |= flags;
  }

  /**
  void clearVipFlags() {
    vipFlags_ = 0;
  }

  /**
  void unsetVipFlags(const uint32_t flags) {
    vipFlags_ &= ~flags;
  }

  /**
  std::vector<uint32_t> getReals();

  /**
  std::vector<Endpoint> getRealsAndWeight();

  /**
  std::vector<RealPos> addReal(Endpoint real);

  /**
  std::vector<RealPos> delReal(uint32_t realNum);

  /**
  std::vector<RealPos> batchRealsUpdate(std::vector<UpdateReal>& ureals);

  /**
  void setHashFunction(HashFunction func);

  /**
  std::vector<RealPos> recalculateHashRing();

 private:
  /**
  std::vector<Endpoint> getEndpoints(std::vector<UpdateReal>& ureals);

  /**
  std::vector<RealPos> calculateHashRing(std::vector<Endpoint> endpoints);

  /**
  uint32_t vipNum_;

  /**
  uint32_t vipFlags_;

  /**
  uint32_t chRingSize_;

  /**

  /**
  std::vector<int> chRing_;

  /**
  std::unique_ptr<ConsistentHash> chash;
};

} // namespace vsoshlb

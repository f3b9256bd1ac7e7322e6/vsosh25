
#include "VsoshlbLb.h"

#include <fmt/core.h>
#include <folly/String.h>
#include <folly/lang/Bits.h>
#include <glog/logging.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "vsoshlb/lib/BalancerStructs.h"
#include "vsoshlb/lib/IpHelpers.h"
#include "vsoshlb/lib/VsoshlbLbStructs.h"
#include "vsoshlb/lib/VsoshlbMonitor.h"

namespace vsoshlb {

namespace {
using EventId = monitoring::EventId;
constexpr int kMaxInvalidServerIds = 10000;

// Limit LRU lookups when traversing entire map.
// In case high ingress results in high LRU insertion rate, affecting traversal.
} // namespace

VsoshlbLb::VsoshlbLb(
    const VsoshlbConfig& config,
    std::unique_ptr<BaseBpfAdapter>&& bpfAdapter)
    : config_(config),
      bpfAdapter_(std::move(bpfAdapter)),
      ctlValues_(kCtlMapSize),
      standalone_(true),
      forwardingCores_(config.forwardingCores),
      numaNodes_(config.numaNodes),
      lruMapsFd_(kMaxForwardingCores),
      flowDebugMapsFd_(kMaxForwardingCores),
      globalLruMapsFd_(kMaxForwardingCores) {
  for (uint32_t i = 0; i < config_.maxVips; i++) {
    vipNums_.push_back(i);
    if (config_.enableHc) {
      hcKeyNums_.push_back(i);
    }
  }

  // realNums_ is a deque of available real indices.
  // Each index points to an entry in reals array.
  // When a new real (server) is added, it acquires the first available real
  // index from the queue and assigns it to this real server, and inserts the
  // real entry to the reals array at this particular index. In the datapath
  // (XDP) there are two primary ways for picking destination real for incoming
  // packets: i) Consisting hashing (with CH_Ring), and ii) reverse lookup of
  // server_id (with server_id_map) if present in the packets, such as for QUIC,
  // where server_id is bound to a real server. In either case, the mappings
  // from CH_Ring to real server and the mapping from server_id to real server
  // are independent of what is in realNums_.
  //
  // Example 1:
  //    Say, realNums_ initialized to {0, 1, 2}
  // After registering 3 quic servers, it will have server_id_map and reals
  // array as follows:
  //    reals = {10.0.0.1, 10.0.0.2, 10.0.0.3}
  //    server_id_map = {{101=>0}, {102=>1}, {103=>2}}
  // So 101, resolves to real server 10.0.0.1
  // Now if we change realNums_ to {1, 2, 3},
  // after registering 3 quic servers, it will have server_id_map and reals
  // array as follows:
  //    reals = {<reserved>, 10.0.0.1, 10.0.0.2, 10.0.0.3}
  //    server_id_map = {{101=>1}, {102=>2}, {103=>3}}
  // server_id 101 still resolves to 10.0.0.1
  //
  // Example 2:
  //    Say, realNums_ is {0, 1, 2}
  // Further, suppose there is 1 vip with 3 real servers with weights {2, 2, 3}.
  // CH_ring size is 2 + 2 + 3 = 7.
  // After CH_ring population, we get CH_ring (without shuffling) and reals
  // array as below
  //    CH_ring = {0, 0, 1, 1, 2, 2, 2}
  //    reals = {10.0.0.1, 10.0.0.2, 10.0.0.3}
  // Thus, from CH_ring index 0, which resolves to real-id 0, we get 10.0.0.1
  // Now we change realNums_ to {1, 2, 3}
  // After CH_ring population, we get CH_ring and reals array as below
  //    CH_ring = {1, 1, 2, 2, 3, 3, 3}
  //    reals = {<reserved>, 10.0.0.1, 10.0.0.2, 10.0.0.3}
  // CH_ring at index 0 it resolves to real-id 1, which still resolves
  // to 10.0.0.1
  //
  // Why avoid real-id 0?
  // BPF arrays are initialized with value of 0. So it's hard to disambiguate
  // issues where '0' is returned as server at index 0 vs error cases where it
  // couldn't find the server. So we preserve 0 as the invalid entry to reals
  // array.
  for (uint32_t i = 1; i < config_.maxReals; i++) {
    realNums_.push_back(i);
  }

  if (!config_.rootMapPath.empty()) {
    standalone_ = false;
  }

  if (config_.hcInterface.empty()) {
    config_.hcInterface = config_.mainInterface;
  }

  if (!config_.testing) {
    ctl_value ctl;
    uint32_t res;

    // populating ctl vector
    if (config_.defaultMac.size() != 6) {
      throw std::invalid_argument("mac's size is not equal to six byte");
    }
    for (int i = 0; i < 6; i++) {
      ctl.mac[i] = config_.defaultMac[i];
    }
    ctlValues_[kMacAddrPos] = ctl;

    if (config_.enableHc) {
      res = config_.hcInterfaceIndex;
      if (res == 0) {
        res = bpfAdapter_->getInterfaceIndex(config_.hcInterface);
        if (res == 0) {
          throw std::invalid_argument(fmt::format(
              "can't resolve ifindex for healthcheck interface {}, error: {}",
              config_.hcInterface,
              folly::errnoStr(errno)));
        }
      }
      ctl.ifindex = res;
      ctlValues_[kHcIntfPos] = ctl;
      if (config_.tunnelBasedHCEncap) {
        res = bpfAdapter_->getInterfaceIndex(config_.v4TunInterface);
        if (!res) {
          throw std::invalid_argument(fmt::format(
              "can't resolve ifindex for v4tunel intf, error: {}",
              folly::errnoStr(errno)));
        }
        ctl.ifindex = res;
        ctlValues_[kIpv4TunPos] = ctl;

        res = bpfAdapter_->getInterfaceIndex(config_.v6TunInterface);
        if (!res) {
          throw std::invalid_argument(fmt::format(
              "can't resolve ifindex for v6tunel intf, error: {}",
              folly::errnoStr(errno)));
        }
        ctl.ifindex = res;
        ctlValues_[kIpv6TunPos] = ctl;
      }
    }

    res = config_.mainInterfaceIndex;
    if (res == 0) {
      // attempt to resolve interface name to the interface index
      res = bpfAdapter_->getInterfaceIndex(config_.mainInterface);
      if (res == 0) {
        throw std::invalid_argument(fmt::format(
            "can't resolve ifindex for main intf {}, error: {}",
            config_.mainInterface,
            folly::errnoStr(errno)));
      }
    }
    ctl.ifindex = res;
    ctlValues_[kMainIntfPos] = ctl;
  }
}

VsoshlbLb::~VsoshlbLb() {
  if (!config_.testing && progsAttached_ && config_.cleanupOnShutdown) {
    int res;
    auto mainIfindex = ctlValues_[kMainIntfPos].ifindex;
    auto hcIfindex = ctlValues_[kHcIntfPos].ifindex;
    if (standalone_) {
      res = bpfAdapter_->detachXdpProg(mainIfindex, config_.xdpAttachFlags);
    } else {
      res = bpfAdapter_->bpfMapDeleteElement(rootMapFd_, &config_.rootMapPos);
    }
    if (res != 0) {
      LOG(ERROR) << fmt::format(
          "wasn't able to delete main bpf prog, error: {}",
          folly::errnoStr(errno));
    }
    if (config_.enableHc) {
      res = bpfAdapter_->deleteTcBpfFilter(
          getHealthcheckerProgFd(),
          hcIfindex,
          "vsoshlb-healthchecker",
          config_.priority,
          TC_EGRESS);
      if (res != 0) {
        LOG(ERROR) << fmt::format(
            "wasn't able to delete hc bpf prog, error: {}",
            folly::errnoStr(errno));
      }
    }
  }
}

AddressType VsoshlbLb::validateAddress(
    const std::string& addr,
    bool allowNetAddr) {
  if (!folly::IPAddress::validate(addr)) {
    if (allowNetAddr && (features_.srcRouting || config_.testing)) {
      auto ret = folly::IPAddress::tryCreateNetwork(addr);
      if (ret.hasValue()) {
        return AddressType::NETWORK;
      }
    }
    lbStats_.addrValidationFailed++;
    LOG(ERROR) << "Invalid address: " << addr;
    return AddressType::INVALID;
  }
  return AddressType::HOST;
}

void VsoshlbLb::initialSanityChecking(bool flowDebug, bool globalLru) {
  int res;

  std::vector<std::string> maps;

  maps.push_back(VsoshlbLbMaps::ctl_array);
  maps.push_back(VsoshlbLbMaps::vip_map);
  maps.push_back(VsoshlbLbMaps::ch_rings);
  maps.push_back(VsoshlbLbMaps::reals);
  maps.push_back(VsoshlbLbMaps::stats);
  maps.push_back(VsoshlbLbMaps::lru_mapping);
  maps.push_back(VsoshlbLbMaps::server_id_map);
  maps.push_back(VsoshlbLbMaps::lru_miss_stats);
  maps.push_back(VsoshlbLbMaps::vip_miss_stats);

  if (flowDebug) {
    maps.push_back(VsoshlbLbMaps::flow_debug_maps);
  }

  if (globalLru) {
    maps.push_back(VsoshlbLbMaps::global_lru_maps);
  }

  res = getVsoshlbProgFd();
  if (res < 0) {
    throw std::invalid_argument(fmt::format(
        "can't get fd for prog: {}, error: {}",
        kBalancerProgName,
        folly::errnoStr(errno)));
  }

  if (config_.enableHc) {
    res = getHealthcheckerProgFd();
    if (res < 0) {
      throw std::invalid_argument(fmt::format(
          "can't get fd for prog: {}, error: {}",
          kHealthcheckerProgName,
          folly::errnoStr(errno)));
    }
    maps.push_back(VsoshlbLbMaps::hc_ctrl_map);
    maps.push_back(VsoshlbLbMaps::hc_reals_map);
    maps.push_back(VsoshlbLbMaps::hc_stats_map);
  }

  // some sanity checking. we will check that all maps exists, so in later
  // code we wouldn't check if their fd != -1
  // names of the maps must be the same as in bpf code.
  for (auto& map : maps) {
    res = bpfAdapter_->getMapFdByName(map);
    if (res < 0) {
      VLOG(4) << "missing map: " << map;
      throw std::invalid_argument(
          fmt::format("map not found, error: {}", folly::errnoStr(errno)));
    }
  }
}

int VsoshlbLb::createLruMap(int size, int flags, int numaNode, int cpu) {
  return bpfAdapter_->createNamedBpfMap(
      VsoshlbLbMaps::vsoshlb_lru + std::to_string(cpu),
      kBpfMapTypeLruHash,
      sizeof(struct flow_key),
      sizeof(struct real_pos_lru),
      size,
      flags,
      numaNode);
}

void VsoshlbLb::initFlowDebugMapForCore(
    int core,
    int size,
    int flags,
    int numaNode) {
  int lru_fd;
  VLOG(3) << "Creating flow debug lru for core " << core;
  lru_fd = bpfAdapter_->createNamedBpfMap(
      VsoshlbLbMaps::flow_debug_lru,
      kBpfMapTypeLruHash,
      sizeof(struct flow_key),
      sizeof(struct flow_debug_info),
      size,
      flags,
      numaNode);
  if (lru_fd < 0) {
    LOG(ERROR) << "can't create lru for core: " << core;
    throw std::runtime_error(fmt::format(
        "can't create LRU for forwarding core, error: {}",
        folly::errnoStr(errno)));
  }
  VLOG(3) << "Created flow debug lru for core " << core;
  flowDebugMapsFd_[core] = lru_fd;
}

void VsoshlbLb::initFlowDebugPrototypeMap() {
  int flow_proto_fd, res;
  if (forwardingCores_.size() != 0) {
    flow_proto_fd = flowDebugMapsFd_[forwardingCores_[kFirstElem]];
  } else {
    VLOG(3) << "Creating generic flow debug lru";
    flow_proto_fd = bpfAdapter_->createNamedBpfMap(
        VsoshlbLbMaps::flow_debug_lru,
        kBpfMapTypeLruHash,
        sizeof(struct flow_key),
        sizeof(struct flow_debug_info),
        vsoshlb::kFallbackLruSize,
        kMapNoFlags,
        kNoNuma);
  }
  if (flow_proto_fd < 0) {
    throw std::runtime_error(fmt::format(
        "can't create LRU prototype, error: {}", folly::errnoStr(errno)));
  }
  res = bpfAdapter_->setInnerMapPrototype(
      VsoshlbLbMaps::flow_debug_maps, flow_proto_fd);
  if (res < 0) {
    throw std::runtime_error(fmt::format(
        "can't update inner_maps_fds w/ prototype for main lru, error: {}",
        folly::errnoStr(errno)));
  }
  VLOG(3) << "Created flow map proto";
}

void VsoshlbLb::attachFlowDebugLru(int core) {
  int map_fd, res, key;
  key = core;
  map_fd = flowDebugMapsFd_[core];
  if (map_fd < 0) {
    throw std::runtime_error(
        fmt::format("Invalid FD found for core {}: {}", core, map_fd));
  }
  res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::flow_debug_maps),
      &key,
      &map_fd);
  if (res < 0) {
    throw std::runtime_error(fmt::format(
        "can't attach lru to forwarding core, error: {}",
        folly::errnoStr(errno)));
  }
  VLOG(3) << "Set cpu core " << core << "flow map id to " << map_fd;
}

void VsoshlbLb::initGlobalLruMapForCore(
    int core,
    int size,
    int flags,
    int numaNode) {
  VLOG(0) << __func__;
  int lru_fd;
  VLOG(3) << "Creating global lru for core " << core;
  lru_fd = bpfAdapter_->createNamedBpfMap(
      VsoshlbLbMaps::global_lru,
      kBpfMapTypeLruHash,
      sizeof(struct flow_key),
      sizeof(uint32_t),
      size,
      flags,
      numaNode);
  if (lru_fd < 0) {
    LOG(ERROR) << "can't create global lru for core: " << core;
    throw std::runtime_error(fmt::format(
        "can't create global LRU for forwarding core, error: {}",
        folly::errnoStr(errno)));
  }
  VLOG(3) << "Created global lru for core " << core;
  globalLruMapsFd_[core] = lru_fd;
}

void VsoshlbLb::initGlobalLruPrototypeMap() {
  VLOG(0) << __func__;
  int proto_fd;
  if (forwardingCores_.size() != 0) {
    proto_fd = globalLruMapsFd_[forwardingCores_[kFirstElem]];
  } else {
    VLOG(3) << "Creating generic flow debug lru";
    proto_fd = bpfAdapter_->createNamedBpfMap(
        VsoshlbLbMaps::global_lru,
        kBpfMapTypeLruHash,
        sizeof(struct flow_key),
        sizeof(uint32_t),
        vsoshlb::kFallbackLruSize,
        kMapNoFlags,
        kNoNuma);
  }
  if (proto_fd < 0) {
    throw std::runtime_error(fmt::format(
        "can't create global LRU prototype, error: {}",
        folly::errnoStr(errno)));
  }
  int res = bpfAdapter_->setInnerMapPrototype(
      VsoshlbLbMaps::global_lru_maps, proto_fd);
  if (res < 0) {
    throw std::runtime_error(fmt::format(
        "can't update inner_maps_fds w/ prototype for global lru, error: {}",
        folly::errnoStr(errno)));
  }
  VLOG(1) << "Created global_lru map proto";
}

void VsoshlbLb::attachGlobalLru(int core) {
  VLOG(0) << __func__;
  int key = core;
  int map_fd = globalLruMapsFd_[core];
  if (map_fd < 0) {
    throw std::runtime_error(
        fmt::format("Invalid FD found for core {}: {}", core, map_fd));
  }
  int res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::global_lru_maps),
      &key,
      &map_fd);
  if (res < 0) {
    throw std::runtime_error(fmt::format(
        "can't attach global lru to forwarding core, error: {}",
        folly::errnoStr(errno)));
  }
  VLOG(1) << "Set cpu core " << core << "global_lru map id to " << map_fd;
}

void VsoshlbLb::initLrus(bool flowDebug, bool globalLru) {
  bool forwarding_cores_specified{false};
  bool numa_mapping_specified{false};

  if (forwardingCores_.size() != 0) {
    if (numaNodes_.size() != 0) {
      if (numaNodes_.size() != forwardingCores_.size()) {
        throw std::runtime_error(
            "numaNodes size mut be equal to forwardingCores");
      }
      numa_mapping_specified = true;
    }
    auto per_core_lru_size = config_.LruSize / forwardingCores_.size();
    VLOG(2) << "per core lru size: " << per_core_lru_size;
    for (int i = 0; i < forwardingCores_.size(); i++) {
      auto core = forwardingCores_[i];
      if ((core > kMaxForwardingCores) || core < 0) {
        LOG(FATAL) << "got core# " << core
                   << " which is not in supported range: [ 0: "
                   << kMaxForwardingCores << " ]";
        throw std::runtime_error("unsuported number of forwarding cores");
      }
      int numa_node = kNoNuma;
      int lru_map_flags = 0;
      if (numa_mapping_specified) {
        numa_node = numaNodes_[i];
        lru_map_flags |= BPF_F_NUMA_NODE;
      }
      int lru_fd =
          createLruMap(per_core_lru_size, lru_map_flags, numa_node, core);
      if (lru_fd < 0) {
        LOG(FATAL) << "can't creat lru for core: " << core;
        throw std::runtime_error(fmt::format(
            "can't create LRU for forwarding core, error: {}",
            folly::errnoStr(errno)));
      }
      lruMapsFd_[core] = lru_fd;
      if (flowDebug) {
        initFlowDebugMapForCore(
            core, per_core_lru_size, lru_map_flags, numa_node);
      }
      if (globalLru) {
        initGlobalLruMapForCore(
            core, config_.globalLruSize, lru_map_flags, numa_node);
      }
    }
    forwarding_cores_specified = true;
  }

  int lru_proto_fd;
  if (forwarding_cores_specified) {
    // creating prototype for main LRU's map in map
    // as we know that forwardingCores_ at least has one element
    // we are going to use the first one as a key to find fd of
    // already created LRU
    lru_proto_fd = lruMapsFd_[forwardingCores_[kFirstElem]];
  } else {
    // creating prototype for LRU's map-in-map. this code path would be hit
    // only during unit tests, where we dont specify forwarding cores
    lru_proto_fd = createLruMap();

    if (lru_proto_fd < 0) {
      throw std::runtime_error("can't create prototype map for test lru");
    }
  }
  int res = bpfAdapter_->setInnerMapPrototype(
      VsoshlbLbMaps::lru_mapping, lru_proto_fd);
  if (res < 0) {
    throw std::runtime_error(fmt::format(
        "can't update inner_maps_fds w/ prototype for main lru, error: {}",
        folly::errnoStr(errno)));
  }
}

void VsoshlbLb::attachLrus(bool flowDebug, bool globalLru) {
  if (!progsLoaded_) {
    throw std::runtime_error("can't attach lru when bpf progs are not loaded");
  }
  int map_fd, res, key;
  for (const auto& core : forwardingCores_) {
    key = core;
    map_fd = lruMapsFd_[core];
    res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::lru_mapping), &key, &map_fd);
    if (res < 0) {
      throw std::runtime_error(fmt::format(
          "can't attach lru to forwarding core, error: {}",
          folly::errnoStr(errno)));
    }
    if (flowDebug) {
      attachFlowDebugLru(core);
    }
    if (globalLru) {
      attachGlobalLru(core);
    }
  }

  if (globalLru) {
    globalLruFallbackFd_ =
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::fallback_glru);
  }
}

void VsoshlbLb::setupGueEnvironment() {
  if (config_.vsoshlbSrcV4.empty() && config_.vsoshlbSrcV6.empty()) {
    throw std::runtime_error(
        "No source address provided to use as source GUE encapsulation");
  }
  if (!config_.vsoshlbSrcV4.empty()) {
    auto srcv4 =
        IpHelpers::parseAddrToBe(folly::IPAddress(config_.vsoshlbSrcV4));
    uint32_t key = kSrcV4Pos;
    auto res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::pckt_srcs), &key, &srcv4);
    if (res < 0) {
      throw std::runtime_error("can not update src v4 address for GUE packet");
    } else {
      VLOG(1) << "update src v4 address " << config_.vsoshlbSrcV4
              << " for GUE packet";
    }
  } else {
    LOG(ERROR) << "Empty IPV4 address provided to use as source in GUE encap";
  }
  if (!config_.vsoshlbSrcV6.empty()) {
    auto srcv6 =
        IpHelpers::parseAddrToBe(folly::IPAddress(config_.vsoshlbSrcV6));
    auto key = kSrcV6Pos;
    auto res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::pckt_srcs), &key, &srcv6);
    if (res < 0) {
      throw std::runtime_error("can not update src v6 address for GUE packet");
    } else {
      VLOG(1) << "update src v6 address " << config_.vsoshlbSrcV6
              << " for GUE packet";
    }
  } else {
    LOG(ERROR) << "Empty IPV6 address provided to use as source in GUE encap";
  }
}

void VsoshlbLb::setupHcEnvironment() {
  auto map_fd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_pckt_srcs_map);
  if (config_.vsoshlbSrcV4.empty() && config_.vsoshlbSrcV6.empty()) {
    throw std::runtime_error(
        "No source address provided for direct healthchecking");
  }
  if (!config_.vsoshlbSrcV4.empty()) {
    auto srcv4 =
        IpHelpers::parseAddrToBe(folly::IPAddress(config_.vsoshlbSrcV4));
    uint32_t key = kSrcV4Pos;
    auto res = bpfAdapter_->bpfUpdateMap(map_fd, &key, &srcv4);
    if (res < 0) {
      throw std::runtime_error(
          "can not update src v4 address for direct healthchecking");
    } else {
      VLOG(1) << "update src v4 address " << config_.vsoshlbSrcV4
              << " for direct healthchecking";
    }
  } else {
    LOG(ERROR) << "Empty IPV4 address provided to use as source in healthcheck";
  }
  if (!config_.vsoshlbSrcV6.empty()) {
    auto srcv6 =
        IpHelpers::parseAddrToBe(folly::IPAddress(config_.vsoshlbSrcV6));
    auto key = kSrcV6Pos;
    auto res = bpfAdapter_->bpfUpdateMap(map_fd, &key, &srcv6);
    if (res < 0) {
      throw std::runtime_error(
          "can not update src v6 address for direct healthchecking");
    } else {
      VLOG(1) << "update src v6 address " << config_.vsoshlbSrcV6
              << " for direct healthchecking";
    }
  } else {
    LOG(ERROR) << "Empty IPV6 address provided to use as source in healthcheck";
  }

  std::array<struct hc_mac, 2> macs;
  // populating mac addresses for healthchecking
  if (config_.localMac.size() != 6) {
    throw std::invalid_argument("src mac's size is not equal to six byte");
  }
  for (int i = 0; i < 6; i++) {
    macs[kHcSrcMacPos].mac[i] = config_.localMac[i];
    macs[kHcDstMacPos].mac[i] = config_.defaultMac[i];
  }
  for (auto position : {kHcSrcMacPos, kHcDstMacPos}) {
    auto res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_pckt_macs),
        &position,
        &macs[position]);
    if (res < 0) {
      throw std::runtime_error("can not update healthchecks mac address");
    }
  }
}

bool VsoshlbLb::addSrcIpForPcktEncap(const folly::IPAddress& src) {
  auto srcBe = IpHelpers::parseAddrToBe(src);
  uint32_t key = src.isV4() ? kSrcV4Pos : kSrcV6Pos;
  // update map for hc_pckt_src
  auto res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_pckt_srcs_map),
      &key,
      &srcBe);
  if (res) {
    LOG(ERROR) << "cannot insert src address in map: hc_pckt_srcs_map";
    return false;
  }
  VLOG(3) << "Successfully updated hc_pckt_srcs_map with ip: " << src.str();

  res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::pckt_srcs), &key, &srcBe);
  if (res) {
    LOG(ERROR) << "cannot insert src address in map: pckt_srcs";
    return false;
  }
  VLOG(3) << "Successfully updated pckt_srcs with ip: " << src.str();

  return true;
}

void VsoshlbLb::enableRecirculation() {
  uint32_t key = kRecirculationIndex;
  int balancer_fd = getVsoshlbProgFd();
  auto res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName("subprograms"), &key, &balancer_fd);
  if (res < 0) {
    throw std::runtime_error("can not update subprograms for recirculation");
  }
}

void VsoshlbLb::featureDiscovering() {
  if (bpfAdapter_->isMapInProg(
          kBalancerProgName.toString(), VsoshlbLbMaps::lpm_src_v4)) {
    VLOG(2) << "source based routing is supported";
    features_.srcRouting = true;
  } else {
    features_.srcRouting = false;
  }
  if (bpfAdapter_->isMapInProg(
          kBalancerProgName.toString(), VsoshlbLbMaps::decap_dst)) {
    VLOG(2) << "inline decapsulation is supported";
    features_.inlineDecap = true;
  } else {
    features_.inlineDecap = false;
  }
  if (bpfAdapter_->isMapInProg(
          kBalancerProgName.toString(), VsoshlbLbMaps::event_pipe)) {
    VLOG(2) << "vsoshlb introspection is enabled";
    features_.introspection = true;
  } else {
    features_.introspection = false;
  }
  if (bpfAdapter_->isMapInProg(
          kBalancerProgName.toString(), VsoshlbLbMaps::pckt_srcs)) {
    VLOG(2) << "GUE encapsulation is enabled";
    features_.gueEncap = true;
  } else {
    features_.gueEncap = false;
  }
  if (bpfAdapter_->isMapInProg(
          kHealthcheckerProgName.toString(), VsoshlbLbMaps::hc_pckt_srcs_map)) {
    VLOG(2) << "Direct healthchecking is enabled";
    features_.directHealthchecking = true;
  } else {
    features_.directHealthchecking = false;
  }

  if (bpfAdapter_->isMapInProg(
          kBalancerProgName.toString(), VsoshlbLbMaps::flow_debug_maps)) {
    VLOG(2) << "Flow debug is enabled";
    features_.flowDebug = true;
  } else {
    features_.flowDebug = false;
  }
}

void VsoshlbLb::startIntrospectionRoutines() {
  auto monitor_config = config_.monitorConfig;
  monitor_config.nCpus = vsoshlb::BpfAdapter::getPossibleCpus();
  monitor_config.mapFd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::event_pipe);
  monitor_ = std::make_shared<VsoshlbMonitor>(monitor_config);
}

void VsoshlbLb::loadBpfProgs() {
  int res;
  bool flowDebugInProg = false;
  bool globalLruInProg = false;

  flowDebugInProg = bpfAdapter_->isMapInBpfObject(
      config_.balancerProgPath, VsoshlbLbMaps::flow_debug_maps);
  globalLruInProg = bpfAdapter_->isMapInBpfObject(
      config_.balancerProgPath, VsoshlbLbMaps::global_lru_maps);
  initLrus(flowDebugInProg, globalLruInProg);
  if (flowDebugInProg) {
    initFlowDebugPrototypeMap();
  }
  if (globalLruInProg) {
    initGlobalLruPrototypeMap();
  }
  res = bpfAdapter_->loadBpfProg(config_.balancerProgPath);
  if (res) {
    throw std::invalid_argument("can't load main bpf program");
  }

  if (config_.enableHc) {
    res = bpfAdapter_->loadBpfProg(config_.healthcheckingProgPath);
    if (res) {
      throw std::invalid_argument(fmt::format(
          "can't load healthchecking bpf program, error: {}",
          folly::errnoStr(errno)));
    }
  }

  initialSanityChecking(flowDebugInProg, globalLruInProg);
  featureDiscovering();

  if (features_.gueEncap) {
    setupGueEnvironment();
  }

  if (features_.inlineDecap) {
    enableRecirculation();
  }

  // add values to main prog ctl_array
  std::vector<uint32_t> balancer_ctl_keys = {kMacAddrPos};

  for (auto ctl_key : balancer_ctl_keys) {
    res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::ctl_array),
        &ctl_key,
        &ctlValues_[ctl_key]);

    if (res != 0) {
      throw std::invalid_argument(fmt::format(
          "can't update ctl array for main program, error: {}",
          folly::errnoStr(errno)));
    }
  }

  if (config_.enableHc) {
    std::vector<uint32_t> hc_ctl_keys = {kMainIntfPos};
    if (config_.tunnelBasedHCEncap) {
      hc_ctl_keys.push_back(kIpv4TunPos);
      hc_ctl_keys.push_back(kIpv6TunPos);
    }
    for (auto ctl_key : hc_ctl_keys) {
      res = bpfAdapter_->bpfUpdateMap(
          bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_ctrl_map),
          &ctl_key,
          &ctlValues_[ctl_key].ifindex);

      if (res != 0) {
        throw std::invalid_argument(fmt::format(
            "can't update ctrl map for hc program, error: {}",
            folly::errnoStr(errno)));
      }
    }
    if (features_.directHealthchecking) {
      setupHcEnvironment();
    }
  }
  progsLoaded_ = true;
  if (features_.introspection) {
    startIntrospectionRoutines();
    introspectionStarted_ = true;
  }

  attachLrus(flowDebugInProg, globalLruInProg);

  vip_definition vip_def;
  memset(&vip_def, 0, sizeof(vip_definition));
  uint32_t key = 0;
  res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::vip_miss_stats),
      &key,
      &vip_def);
  if (res) {
    LOG(ERROR) << "can't update lru miss stat vip, error: "
               << folly::errnoStr(errno);
  }
}

bool VsoshlbLb::reloadBalancerProg(
    const std::string& path,
    std::optional<VsoshlbConfig> config) {
  int res;
  res = bpfAdapter_->reloadBpfProg(path);
  if (res) {
    return false;
  }

  if (config.has_value()) {
  }

  config_.balancerProgPath = path;

  bool flowDebugInProg =
      bpfAdapter_->isMapInBpfObject(path, VsoshlbLbMaps::flow_debug_maps);
  bool globalLruInProg =
      bpfAdapter_->isMapInBpfObject(path, VsoshlbLbMaps::global_lru_maps);
  initialSanityChecking(flowDebugInProg, globalLruInProg);
  featureDiscovering();

  if (features_.gueEncap) {
    setupGueEnvironment();
  }

  if (features_.inlineDecap) {
    enableRecirculation();
  }

  if (features_.introspection && !introspectionStarted_) {
    startIntrospectionRoutines();
    introspectionStarted_ = true;
  }
  progsReloaded_ = true;
  return true;
}

void VsoshlbLb::attachBpfProgs() {
  if (!progsLoaded_) {
    throw std::invalid_argument("failed to attach bpf prog: prog not loaded");
  }
  int res;
  auto main_fd = bpfAdapter_->getProgFdByName(kBalancerProgName.toString());
  auto interface_index = ctlValues_[kMainIntfPos].ifindex;
  if (standalone_) {
    // attaching main bpf prog in standalone mode
    res = bpfAdapter_->modifyXdpProg(
        main_fd, interface_index, config_.xdpAttachFlags);
    if (res != 0) {
      throw std::invalid_argument(fmt::format(
          "can't attach main bpf prog "
          "to main inteface, error: {}",
          folly::errnoStr(errno)));
    }
  } else if (config_.useRootMap) {
    // we are in "shared" mode and must register ourself in root xdp prog
    rootMapFd_ = bpfAdapter_->getPinnedBpfObject(config_.rootMapPath);
    if (rootMapFd_ < 0) {
      throw std::invalid_argument(fmt::format(
          "can't get fd of xdp's root map, error: {}", folly::errnoStr(errno)));
    }
    res = bpfAdapter_->bpfUpdateMap(rootMapFd_, &config_.rootMapPos, &main_fd);
    if (res) {
      throw std::invalid_argument(fmt::format(
          "can't register in root array, error: {}", folly::errnoStr(errno)));
    }
  }

  if (config_.enableHc && !progsReloaded_) {
    // attaching healthchecking bpf prog.
    auto hc_fd = getHealthcheckerProgFd();
    res = bpfAdapter_->addTcBpfFilter(
        hc_fd,
        ctlValues_[kHcIntfPos].ifindex,
        "vsoshlb-healthchecker",
        config_.priority,
        TC_EGRESS);
    if (res != 0) {
      if (standalone_) {
        // will try to remove main bpf prog.
        bpfAdapter_->detachXdpProg(interface_index, config_.xdpAttachFlags);
      } else {
        bpfAdapter_->bpfMapDeleteElement(rootMapFd_, &config_.rootMapPos);
      }
      throw std::invalid_argument(fmt::format(
          "can't attach healthchecking bpf prog "
          "to given inteface: {}, error: {}",
          config_.hcInterface,
          folly::errnoStr(errno)));
    }
  }
  progsAttached_ = true;
}

bool VsoshlbLb::changeMac(const std::vector<uint8_t> newMac) {
  uint32_t key = kMacAddrPos;

  VLOG(4) << "adding new mac address";

  if (newMac.size() != kMacBytes) {
    return false;
  }
  for (int i = 0; i < kMacBytes; i++) {
    ctlValues_[kMacAddrPos].mac[i] = newMac[i];
  }
  if (!config_.testing) {
    auto res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::ctl_array),
        &key,
        &ctlValues_[kMacAddrPos].mac);
    if (res != 0) {
      lbStats_.bpfFailedCalls++;
      VLOG(4) << "can't add new mac address";
      return false;
    }

    if (features_.directHealthchecking) {
      key = kHcDstMacPos;
      auto res_2 = bpfAdapter_->bpfUpdateMap(
          bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_pckt_macs),
          &key,
          &ctlValues_[kMacAddrPos].mac);
      if (res_2 != 0) {
        lbStats_.bpfFailedCalls++;
        VLOG(4) << "can't add new mac address for direct healthchecks";
        return false;
      }
    }
  }
  return true;
}

std::vector<uint8_t> VsoshlbLb::getMac() {
  return std::vector<uint8_t>(
      std::begin(ctlValues_[kMacAddrPos].mac),
      std::end(ctlValues_[kMacAddrPos].mac));
}

std::map<int, uint32_t> VsoshlbLb::getIndexOfNetworkInterfaces() {
  std::map<int, uint32_t> res;
  res[kMainIntfPos] = ctlValues_[kMainIntfPos].ifindex;
  if (config_.enableHc) {
    res[kHcIntfPos] = ctlValues_[kHcIntfPos].ifindex;
    if (config_.tunnelBasedHCEncap) {
      res[kIpv4TunPos] = ctlValues_[kIpv4TunPos].ifindex;
      res[kIpv6TunPos] = ctlValues_[kIpv6TunPos].ifindex;
    }
  }
  return res;
}

bool VsoshlbLb::addVip(const VipKey& vip, const uint32_t flags) {
  if (validateAddress(vip.address) == AddressType::INVALID) {
    LOG(ERROR) << "Invalid Vip address: " << vip.address;
    return false;
  }
  VLOG(1) << fmt::format(
      "adding new vip: {}:{}:{}", vip.address, vip.port, vip.proto);

  if (vipNums_.size() == 0) {
    LOG(ERROR) << "exhausted vip's space";
    return false;
  }
  if (vips_.find(vip) != vips_.end()) {
    LOG(ERROR) << "trying to add already existing vip";
    return false;
  }
  auto vip_num = vipNums_[0];
  vipNums_.pop_front();
  vips_.emplace(
      vip, Vip(vip_num, flags, config_.chRingSize, config_.hashFunction));
  if (!config_.testing) {
    vip_meta meta;
    meta.vip_num = vip_num;
    meta.flags = flags;
    updateVipMap(ModifyAction::ADD, vip, &meta);
  }
  return true;
}

bool VsoshlbLb::addHcKey(const VipKey& hcKey) {
  if (!config_.enableHc) {
    LOG(ERROR) << "Ignoring addHcKey call on non-healthchecking instance";
    return false;
  }

  if (hcKeyNums_.size() == 0) {
    LOG(ERROR) << "exhausted hc key's space";
    return false;
  }
  if (hckeys_.find(hcKey) != hckeys_.end()) {
    LOG(ERROR) << "trying to add already existing hc key";
    return false;
  }
  auto hc_key_num = hcKeyNums_[0];
  hcKeyNums_.pop_front();
  hckeys_.emplace(hcKey, hc_key_num);
  if (!config_.testing) {
    return updateHcKeyMap(ModifyAction::ADD, hcKey, hc_key_num);
  }
  return true;
}

bool VsoshlbLb::changeHashFunctionForVip(const VipKey& vip, HashFunction func) {
  if (validateAddress(vip.address) == AddressType::INVALID) {
    LOG(ERROR) << "Invalid Vip address: " << vip.address;
    return false;
  }
  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    LOG(ERROR) << "trying to change non existing vip";
    return false;
  }
  vip_iter->second.setHashFunction(func);
  auto positions = vip_iter->second.recalculateHashRing();
  programHashRing(positions, vip_iter->second.getVipNum());
  return true;
}

bool VsoshlbLb::delVip(const VipKey& vip) {
  VLOG(1) << fmt::format(
      "deleting vip: {}:{}:{}", vip.address, vip.port, vip.proto);

  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    LOG(ERROR) << "trying to delete non-existing vip";
    return false;
  }

  auto vip_reals = vip_iter->second.getReals();
  // decreasing ref count for reals. delete em if it became 0
  for (auto& vip_real : vip_reals) {
    auto real_name = numToReals_[vip_real];
    decreaseRefCountForReal(real_name);
  }
  vipNums_.push_back(vip_iter->second.getVipNum());
  if (!config_.testing) {
    updateVipMap(ModifyAction::DEL, vip);
  }
  vips_.erase(vip_iter);
  return true;
}

bool VsoshlbLb::delHcKey(const VipKey& hcKey) {
  if (!config_.enableHc) {
    LOG(ERROR) << "Ignoring delHcKey call on non-healthchecking instance";
    return false;
  }

  VLOG(1) << fmt::format(
      "deleting hc_key: {}:{}:{}", hcKey.address, hcKey.port, hcKey.proto);

  auto hc_key_iter = hckeys_.find(hcKey);
  if (hc_key_iter == hckeys_.end()) {
    LOG(ERROR) << "trying to delete non-existing hc_key";
    return false;
  }
  hcKeyNums_.push_back(hc_key_iter->second);
  hckeys_.erase(hcKey);

  if (!config_.testing) {
    return updateHcKeyMap(ModifyAction::DEL, hcKey);
  }
  return true;
}

std::vector<VipKey> VsoshlbLb::getAllVips() {
  std::vector<VipKey> vips(vips_.size());
  int i = 0;
  for (auto& vip : vips_) {
    vips[i++] = vip.first;
  }
  return vips;
}

uint32_t VsoshlbLb::getVipFlags(const VipKey& vip) {
  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    throw std::invalid_argument(fmt::format(
        "trying to get flags from non-existing vip: {}", vip.address));
  }
  return vip_iter->second.getVipFlags();
}

bool VsoshlbLb::modifyVip(const VipKey& vip, uint32_t flag, bool set) {
  VLOG(1) << fmt::format(
      "modifying vip: {}:{}:{}", vip.address, vip.port, vip.proto);

  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    LOG(ERROR) << fmt::format(
        "trying to modify non-existing vip: {}", vip.address);
    return false;
  }
  if (set) {
    vip_iter->second.setVipFlags(flag);
  } else {
    vip_iter->second.unsetVipFlags(flag);
  }
  if (!config_.testing) {
    vip_meta meta;
    meta.vip_num = vip_iter->second.getVipNum();
    meta.flags = vip_iter->second.getVipFlags();
    return updateVipMap(ModifyAction::ADD, vip, &meta);
  }
  return true;
}

bool VsoshlbLb::addRealForVip(const NewReal& real, const VipKey& vip) {
  std::vector<NewReal> reals;
  reals.push_back(real);
  return modifyRealsForVip(ModifyAction::ADD, reals, vip);
}

bool VsoshlbLb::delRealForVip(const NewReal& real, const VipKey& vip) {
  std::vector<NewReal> reals;
  reals.push_back(real);
  return modifyRealsForVip(ModifyAction::DEL, reals, vip);
}

bool VsoshlbLb::modifyReal(const std::string& real, uint8_t flags, bool set) {
  if (validateAddress(real) == AddressType::INVALID) {
    LOG(ERROR) << "invalid real's address: " << real;
    return false;
  }

  VLOG(4) << fmt::format("modifying real: {} ", real);
  folly::IPAddress raddr(real);
  auto real_iter = reals_.find(raddr);
  if (real_iter == reals_.end()) {
    LOG(ERROR) << fmt::format("trying to modify non-existing real: {}", real);
    return false;
  }
  flags &= ~V6DADDR; // to keep IPv4/IPv6 specific flag
  if (set) {
    real_iter->second.flags |= flags;
  } else {
    real_iter->second.flags &= ~flags;
  }
  reals_[raddr].flags = real_iter->second.flags;
  if (!config_.testing) {
    updateRealsMap(raddr, real_iter->second.num, real_iter->second.flags);
  }
  return true;
}

bool VsoshlbLb::modifyRealsForVip(
    const ModifyAction action,
    const std::vector<NewReal>& reals,
    const VipKey& vip) {
  UpdateReal ureal;
  std::vector<UpdateReal> ureals;
  ureal.action = action;

  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    LOG(ERROR) << fmt::format(
        "trying to modify reals for non existing vip: {}", vip.address);
    return false;
  }
  auto cur_reals = vip_iter->second.getReals();
  for (const auto& real : reals) {
    if (validateAddress(real.address) == AddressType::INVALID) {
      LOG(ERROR) << "Invalid real's address: " << real.address;
      continue;
    }
    folly::IPAddress raddr(real.address);
    VLOG(4) << fmt::format(
        "modifying real: {} with weight {} for vip {}:{}:{}. action is {}",
        real.address,
        real.weight,
        vip.address,
        vip.port,
        vip.proto,
        (int)action);

    if (action == ModifyAction::DEL) {
      auto real_iter = reals_.find(raddr);
      if (real_iter == reals_.end()) {
        LOG(ERROR) << "trying to delete non-existing real";
        continue;
      }
      if (std::find(
              cur_reals.begin(), cur_reals.end(), real_iter->second.num) ==
          cur_reals.end()) {
        // this real doesn't belong to this vip
        LOG(ERROR) << fmt::format(
            "trying to delete non-existing real for the VIP: {}", vip.address);
        continue;
      }
      ureal.updatedReal.num = real_iter->second.num;
      decreaseRefCountForReal(raddr);
    } else {
      auto real_iter = reals_.find(raddr);
      if (real_iter != reals_.end()) {
        if (std::find(
                cur_reals.begin(), cur_reals.end(), real_iter->second.num) ==
            cur_reals.end()) {
          // increment ref count if it's a new real for this vip
          increaseRefCountForReal(raddr, real.flags);
          cur_reals.push_back(real_iter->second.num);
        }
        ureal.updatedReal.num = real_iter->second.num;
      } else {
        auto rnum = increaseRefCountForReal(raddr, real.flags);
        if (rnum == config_.maxReals) {
          LOG(ERROR) << "exhausted real's space";
          continue;
        }
        ureal.updatedReal.num = rnum;
      }
      ureal.updatedReal.weight = real.weight;
      ureal.updatedReal.hash = raddr.hash();
    }
    ureals.push_back(ureal);
  }

  auto ch_positions = vip_iter->second.batchRealsUpdate(ureals);
  auto vip_num = vip_iter->second.getVipNum();
  programHashRing(ch_positions, vip_num);
  return true;
}

void VsoshlbLb::programHashRing(
    const std::vector<RealPos>& chPositions,
    const uint32_t vipNum) {
  if (chPositions.empty()) {
    return;
  }

  if (!config_.testing) {
    uint32_t updateSize = chPositions.size();
    uint32_t keys[updateSize];
    uint32_t values[updateSize];

    auto ch_fd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::ch_rings);
    for (uint32_t i = 0; i < updateSize; i++) {
      values[i] = chPositions[i].real;
    }

    auto res = bpfAdapter_->bpfUpdateMapBatch(ch_fd, keys, values, updateSize);
    if (res != 0) {
      lbStats_.bpfFailedCalls++;
      LOG(ERROR) << "can't update ch ring"
                 << ", error: " << folly::errnoStr(errno);
    }
  }
}

std::vector<NewReal> VsoshlbLb::getRealsForVip(const VipKey& vip) {
  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    throw std::invalid_argument(fmt::format(
        "trying to get real from non-existing vip: {}", vip.address));
  }
  auto vip_reals_ids = vip_iter->second.getRealsAndWeight();
  std::vector<NewReal> reals(vip_reals_ids.size());
  int i = 0;
  for (auto real_id : vip_reals_ids) {
    reals[i].weight = real_id.weight;
    reals[i].address = numToReals_[real_id.num].str();
    reals[i].flags = reals_[numToReals_[real_id.num]].flags;
    ++i;
  }
  return reals;
}

int64_t VsoshlbLb::getIndexForReal(const std::string& real) {
  if (validateAddress(real) != AddressType::INVALID) {
    folly::IPAddress raddr(real);
    auto real_iter = reals_.find(raddr);
    if (real_iter != reals_.end()) {
      return real_iter->second.num;
    }
  }
  return kError;
}

int VsoshlbLb::addSrcRoutingRule(
    const std::vector<std::string>& srcs,
    const std::string& dst) {
  int num_errors = 0;
  if (!features_.srcRouting && !config_.testing) {
    LOG(ERROR) << "Source based routing is not enabled in forwarding plane";
    return kError;
  }
  if (validateAddress(dst) == AddressType::INVALID) {
    LOG(ERROR) << "Invalid dst address for src routing: " << dst;
    return kError;
  }
  std::vector<folly::CIDRNetwork> src_networks;
  for (auto& src : srcs) {
    if (validateAddress(src, true) != AddressType::NETWORK) {
      LOG(ERROR) << "trying to add incorrect addr for src routing " << src;
      num_errors++;
      // dont want to stop even if one addr is incorrect.
      continue;
    }
    if (lpmSrcMapping_.size() + src_networks.size() + 1 >
        config_.maxLpmSrcSize) {
      LOG(ERROR) << "source mappings map size is exhausted";
      // num errors is equal to number of routes which don't have space to be
      // installed to
      num_errors += (srcs.size() - src_networks.size());
      // no point to continue. bailing out
      break;
    }
    // we already validated above that this is network address so wont throw
    src_networks.push_back(folly::IPAddress::createNetwork(src));
  }
  auto rval = addSrcRoutingRule(src_networks, dst);
  if (rval == kError) {
    num_errors = rval;
  }
  return num_errors;
}

int VsoshlbLb::addSrcRoutingRule(
    const std::vector<folly::CIDRNetwork>& srcs,
    const std::string& dst) {
  if (!features_.srcRouting && !config_.testing) {
    LOG(ERROR) << "Source based routing is not enabled in forwarding plane";
    return kError;
  }
  if (validateAddress(dst) == AddressType::INVALID) {
    LOG(ERROR) << "Invalid dst address for src routing: " << dst;
    return kError;
  }
  for (auto& src : srcs) {
    if (lpmSrcMapping_.size() + 1 > config_.maxLpmSrcSize) {
      LOG(ERROR) << "source mappings map size is exhausted";
      // no point to continue. bailing out
      return kError;
    }
    auto rnum = increaseRefCountForReal(folly::IPAddress(dst));
    if (rnum == config_.maxReals) {
      LOG(ERROR) << "exhausted real's space";
      // all src using same dst. no point to continue if we can't add this dst
      return kError;
    }
    lpmSrcMapping_[src] = rnum;
    if (!config_.testing) {
      modifyLpmSrcRule(ModifyAction::ADD, src, rnum);
    }
  }
  return 0;
}

bool VsoshlbLb::delSrcRoutingRule(const std::vector<std::string>& srcs) {
  if (!features_.srcRouting && !config_.testing) {
    LOG(ERROR) << "Source based routing is not enabled in forwarding plane";
    return false;
  }
  std::vector<folly::CIDRNetwork> src_networks;
  for (auto& src : srcs) {
    auto network = folly::IPAddress::tryCreateNetwork(src);
    if (network.hasValue()) {
      src_networks.push_back(network.value());
    }
  }
  return delSrcRoutingRule(src_networks);
}

bool VsoshlbLb::delSrcRoutingRule(const std::vector<folly::CIDRNetwork>& srcs) {
  if (!features_.srcRouting && !config_.testing) {
    LOG(ERROR) << "Source based routing is not enabled in forwarding plane";
    return false;
  }
  for (auto& src : srcs) {
    auto src_iter = lpmSrcMapping_.find(src);
    if (src_iter == lpmSrcMapping_.end()) {
      LOG(ERROR) << "trying to delete non existing src mapping " << src.first
                 << "/" << src.second;
      continue;
    }
    auto dst = numToReals_[src_iter->second];
    decreaseRefCountForReal(dst);
    if (!config_.testing) {
      modifyLpmSrcRule(ModifyAction::DEL, src, src_iter->second);
    }
    lpmSrcMapping_.erase(src_iter);
  }
  return true;
}

bool VsoshlbLb::clearAllSrcRoutingRules() {
  if (!features_.srcRouting && !config_.testing) {
    LOG(ERROR) << "Source based routing is not enabled in forwarding plane";
    return false;
  }
  for (auto& rule : lpmSrcMapping_) {
    auto dst_iter = numToReals_.find(rule.second);
    decreaseRefCountForReal(dst_iter->second);
    if (!config_.testing) {
      modifyLpmSrcRule(ModifyAction::DEL, rule.first, rule.second);
    }
  }
  lpmSrcMapping_.clear();
  return true;
}

uint32_t VsoshlbLb::getSrcRoutingRuleSize() {
  return lpmSrcMapping_.size();
}

std::unordered_map<std::string, std::string> VsoshlbLb::getSrcRoutingRule() {
  std::unordered_map<std::string, std::string> src_mapping;
  if (!features_.srcRouting && !config_.testing) {
    LOG(ERROR) << "Source based routing is not enabled in forwarding plane";
    return src_mapping;
  }
  for (auto& src : lpmSrcMapping_) {
    auto real = numToReals_[src.second];
    auto src_network =
        fmt::format("{}/{}", src.first.first.str(), src.first.second);
    src_mapping[src_network] = real.str();
  }
  return src_mapping;
}

std::unordered_map<folly::CIDRNetwork, std::string>
VsoshlbLb::getSrcRoutingRuleCidr() {
  std::unordered_map<folly::CIDRNetwork, std::string> src_mapping;
  if (!features_.srcRouting && !config_.testing) {
    LOG(ERROR) << "Source based routing is not enabled in forwarding plane";
    return src_mapping;
  }
  for (auto& src : lpmSrcMapping_) {
    auto real = numToReals_[src.second];
    src_mapping[src.first] = real.str();
  }
  return src_mapping;
}

const std::unordered_map<uint32_t, std::string> VsoshlbLb::getNumToRealMap() {
  std::unordered_map<uint32_t, std::string> reals;
  for (const auto& real : numToReals_) {
    reals[real.first] = real.second.str();
  }
  return reals;
}

bool VsoshlbLb::changeVsoshlbMonitorForwardingState(VsoshlbMonitorState state) {
  uint32_t key = kIntrospectionGkPos;
  struct ctl_value value;
  switch (state) {
    case VsoshlbMonitorState::ENABLED:
      value.value = 1;
      break;
    case VsoshlbMonitorState::DISABLED:
      value.value = 0;
      break;
  }
  auto res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::ctl_array), &key, &value);
  if (res != 0) {
    LOG(ERROR) << "can't change state of introspection forwarding plane";
    lbStats_.bpfFailedCalls++;
    return false;
  }
  return true;
}

bool VsoshlbLb::stopVsoshlbMonitor() {
  if (!monitor_) {
    return false;
  }
  if (!changeVsoshlbMonitorForwardingState(VsoshlbMonitorState::DISABLED)) {
    return false;
  }
  monitor_->stopMonitor();
  return true;
}

std::unique_ptr<folly::IOBuf> VsoshlbLb::getVsoshlbMonitorEventBuffer(
    EventId event) {
  if (!monitor_) {
    return nullptr;
  }
  return monitor_->getEventBuffer(event);
}

bool VsoshlbLb::restartVsoshlbMonitor(
    uint32_t limit,
    std::optional<PcapStorageFormat> storage) {
  if (!monitor_) {
    return false;
  }
  if (!changeVsoshlbMonitorForwardingState(VsoshlbMonitorState::ENABLED)) {
    return false;
  }
  monitor_->restartMonitor(limit, storage);
  return true;
}

VsoshlbMonitorStats VsoshlbLb::getVsoshlbMonitorStats() {
  struct VsoshlbMonitorStats stats;
  if (!monitor_) {
    return stats;
  }
  auto writer_stats = monitor_->getWriterStats();
  stats.limit = writer_stats.limit;
  stats.amount = writer_stats.amount;
  stats.bufferFull = writer_stats.bufferFull;
  return stats;
}

bool VsoshlbLb::modifyLpmSrcRule(
    ModifyAction action,
    const folly::CIDRNetwork& src,
    uint32_t rnum) {
  return modifyLpmMap("lpm_src", action, src, &rnum);
}

bool VsoshlbLb::modifyLpmMap(
    const std::string& lpmMapNamePrefix,
    ModifyAction action,
    const folly::CIDRNetwork& prefix,
    void* value) {
  auto lpm_addr = IpHelpers::parseAddrToBe(prefix.first.str());
  if (prefix.first.isV4()) {
    struct v4_lpm_key key_v4 = {
        .prefixlen = prefix.second, .addr = lpm_addr.daddr};
    std::string mapName = lpmMapNamePrefix + "_v4";
    if (action == ModifyAction::ADD) {
      auto res = bpfAdapter_->bpfUpdateMap(
          bpfAdapter_->getMapFdByName(mapName), &key_v4, value);
      if (res != 0) {
        LOG(ERROR) << "can't add new element into " << mapName
                   << ", error: " << folly::errnoStr(errno);
        lbStats_.bpfFailedCalls++;
        return false;
      }
    } else {
      auto res = bpfAdapter_->bpfMapDeleteElement(
          bpfAdapter_->getMapFdByName(mapName), &key_v4);
      if (res != 0) {
        LOG(ERROR) << "can't delete element from " << mapName
                   << ", error: " << folly::errnoStr(errno);
        lbStats_.bpfFailedCalls++;
        return false;
      }
    }
  } else {
    struct v6_lpm_key key_v6 = {
        .prefixlen = prefix.second,
    };
    std::string mapName = lpmMapNamePrefix + "_v6";
    std::memcpy(key_v6.addr, lpm_addr.v6daddr, 16);
    if (action == ModifyAction::ADD) {
      auto res = bpfAdapter_->bpfUpdateMap(
          bpfAdapter_->getMapFdByName(mapName), &key_v6, value);
      if (res != 0) {
        LOG(ERROR) << "can't add new element into " << mapName
                   << ", error: " << folly::errnoStr(errno);
        lbStats_.bpfFailedCalls++;
        return false;
      }
    } else {
      auto res = bpfAdapter_->bpfMapDeleteElement(
          bpfAdapter_->getMapFdByName(mapName), &key_v6);
      if (res != 0) {
        LOG(ERROR) << "can't delete element from " << mapName
                   << ", error: " << folly::errnoStr(errno);
        lbStats_.bpfFailedCalls++;
        return false;
      }
    }
  }
  return true;
}

bool VsoshlbLb::addInlineDecapDst(const std::string& dst) {
  if (!features_.inlineDecap && !config_.testing) {
    LOG(ERROR) << "source based routing is not enabled in forwarding plane";
    return false;
  }
  if (validateAddress(dst) == AddressType::INVALID) {
    LOG(ERROR) << "invalid decap destination address: " << dst;
    return false;
  }
  folly::IPAddress daddr(dst);
  if (decapDsts_.find(daddr) != decapDsts_.end()) {
    LOG(ERROR) << "trying to add already existing decap dst";
    return false;
  }
  if (decapDsts_.size() + 1 > config_.maxDecapDst) {
    LOG(ERROR) << "size of decap destinations map is exhausted";
    return false;
  }
  VLOG(2) << "adding decap dst " << dst;
  decapDsts_.insert(daddr);
  if (!config_.testing) {
    modifyDecapDst(ModifyAction::ADD, daddr);
  }
  return true;
}

bool VsoshlbLb::delInlineDecapDst(const std::string& dst) {
  if (!features_.inlineDecap && !config_.testing) {
    LOG(ERROR) << "source based routing is not enabled in forwarding plane";
    return false;
  }
  if (validateAddress(dst) == AddressType::INVALID) {
    LOG(ERROR) << "provided address in invalid format: " << dst;
    return false;
  }
  folly::IPAddress daddr(dst);
  auto dst_iter = decapDsts_.find(daddr);
  if (dst_iter == decapDsts_.end()) {
    LOG(ERROR) << "trying to delete non-existing decap dst " << dst;
    return false;
  }
  VLOG(2) << "deleting decap dst " << dst;
  decapDsts_.erase(dst_iter);
  if (!config_.testing) {
    modifyDecapDst(ModifyAction::DEL, daddr);
  }
  return true;
}

std::vector<std::string> VsoshlbLb::getInlineDecapDst() {
  std::vector<std::string> dsts;
  for (auto& dst : decapDsts_) {
    dsts.push_back(dst.str());
  }
  return dsts;
}

bool VsoshlbLb::modifyDecapDst(
    ModifyAction action,
    const folly::IPAddress& dst,
    uint32_t flags) {
  auto addr = IpHelpers::parseAddrToBe(dst);
  if (action == ModifyAction::ADD) {
    auto res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::decap_dst), &addr, &flags);
    if (res != 0) {
      LOG(ERROR) << "error while adding dst for inline decap " << dst
                 << ", error: " << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
      return false;
    }
  } else {
    auto res = bpfAdapter_->bpfMapDeleteElement(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::decap_dst), &addr);
    if (res != 0) {
      LOG(ERROR) << "error while deleting dst for inline decap " << dst
                 << ", error: " << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
      return false;
    }
  }
  return true;
}

void VsoshlbLb::modifyQuicRealsMapping(
    const ModifyAction action,
    const std::vector<QuicReal>& reals) {
  std::unordered_map<uint32_t, uint32_t> to_update;
  for (auto& real : reals) {
    if (validateAddress(real.address) == AddressType::INVALID) {
      LOG(ERROR) << "Invalid quic real's address: " << real.address;
      continue;
    }
    // kMaxQuicIdV2 was defined for cid v2 which is 24-bit.
    // cid v3 supports 32 bit server id.
    if (!config_.enableCidV3 && (real.id > kMaxQuicIdV2)) {
      LOG(ERROR) << "trying to add mapping for id out of assigned space";
      continue;
    }
    VLOG(4) << fmt::format(
        "modifying quic's real {} id 0x{:x}. action: {}",
        real.address,
        real.id,
        (int)action);
    auto raddr = folly::IPAddress(real.address);
    auto real_iter = quicMapping_.find(real.id);
    if (action == ModifyAction::DEL) {
      if (real_iter == quicMapping_.end()) {
        LOG(ERROR) << fmt::format(
            "trying to delete nonexisting mapping for id {:x} address {}",
            real.id,
            real.address);
        continue;
      }
      if (real_iter->second != raddr) {
        LOG(ERROR) << fmt::format(
            "deleted id {} pointed to diffrent address {} than given {}",
            real.id,
            real_iter->second.str(),
            real.address);
        continue;
      }
      decreaseRefCountForReal(raddr);
      quicMapping_.erase(real_iter);
    } else {
      if (real_iter != quicMapping_.end()) {
        if (real_iter->second == raddr) {
          continue;
        }
        LOG(WARNING) << fmt::format(
            "overriding address {} for existing mapping id {} address {}",
            real_iter->second.str(),
            real.id,
            real.address);
        decreaseRefCountForReal(real_iter->second);
      }
      auto rnum = increaseRefCountForReal(raddr);
      if (rnum == config_.maxReals) {
        LOG(ERROR) << "exhausted real's space";
        continue;
      }
      to_update[real.id] = rnum;
      quicMapping_[real.id] = raddr;
    }
  }
  if (!config_.testing) {
    auto server_id_map_fd =
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::server_id_map);
    uint32_t id, rnum;
    int res;
    for (auto& mapping : to_update) {
      id = mapping.first;
      rnum = mapping.second;
      res = bpfAdapter_->bpfUpdateMap(server_id_map_fd, &id, &rnum);
      if (res != 0) {
        LOG(ERROR) << "can't update quic mapping, error: "
                   << folly::errnoStr(errno);
        lbStats_.bpfFailedCalls++;
      }
    }
  }
}

std::vector<QuicReal> VsoshlbLb::getQuicRealsMapping() {
  std::vector<QuicReal> reals;
  QuicReal real;
  for (auto& mapping : quicMapping_) {
    real.id = mapping.first;
    real.address = mapping.second.str();
    reals.push_back(real);
  }
  return reals;
}

lb_stats VsoshlbLb::getStatsForVip(const VipKey& vip) {
  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    LOG(ERROR) << fmt::format(
        "trying to get stats for non-existing vip  {}:{}:{}",
        vip.address,
        vip.port,
        vip.proto);
    return lb_stats{};
  }
  auto num = vip_iter->second.getVipNum();
  return getLbStats(num);
}

lb_stats VsoshlbLb::getDecapStatsForVip(const VipKey& vip) {
  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    LOG(ERROR) << fmt::format(
        "trying to get stats for non-existing vip  {}:{}:{}",
        vip.address,
        vip.port,
        vip.proto);
    return lb_stats{};
  }
  auto num = vip_iter->second.getVipNum();
  return getLbStats(num, "decap_vip_stats");
}

uint64_t VsoshlbLb::getPacketsProcessedForHcKey(const VipKey& hcKey) {
  auto hc_key_iter = hckeys_.find(hcKey);
  if (hc_key_iter == hckeys_.end()) {
    LOG(ERROR) << "couldn't find hc_key";
    return 0;
  }

  uint32_t hcKeyId = hc_key_iter->second;

  unsigned int nr_cpus = BpfAdapter::getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return 0;
  }

  uint64_t stats[nr_cpus];
  uint64_t sum_stat = 0;

  for (auto& stat : stats) {
    stat = 0;
  }

  if (!config_.testing) {
    auto res = bpfAdapter_->bpfMapLookupElement(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::per_hckey_stats),
        &hcKeyId,
        stats);
    if (!res) {
      for (auto& stat : stats) {
        sum_stat += stat;
      }
    } else {
      LOG(ERROR) << "failed to lookup element in per_hckey_stats";
      lbStats_.bpfFailedCalls++;
    }
  }
  return sum_stat;
}

bool VsoshlbLb::logVipLruMissStats(VipKey& vip) {
  if (validateAddress(vip.address) == AddressType::INVALID) {
    LOG(ERROR) << "Invalid Vip address: " << vip.address;
    return false;
  }
  vip.address = folly::IPAddress(vip.address).str();
  lruMissStatsVip_ = vip;
  vip_definition vip_def = vipKeyToVipDefinition(vip);
  uint32_t vip_key = 0;
  auto res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::vip_miss_stats),
      &vip_key,
      &vip_def);
  if (res != 0) {
    LOG(ERROR) << "can't update lru miss stat vip, error: "
               << folly::errnoStr(errno);
    lbStats_.bpfFailedCalls++;
    return false;
  }

  int fd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::lru_miss_stats);
  if (fd < 0) {
    LOG(ERROR) << "Unable to get fd for lru_miss_stats";
    return false;
  }
  unsigned int nr_cpus = BpfAdapter::getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return false;
  }

  uint64_t value[nr_cpus];
  memset(value, 0, sizeof(value));
  for (uint32_t key = 0; key < config_.maxReals; key++) {
    res = bpfAdapter_->bpfUpdateMap(fd, &key, value);
    if (res != 0) {
      LOG(ERROR) << "Unable to reset lru_miss_stats";
      return false;
    }
  }

  return true;
}

std::unordered_map<std::string, int32_t> VsoshlbLb::getVipLruMissStats(
    VipKey& vip) {
  if (validateAddress(vip.address) == AddressType::INVALID) {
    LOG(ERROR) << "Invalid Vip address: " << vip.address;
    return std::unordered_map<std::string, int32_t>{};
  }
  vip.address = folly::IPAddress(vip.address).str();
  if (!lruMissStatsVip_.has_value() || !(lruMissStatsVip_.value() == vip)) {
    return std::unordered_map<std::string, int32_t>{};
  }

  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    LOG(ERROR) << "trying to get stats for non-existing vip";
    return std::unordered_map<std::string, int32_t>{};
  }

  auto reals_ids = vip_iter->second.getReals();

  std::unordered_map<std::string, int32_t> stats;
  unsigned int nr_cpus = libbpf_num_possible_cpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return stats;
  }

  uint64_t value[nr_cpus];
  for (uint32_t i = 0; i < reals_ids.size(); i++) {
    uint32_t key = reals_ids[i];
    int fd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::lru_miss_stats);
    auto res = bpfAdapter_->bpfMapLookupElement(fd, &key, value);
    if (res) {
      lbStats_.bpfFailedCalls++;
      LOG(ERROR) << "Unable to get lru_miss_stats";
      return std::unordered_map<std::string, int32_t>{};
    }
    int sum = 0;
    for (int j = 0; j < nr_cpus; j++) {
      sum += value[j];
    }
    auto real_info = numToReals_[reals_ids[i]].str();
    stats[real_info] = sum;
  }
  return stats;
}

lb_stats VsoshlbLb::getLruStats() {
  return getLbStats(config_.maxVips + kLruCntrOffset);
}

lb_stats VsoshlbLb::getLruMissStats() {
  return getLbStats(config_.maxVips + kLruMissOffset);
}

lb_stats VsoshlbLb::getLruFallbackStats() {
  return getLbStats(config_.maxVips + kLruFallbackOffset);
}

lb_stats VsoshlbLb::getIcmpTooBigStats() {
  return getLbStats(config_.maxVips + kIcmpTooBigOffset);
}

lb_quic_packets_stats VsoshlbLb::getLbQuicPacketsStats() {
  unsigned int nr_cpus = BpfAdapter::getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return lb_quic_packets_stats{};
  }
  lb_quic_packets_stats stats[nr_cpus];
  lb_quic_packets_stats sum_stat = {};

  if (!config_.testing) {
    int position = 0;
    auto res = bpfAdapter_->bpfMapLookupElement(
        bpfAdapter_->getMapFdByName("quic_stats_map"), &position, stats);

    if (!res) {
      for (auto& stat : stats) {
        sum_stat.ch_routed += stat.ch_routed;
        sum_stat.cid_initial += stat.cid_initial;
        sum_stat.cid_invalid_server_id += stat.cid_invalid_server_id;
        if (stat.cid_invalid_server_id_sample &&
            (invalidServerIds_.find(stat.cid_invalid_server_id_sample) ==
             invalidServerIds_.end()) &&
            invalidServerIds_.size() < kMaxInvalidServerIds) {
          LOG(ERROR) << "Invalid server id "
                     << stat.cid_invalid_server_id_sample << " in quic packet";
          invalidServerIds_.insert(stat.cid_invalid_server_id_sample);
          if (invalidServerIds_.size() == kMaxInvalidServerIds) {
            LOG(ERROR) << "Too many invalid server ids, will skip logging";
          }
        }
        sum_stat.cid_routed += stat.cid_routed;
        sum_stat.cid_unknown_real_dropped += stat.cid_unknown_real_dropped;
        sum_stat.cid_v0 += stat.cid_v0;
        sum_stat.cid_v1 += stat.cid_v1;
        sum_stat.cid_v2 += stat.cid_v2;
        sum_stat.cid_v3 += stat.cid_v3;
        sum_stat.dst_match_in_lru += stat.dst_match_in_lru;
        sum_stat.dst_mismatch_in_lru += stat.dst_mismatch_in_lru;
        sum_stat.dst_not_found_in_lru += stat.dst_not_found_in_lru;
      }
    } else {
      lbStats_.bpfFailedCalls++;
    }
  }
  return sum_stat;
}

lb_tpr_packets_stats VsoshlbLb::getTcpServerIdRoutingStats() {
  unsigned int nr_cpus = BpfAdapter::getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return lb_tpr_packets_stats{};
  }
  lb_tpr_packets_stats stats[nr_cpus];
  lb_tpr_packets_stats sum_stat = {};

  if (!config_.testing) {
    int position = 0;
    auto res = bpfAdapter_->bpfMapLookupElement(
        bpfAdapter_->getMapFdByName("tpr_stats_map"), &position, stats);
    if (!res) {
      for (auto& stat : stats) {
        sum_stat.ch_routed += stat.ch_routed;
        sum_stat.dst_mismatch_in_lru += stat.dst_mismatch_in_lru;
        sum_stat.sid_routed += stat.sid_routed;
        sum_stat.tcp_syn += stat.tcp_syn;
      }
    } else {
      lbStats_.bpfFailedCalls++;
    }
  }
  return sum_stat;
}

lb_stable_rt_packets_stats VsoshlbLb::getUdpStableRoutingStats() {
  unsigned int nr_cpus = BpfAdapter::getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return lb_stable_rt_packets_stats{};
  }
  lb_stable_rt_packets_stats stats[nr_cpus];
  lb_stable_rt_packets_stats sum_stat = {};

  if (!config_.testing) {
    int position = 0;
    auto res = bpfAdapter_->bpfMapLookupElement(
        bpfAdapter_->getMapFdByName("stable_rt_stats"), &position, stats);
    if (!res) {
      for (auto& stat : stats) {
        sum_stat.ch_routed += stat.ch_routed;
        sum_stat.cid_routed += stat.cid_routed;
        sum_stat.cid_invalid_server_id += stat.cid_invalid_server_id;
        sum_stat.cid_unknown_real_dropped += stat.cid_unknown_real_dropped;
      }
    } else {
      lbStats_.bpfFailedCalls++;
    }
  }
  return sum_stat;
}

lb_stats VsoshlbLb::getChDropStats() {
  return getLbStats(config_.maxVips + kChDropOffset);
}

lb_stats VsoshlbLb::getSrcRoutingStats() {
  return getLbStats(config_.maxVips + kLpmSrcOffset);
}

lb_stats VsoshlbLb::getInlineDecapStats() {
  return getLbStats(config_.maxVips + kInlineDecapOffset);
}

lb_stats VsoshlbLb::getGlobalLruStats() {
  return getLbStats(config_.maxVips + kGlobalLruOffset);
}

lb_stats VsoshlbLb::getDecapStats() {
  return getLbStats(config_.maxVips + kDecapCounterOffset);
}

lb_stats VsoshlbLb::getQuicIcmpStats() {
  return getLbStats(config_.maxVips + kQuicIcmpOffset);
}

lb_stats VsoshlbLb::getIcmpPtbV6Stats() {
  return getLbStats(config_.maxVips + kIcmpPtbV6Offset);
}

lb_stats VsoshlbLb::getIcmpPtbV4Stats() {
  return getLbStats(config_.maxVips + kIcmpPtbV4Offset);
}

lb_stats VsoshlbLb::getRealStats(uint32_t index) {
  return getLbStats(index, "reals_stats");
}

lb_stats VsoshlbLb::getLbStats(uint32_t position, const std::string& map) {
  unsigned int nr_cpus = BpfAdapter::getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return lb_stats();
  }
  lb_stats stats[nr_cpus];
  lb_stats sum_stat = {};

  if (!config_.testing) {
    auto res = bpfAdapter_->bpfMapLookupElement(
        bpfAdapter_->getMapFdByName(map), &position, stats);
    if (!res) {
      for (auto& stat : stats) {
        sum_stat.v1 += stat.v1;
        sum_stat.v2 += stat.v2;
      }
    } else {
      lbStats_.bpfFailedCalls++;
    }
  }
  return sum_stat;
}

std::vector<int64_t> VsoshlbLb::getPerCorePacketsStats() {
  if (config_.testing) {
    return {};
  }
  unsigned int nr_cpus = BpfAdapter::getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return {};
  }
  std::vector<int64_t> corePackets(nr_cpus, 0);
  lb_stats stats[nr_cpus];

  auto fd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::stats);

  for (const auto& [_, vip] : vips_) {
    uint32_t num = vip.getVipNum();
    auto res = bpfAdapter_->bpfMapLookupElement(fd, &num, stats);
    if (!res) {
      for (int i = 0; i < nr_cpus; i++) {
        corePackets[i] += stats[i].v1;
      }
    } else {
      LOG(ERROR) << "Error while querying stats for vip num " << num;
    }
  }
  return corePackets;
}

vip_definition VsoshlbLb::vipKeyToVipDefinition(const VipKey& vipKey) {
  auto vip_addr = IpHelpers::parseAddrToBe(vipKey.address);
  vip_definition vip_def = {};
  if ((vip_addr.flags & V6DADDR) > 0) {
    std::memcpy(vip_def.vipv6, vip_addr.v6daddr, 16);
  } else {
    vip_def.vip = vip_addr.daddr;
  }
  vip_def.port = folly::Endian::big(vipKey.port);
  vip_def.proto = vipKey.proto;
  return vip_def;
}

HealthCheckProgStats VsoshlbLb::getStatsForHealthCheckProgram() {
  unsigned int nr_cpus = BpfAdapter::getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Error while getting number of possible cpus";
    return HealthCheckProgStats();
  }
  uint32_t stats_index = kDefaultStatsIndex;
  HealthCheckProgStats stats[nr_cpus];
  HealthCheckProgStats total_stats = {};
  if (!config_.testing) {
    auto res = bpfAdapter_->bpfMapLookupElement(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_stats_map),
        &stats_index,
        stats);
    if (res) {
      lbStats_.bpfFailedCalls++;
    } else {
      for (auto& perCpuStat : stats) {
        total_stats.packetsProcessed += perCpuStat.packetsProcessed;
        total_stats.packetsSkipped += perCpuStat.packetsSkipped;
        total_stats.packetsDropped += perCpuStat.packetsDropped;
        total_stats.packetsTooBig += perCpuStat.packetsTooBig;
      }
    }
  }
  return total_stats;
}

VsoshlbBpfMapStats VsoshlbLb::getBpfMapStats(const std::string& map) {
  VsoshlbBpfMapStats map_stats = {0};
  int res = bpfAdapter_->getBpfMapMaxSize(map);
  if (res < 0) {
    throw std::runtime_error(fmt::format(
        "Failed to gather max entry count for map '{}'. res: {}", map, res));
  } else {
    map_stats.maxEntries = res;
  }
  res = bpfAdapter_->getBpfMapUsedSize(map);
  if (res < 0) {
    throw std::runtime_error(fmt::format(
        "Failed to gather current entry count for map '{}'. res: {}",
        map,
        res));
  } else {
    map_stats.currentEntries = res;
  }
  return map_stats;
}

bool VsoshlbLb::addHealthcheckerDst(
    const uint32_t somark,
    const std::string& dst) {
  if (!config_.enableHc) {
    return false;
  }
  if (validateAddress(dst) == AddressType::INVALID) {
    LOG(ERROR) << "Invalid healthcheck's destanation: " << dst;
    return false;
  }
  VLOG(4) << fmt::format(
      "adding healtcheck with so_mark {} to dst {}", somark, dst);
  folly::IPAddress hcaddr(dst);
  uint32_t key = somark;
  beaddr addr;

  auto hc_iter = hcReals_.find(somark);
  if (hc_iter == hcReals_.end() && hcReals_.size() == config_.maxReals) {
    LOG(ERROR) << "healthchecker's reals space exhausted";
    return false;
  }
  // for md bassed tunnels remote_ipv4 must be in host endian format
  if (hcaddr.isV4() && !features_.directHealthchecking) {
    addr = IpHelpers::parseAddrToInt(hcaddr);
  } else {
    addr = IpHelpers::parseAddrToBe(hcaddr);
  }
  if (!config_.testing) {
    auto res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_reals_map), &key, &addr);
    if (res != 0) {
      LOG(ERROR) << "can't add new real for healthchecking, error: "
                 << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
      return false;
    }
  }
  hcReals_[somark] = hcaddr;
  return true;
}

bool VsoshlbLb::delHealthcheckerDst(const uint32_t somark) {
  if (!config_.enableHc) {
    return false;
  }
  VLOG(4) << fmt::format("deleting healtcheck with so_mark {}", somark);

  uint32_t key = somark;

  auto hc_iter = hcReals_.find(somark);
  if (hc_iter == hcReals_.end()) {
    LOG(ERROR) << "trying to remove non-existing healthcheck";
    return false;
  }
  if (!config_.testing) {
    auto res = bpfAdapter_->bpfMapDeleteElement(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_reals_map), &key);
    if (res) {
      LOG(ERROR) << "can't remove hc w/ somark: " << key
                 << ", error: " << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
      return false;
    }
  }
  hcReals_.erase(hc_iter);
  return true;
}

std::unordered_map<uint32_t, std::string> VsoshlbLb::getHealthcheckersDst() {
  // would be empty map in case if enableHc_ is false
  std::unordered_map<uint32_t, std::string> hcs;
  for (const auto& hc : hcReals_) {
    hcs[hc.first] = hc.second.str();
  }
  return hcs;
}

std::string VsoshlbLb::simulatePacket(const std::string& inPacket) {
  std::string result;
  if (!initSimulator()) {
    return result;
  }
  auto inBuf = folly::IOBuf::copyBuffer(inPacket);
  auto outBuf = simulator_->runSimulation(std::move(inBuf));
  if (!outBuf) {
    LOG(ERROR) << "simulator failed to run simulation";
  } else {
    result = outBuf->moveToFbString().toStdString();
  }
  return result;
}

const std::string VsoshlbLb::getRealForFlow(const VsoshlbFlow& flow) {
  std::string result;
  if (!initSimulator()) {
    return result;
  }
  result = simulator_->getRealForFlow(flow);
  return result;
}

VsoshlbLb::LruStatsResponse VsoshlbLb::analyzeLru() {
  LruStatsResponse resp;
  for (int cpu = 0; cpu < lruMapsFd_.size(); cpu++) {
    int mapFd = lruMapsFd_[cpu];
    if (mapFd <= 0) {
      continue;
    }
    analyzeLruMap(mapFd, resp);
  }

  // Populate human readable vip 3-tuple
  for (const auto& [vipKey, _] : vips_) {
    auto maybeFlowKey = flowKeyFromParams(vipKey, "::0", 0);
    if (!maybeFlowKey) {
      LOG(ERROR) << "invalid vips_ key";
      resp.error = "invalid vips_ key";
      continue;
    }
    LruVipKey lruVipKey;
    lruVipKey.proto = maybeFlowKey->proto;
    lruVipKey.port = maybeFlowKey->port16[1];
    lruVipKey.dstv6[0] = maybeFlowKey->dstv6[0];
    lruVipKey.dstv6[1] = maybeFlowKey->dstv6[1];
    lruVipKey.dstv6[2] = maybeFlowKey->dstv6[2];
    lruVipKey.dstv6[3] = maybeFlowKey->dstv6[3];
    auto vipStatsIt = resp.perVipStats.find(lruVipKey);
    if (vipStatsIt == resp.perVipStats.end()) {
      continue;
    }
    vipStatsIt->second.vip =
        fmt::format("{}-{}-{}", vipKey.address, vipKey.port, vipKey.proto);
  }
  // We have VIPs with wildcard ports, such VipKey entries don't match any
  // existing VIPs above. Aggregate separate entries at IP level, ignoring port.
  std::unordered_map<LruVipKey, VipLruStats, LruVipKeyHash> aggregateVipStats;
  for (const auto& [vipKey, vipStats] : resp.perVipStats) {
    if (!vipStats.vip.empty()) {
      continue;
    }
    LruVipKey aggVipKey = vipKey;
    aggVipKey.port = 0;
    aggregateVipStats[aggVipKey].add(vipStats);
  }
  for (auto it = resp.perVipStats.begin(); it != resp.perVipStats.end();) {
    if (it->second.vip.empty()) {
      it = resp.perVipStats.erase(it);
    } else {
      it++;
    }
  }
  for (auto& [vipKey, vipStats] : aggregateVipStats) {
    resp.perVipStats[vipKey].add(vipStats);
    if (resp.perVipStats[vipKey].vip.empty()) {
      resp.perVipStats[vipKey].vip = fmt::format(
          "agg-{:x},{:x},{:x},{:x}-{}",
          vipKey.dstv6[0],
          vipKey.dstv6[1],
          vipKey.dstv6[2],
          vipKey.dstv6[3],
          vipKey.proto);
    }
  }

  return resp;
}

VsoshlbLb::LruEntries VsoshlbLb::searchLru(
    const VipKey& dstVip,
    const std::string& srcIp,
    uint16_t srcPort) {
  LruEntries lruEntries;

  auto maybeKey = flowKeyFromParams(dstVip, srcIp, srcPort);
  if (!maybeKey) {
    return lruEntries;
  }

  for (int cpu = 0; cpu < lruMapsFd_.size(); cpu++) {
    int mapFd = lruMapsFd_[cpu];
    if (mapFd <= 0) {
      continue;
    }
    auto maybeEntry = lookupLruMap(mapFd, key);
    if (maybeEntry) {
      maybeEntry->sourceMap = "cpu" + std::to_string(cpu);
      lruEntries.entries.push_back(*maybeEntry);
    } else {
      lruEntries.error = maybeEntry.error();
    }
  }

  int fallbackMapFd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::fallback_cache);
  if (fallbackMapFd > 0) {
    auto maybeEntry = lookupLruMap(fallbackMapFd, key);
    if (maybeEntry) {
      maybeEntry->sourceMap = "fallback";
      lruEntries.entries.push_back(*maybeEntry);
    } else {
      lruEntries.error = maybeEntry.error();
    }
  } else {
    LOG(ERROR) << "LRU fallback cache map not found";
  }

  fillAtimeDelta(lruEntries);
  return lruEntries;
}

VsoshlbLb::LruEntries VsoshlbLb::listLru(const VipKey& dstVip, int limit) {
  LruEntries response;

  // we only need dst values, setting src to dummy values
  auto maybeKey = flowKeyFromParams(dstVip, "::0", 0);
  if (!maybeKey) {
    response.error = "invalid vip address";
    return response;
  }
  bool isIPv6 = false;
  auto maybeVipAddr = folly::IPAddress::tryFromString(dstVip.address);
  if (maybeVipAddr) {
    isIPv6 = maybeVipAddr->isV6();
  }

  for (int cpu = 0; cpu < lruMapsFd_.size(); cpu++) {
    int mapFd = lruMapsFd_[cpu];
    if (mapFd <= 0) {
      continue;
    }
    if (!listLruMap(
            mapFd, "cpu" + std::to_string(cpu), key, isIPv6, limit, response)) {
      break;
    }
  }

  int fallbackMapFd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::fallback_cache);
  if (fallbackMapFd > 0) {
    listLruMap(fallbackMapFd, "fallback", key, isIPv6, limit, response);
  } else {
    LOG(ERROR) << "LRU fallback cache map not found";
  }

  fillAtimeDelta(response);
  return response;
}

void VsoshlbLb::fillAtimeDelta(LruEntries& entries) {
  int64_t currentTimeNs = BpfAdapter::getKtimeNs();
  for (auto& entry : entries.entries) {
    if (entry.atime > 0) {
      entry.atime_delta_sec = (currentTimeNs - entry.atime) / 1000000000;
    }
  }
}

folly::Expected<VsoshlbLb::LruEntry, std::string> VsoshlbLb::lookupLruMap(
    int mapFd,
    flow_key& key) {
  real_pos_lru real = {};
  int res = bpfAdapter_->bpfMapLookupElement(mapFd, &key, &real);
  if (res != 0) {
    if (res != -ENOENT) {
      // ENOENT is expected in case there is no entry in the lru map
      LOG(ERROR) << "Error while querying lru map: " << res;
    }
    return folly::makeUnexpected(fmt::format("error={}", res));
  }
  LruEntry entry;
  entry.realPos = real.pos;
  entry.atime = real.atime;
  if (real.pos == 0) {
    LOG(ERROR) << "Real position is 0";
  } else {
    auto realIt = numToReals_.find(real.pos);
    if (realIt == numToReals_.end()) {
      LOG(ERROR) << "Real with num " << real.pos << " not found";
    } else {
      entry.realAddress = realIt->second.str();
    }
  }
  return entry;
}

bool VsoshlbLb::listLruMap(
    int mapFd,
    const std::string& sourceMap,
    flow_key& filter,
    bool isIPv6,
    int limit,
    LruEntries& lruEntries) {
  real_pos_lru real = {};

  flow_key key;
  flow_key nextKey;
  memset(&key, 0, sizeof(key));
  memset(&nextKey, 0, sizeof(nextKey));

  for (int i = 0; i < kLruMaxLookups; i++) {
    int nextKeyRes = bpfAdapter_->bpfMapGetNextKey(mapFd, &key, &nextKey);
    // First lookup has empty key
    if (i != 0) {
      bool vipMatch =
          (key.proto == filter.proto && key.port16[1] == filter.port16[1] &&
           key.dstv6[0] == filter.dstv6[0] && key.dstv6[1] == filter.dstv6[1] &&
           key.dstv6[2] == filter.dstv6[2] && key.dstv6[3] == filter.dstv6[3]);
      if (vipMatch) {
        int lookupRes = bpfAdapter_->bpfMapLookupElement(mapFd, &key, &real);
        if (lookupRes == 0) {
          LruEntry entry;
          entry.sourceMap = sourceMap;
          entry.realPos = real.pos;
          entry.atime = real.atime;
          if (real.pos == 0) {
            LOG(ERROR) << "Real position is 0";
          } else {
            auto realIt = numToReals_.find(real.pos);
            if (realIt == numToReals_.end()) {
              LOG(ERROR) << "Real with num " << real.pos << " not found";
            } else {
              entry.realAddress = realIt->second.str();
            }
          }
          if (isIPv6) {
            entry.srcAddress =
                folly::IPAddressV6::fromBinary(
                    folly::ByteRange(
                        reinterpret_cast<const uint8_t*>(key.srcv6), 16))
                    .str();
          } else {
            entry.srcAddress = folly::IPAddressV4::fromLong(key.src).str();
          }
          entry.srcPort = ntohs(key.port16[0]);
          lruEntries.entries.push_back(entry);
        } else {
          LOG(ERROR) << "Error while querying lru map, error=" << lookupRes;
          lruEntries.error = "lookup error";
        }
      }
    }
    if (nextKeyRes != 0) {
      if (nextKeyRes == -ENOENT) {
        // ENOENT is returned when last entry in the map is reached
        return true;
      } else {
        LOG(ERROR) << "Error while querying lru map, error=" << nextKeyRes;
        lruEntries.error = "query error";
        return true;
      }
    }
    if (i == kLruMaxLookups - 1) {
      LOG(ERROR) << "Max lookups reached";
      lruEntries.error = "maxed lookups";
    }
    if (lruEntries.entries.size() >= limit) {
      LOG(ERROR) << "lookupLru limit reached";
      lruEntries.error = "limit reached";
      return false;
    }
    key = nextKey;
  }
  return true;
}

void VsoshlbLb::analyzeLruMap(int mapFd, LruStatsResponse& lruStats) {
  int64_t currentTimeNs = BpfAdapter::getKtimeNs();

  real_pos_lru real = {};

  flow_key key;
  flow_key nextKey;
  memset(&key, 0, sizeof(key));
  memset(&nextKey, 0, sizeof(nextKey));

  for (int i = 0; i < kLruMaxLookups; i++) {
    int nextKeyRes = bpfAdapter_->bpfMapGetNextKey(mapFd, &key, &nextKey);
    // First lookup has empty key
    if (i != 0) {
      int lookupRes = bpfAdapter_->bpfMapLookupElement(mapFd, &key, &real);
      if (lookupRes == 0) {
        LruVipKey vipKey;
        vipKey.proto = key.proto;
        vipKey.port = key.port16[1];
        vipKey.dstv6[0] = key.dstv6[0];
        vipKey.dstv6[1] = key.dstv6[1];
        vipKey.dstv6[2] = key.dstv6[2];
        vipKey.dstv6[3] = key.dstv6[3];

        VipLruStats& stats = lruStats.perVipStats[vipKey];

        stats.count++;
        if (real.pos == 0) {
          stats.staleRealsCount++;
          LOG(ERROR) << "Real position is 0";
        } else {
          auto realIt = numToReals_.find(real.pos);
          if (realIt == numToReals_.end()) {
            LOG(ERROR) << "Real with num " << real.pos << " not found";
            stats.staleRealsCount++;
          }
        }
        if (real.atime == 0) {
          stats.atimeZeroCount++;
        } else {
          int deltaSec = (currentTimeNs - real.atime) / 1000000000;
          if (deltaSec < 30) {
            stats.atimeUnder30secCount++;
          } else if (deltaSec < 60) {
            stats.atime30to60secCount++;
          } else {
            stats.atimeOver60secCount++;
          }
        }
      } else {
        LOG(ERROR) << "Error while querying lru map, error=" << lookupRes;
        lruStats.error = "lookup error";
      }
    }
    if (nextKeyRes != 0) {
      if (nextKeyRes == -ENOENT) {
        // ENOENT is returned when last entry in the map is reached
        return;
      } else {
        LOG(ERROR) << "Error while querying lru map, error=" << nextKeyRes;
        lruStats.error = "query error";
        return;
      }
    }
    if (i == kLruMaxLookups - 1) {
      LOG(ERROR) << "Max lookups reached";
      lruStats.error = "maxed lookups";
    }
    key = nextKey;
  }
}

std::optional<flow_key> VsoshlbLb::flowKeyFromParams(
    const VipKey& dstVip,
    const std::string& srcIp,
    uint16_t srcPort) {
  flow_key key = {};
  memset(&key, 0, sizeof(key));

  auto vip = folly::IPAddress::tryFromString(dstVip.address);
  if (!vip) {
    LOG(ERROR) << "Invalid vip address: " << dstVip.address;
    return std::nullopt;
  }
  auto beAddr = IpHelpers::parseAddrToBe(*vip);
  if (vip->isV4()) {
    key.dst = beAddr.daddr;
  } else {
    memcpy(key.dstv6, beAddr.v6daddr, sizeof(key.dstv6));
  }
  key.port16[1] = htons(dstVip.port);

  auto src = folly::IPAddress::tryFromString(srcIp);
  if (!src) {
    LOG(ERROR) << "Invalid src address: " << srcIp;
    return std::nullopt;
  }
  auto beSrcAddr = IpHelpers::parseAddrToBe(*src);
  if (src->isV4()) {
    key.src = beSrcAddr.daddr;
  } else {
    memcpy(key.srcv6, beSrcAddr.v6daddr, sizeof(key.srcv6));
  }
  key.port16[0] = htons(srcPort);
  key.proto = dstVip.proto;
  return key;
}

std::vector<std::string> VsoshlbLb::deleteLru(
    const VipKey& dstVip,
    const std::string& srcIp,
    uint16_t srcPort) {
  std::vector<std::string> mapsWithDeletions;
  auto maybeKey = flowKeyFromParams(dstVip, srcIp, srcPort);
  if (!maybeKey) {
    return mapsWithDeletions;
  }
  for (int cpu = 0; cpu < lruMapsFd_.size(); cpu++) {
    int mapFd = lruMapsFd_[cpu];
    if (mapFd <= 0) {
      continue;
    }
    int res = bpfAdapter_->bpfMapDeleteElement(mapFd, &key);
    if (res == 0) {
      mapsWithDeletions.push_back("cpu" + std::to_string(cpu));
    } else {
      if (errno != ENOENT) {
        // ENOENT is expected in case there is no entry in the lru map
        LOG(ERROR) << "Error while querying lru map: " << errno;
      }
    }
  }

  int fallbackMapFd = bpfAdapter_->getMapFdByName(VsoshlbLbMaps::fallback_cache);
  if (fallbackMapFd > 0) {
    int res = bpfAdapter_->bpfMapDeleteElement(fallbackMapFd, &key);
    if (res == 0) {
      mapsWithDeletions.push_back("fallback");
    } else {
      if (errno != ENOENT) {
        // ENOENT is expected in case there is no entry in the lru map
        LOG(ERROR) << "Error while querying lru map: " << errno;
      }
    }
  } else {
    LOG(ERROR) << "LRU fallback cache map not found";
  }
  return mapsWithDeletions;
}

VsoshlbLb::PurgeResponse VsoshlbLb::purgeVipLru(const VipKey& dstVip) {
  PurgeResponse response;
  // we only need dst values, setting src to dummy values
  auto maybeKey = flowKeyFromParams(dstVip, "::0", 0);
  if (!maybeKey) {
    response.error = "invalid vip address";
    return response;
  }
  for (int cpu = 0; cpu < lruMapsFd_.size(); cpu++) {
    int mapFd = lruMapsFd_[cpu];
    if (mapFd <= 0) {
      continue;
    }
    auto mapResp = purgeVipLruMap(mapFd, filterKey);
    response.deletedCount += mapResp.deletedCount;
    if (!mapResp.error.empty()) {
      response.error = mapResp.error;
    }
  }
  int fallbackMapFd = bpfAdapter_->getMapFdByName("fallback_cache");
  if (fallbackMapFd > 0) {
    auto mapResp = purgeVipLruMap(fallbackMapFd, filterKey);
    response.deletedCount += mapResp.deletedCount;
    if (!mapResp.error.empty()) {
      response.error = mapResp.error;
    }
  } else {
    LOG(ERROR) << "LRU fallback cache map not found";
  }
  return response;
}

VsoshlbLb::PurgeResponse VsoshlbLb::purgeVipLruMap(
    int mapFd,
    const flow_key& filter) {
  PurgeResponse response;

  flow_key key;
  flow_key nextKey;
  memset(&key, 0, sizeof(key));
  memset(&nextKey, 0, sizeof(nextKey));

  for (int i = 0; i < kLruMaxLookups; i++) {
    int lookupRes = bpfAdapter_->bpfMapGetNextKey(mapFd, &key, &nextKey);
    // First lookup has empty key
    if (i != 0) {
      if (key.proto == filter.proto && key.port16[1] == filter.port16[1] &&
          key.dstv6[0] == filter.dstv6[0] && key.dstv6[1] == filter.dstv6[1] &&
          key.dstv6[2] == filter.dstv6[2] && key.dstv6[3] == filter.dstv6[3]) {
        // dst of the key matches the filter
        int delRes = bpfAdapter_->bpfMapDeleteElement(mapFd, &key);
        if (delRes != 0) {
          LOG(ERROR) << "Error while deleting lru map: " << delRes;
          response.error = "delete error";
        } else {
          response.deletedCount++;
        }
      }
    }
    if (lookupRes != 0) {
      if (lookupRes == -ENOENT) {
        // ENOENT is returned when last entry in the map is reached
        return response;
      } else {
        LOG(ERROR) << "Error while querying lru map, error=" << lookupRes;
        response.error = "query error";
        return response;
      }
    }
    if (i == kLruMaxLookups - 1) {
      LOG(ERROR) << "Max lookups reached";
      response.error = "maxed lookups";
    }
    key = nextKey;
  }
  return response;
}

bool VsoshlbLb::updateVipMap(
    const ModifyAction action,
    const VipKey& vip,
    vip_meta* meta) {
  vip_definition vip_def = vipKeyToVipDefinition(vip);
  if (action == ModifyAction::ADD) {
    auto res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::vip_map), &vip_def, meta);
    if (res != 0) {
      LOG(ERROR) << "can't add new element into vip_map, error: "
                 << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
      return false;
    }
  } else {
    auto res = bpfAdapter_->bpfMapDeleteElement(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::vip_map), &vip_def);
    if (res != 0) {
      LOG(ERROR) << "can't delete element from vip_map, error: "
                 << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
      return false;
    }
  }
  return true;
}

bool VsoshlbLb::updateHcKeyMap(
    const ModifyAction action,
    const VipKey& hcKey,
    uint32_t hcKeyId) {
  vip_definition hc_key_def = vipKeyToVipDefinition(hcKey);

  if (action == ModifyAction::ADD) {
    auto res = bpfAdapter_->bpfUpdateMap(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_key_map),
        &hc_key_def,
        &hcKeyId);
    if (res != 0) {
      LOG(ERROR) << "can't add new element into hc_key_map, error: "
                 << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
      return false;
    }
  } else {
    auto res = bpfAdapter_->bpfMapDeleteElement(
        bpfAdapter_->getMapFdByName(VsoshlbLbMaps::hc_key_map), &hc_key_def);
    if (res != 0) {
      LOG(ERROR) << "can't delete element from hc_key_map, error: "
                 << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
      return false;
    }
  }
  return true;
}

bool VsoshlbLb::updateRealsMap(
    const folly::IPAddress& real,
    uint32_t num,
    uint8_t flags) {
  auto real_addr = IpHelpers::parseAddrToBe(real);
  flags &= ~V6DADDR; // to keep IPv4/IPv6 specific flag
  real_addr.flags |= flags;
  auto res = bpfAdapter_->bpfUpdateMap(
      bpfAdapter_->getMapFdByName("reals"), &num, &real_addr);
  if (res != 0) {
    LOG(ERROR) << "can't add new real, error: " << folly::errnoStr(errno);
    lbStats_.bpfFailedCalls++;
    return false;
  } else {
    return true;
  }
};

void VsoshlbLb::decreaseRefCountForReal(const folly::IPAddress& real) {
  auto real_iter = reals_.find(real);
  if (real_iter == reals_.end()) {
    return;
  }
  real_iter->second.refCount--;
  if (real_iter->second.refCount == 0) {
    auto num = real_iter->second.num;
    // no more vips using this real
    realNums_.push_back(num);
    reals_.erase(real_iter);
    numToReals_.erase(num);

    if (realsIdCallback_) {
      realsIdCallback_->onRealDeleted(real, num);
    }
  }
}

uint32_t VsoshlbLb::increaseRefCountForReal(
    const folly::IPAddress& real,
    uint8_t flags) {
  auto real_iter = reals_.find(real);
  flags &= ~V6DADDR; // to keep IPv4/IPv6 specific flag
  if (real_iter != reals_.end()) {
    real_iter->second.refCount++;
    return real_iter->second.num;
  } else {
    if (realNums_.size() == 0) {
      return config_.maxReals;
    }
    auto rnum = realNums_[0];
    realNums_.pop_front();
    numToReals_[rnum] = real;
    rmeta.refCount = 1;
    rmeta.num = rnum;
    rmeta.flags = flags;
    reals_[real] = rmeta;
    if (!config_.testing) {
      updateRealsMap(real, rnum, flags);
    }

    if (realsIdCallback_) {
      realsIdCallback_->onRealAdded(real, rnum);
    }

    return rnum;
  }
}

bool VsoshlbLb::hasFeature(VsoshlbFeatureEnum feature) {
  switch (feature) {
    case VsoshlbFeatureEnum::LocalDeliveryOptimization:
      return features_.localDeliveryOptimization;
    case VsoshlbFeatureEnum::DirectHealthchecking:
      return features_.directHealthchecking;
    case VsoshlbFeatureEnum::GueEncap:
      return features_.gueEncap;
    case VsoshlbFeatureEnum::InlineDecap:
      return features_.inlineDecap;
    case VsoshlbFeatureEnum::Introspection:
      return features_.introspection;
    case VsoshlbFeatureEnum::SrcRouting:
      return features_.srcRouting;
    case VsoshlbFeatureEnum::FlowDebug:
      return features_.flowDebug;
  }
  folly::assume_unreachable();
}

bool VsoshlbLb::installFeature(
    VsoshlbFeatureEnum feature,
    const std::string& prog_path) {
  if (hasFeature(feature)) {
    LOG(ERROR) << "already have requested feature";
    return true;
  }
  if (prog_path.empty()) {
    LOG(ERROR) << "failed to install feature: prog_path is empty";
    return false;
  }
  auto original_balancer_prog = config_.balancerProgPath;
  if (!reloadBalancerProg(prog_path)) {
    LOG(ERROR) << "failed to install feature: reloading prog failed";

    if (!reloadBalancerProg(original_balancer_prog)) {
      LOG(ERROR) << "failed to reload original balancer prog";
    }
    return false;
  }
  if (!config_.testing) {
    attachBpfProgs();
  }
  return hasFeature(feature);
}

bool VsoshlbLb::removeFeature(
    VsoshlbFeatureEnum feature,
    const std::string& prog_path) {
  if (!hasFeature(feature)) {
    return true;
  }
  if (prog_path.empty()) {
    return false;
  }
  auto original_balancer_prog = config_.balancerProgPath;
  if (!reloadBalancerProg(prog_path)) {
    LOG(ERROR) << "provided prog does not have wanted feature, "
               << "reverting by reloading original balancer prog";

    if (!reloadBalancerProg(original_balancer_prog)) {
      LOG(ERROR) << "failed to reload original balancer prog";
    }
    return false;
  }
  if (!config_.testing) {
    attachBpfProgs();
  }
  return !hasFeature(feature);
}

void VsoshlbLb::setRealsIdCallback(RealsIdCallback* callback) {
  realsIdCallback_ = callback;
}

void VsoshlbLb::unsetRealsIdCallback() {
  realsIdCallback_ = nullptr;
}

std::vector<int> VsoshlbLb::getGlobalLruMapsFds() {
  std::vector<int> result;
  for (auto& forwardingCore : forwardingCores_) {
    result.push_back(globalLruMapsFd_[forwardingCore]);
  }
  if (globalLruFallbackFd_ >= 0) {
    result.push_back(globalLruFallbackFd_);
  }
  return result;
}

lb_stats VsoshlbLb::getSidRoutingStatsForVip(const VipKey& vip) {
  auto vip_iter = vips_.find(vip);
  if (vip_iter == vips_.end()) {
    VLOG(1) << fmt::format(
        "trying to get sid routing stats for non-existing vip  {}:{}:{}",
        vip.address,
        vip.port,
        vip.proto);
    return lb_stats{};
  }
  auto vipNum = vip_iter->second.getVipNum();
  return getLbStats(vipNum, "server_id_stats");
}

void VsoshlbLb::invalidateServerIds(const std::vector<int32_t>& serverIds) {
  auto server_id_map_fd =
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::server_id_map);
  for (auto serverId : serverIds) {
    LOG(ERROR) << "Invalidate the server id " << serverId
               << " in server_id_map";
    int res;
    if (config_.enableCidV3) {
      // server_id_map is of BPF_MAP_TYPE_HASH when cidv3 is enabled
      res = bpfAdapter_->bpfMapDeleteElement(server_id_map_fd, &serverId);
    } else {
      // server_id_map is of BPF_MAP_TYPE_ARRAY when cidv3 is NOT enabled
      int valueZero = 0;
      res = bpfAdapter_->bpfUpdateMap(server_id_map_fd, &serverId, &valueZero);
    }
    if (res != 0) {
      LOG(ERROR) << "can't invalidate the entry for the server id " << serverId
                 << " from the server_id_map, error: "
                 << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
    }
  }
}

void VsoshlbLb::revalidateServerIds(const std::vector<QuicReal>& quicReals) {
  auto server_id_map_fd =
      bpfAdapter_->getMapFdByName(VsoshlbLbMaps::server_id_map);
  for (auto [real, serverId] : quicReals) {
    int realIndex = getIndexForReal(real);
    if (realIndex == kError) {
      LOG(ERROR) << "Can't find real " << real << " in the reals map";
      continue;
    }
    LOG(WARNING) << "Updating server id map with server id " << serverId
                 << ", real index " << realIndex;
    auto res =
        bpfAdapter_->bpfUpdateMap(server_id_map_fd, &serverId, &realIndex);
    if (res != 0) {
      LOG(ERROR) << "can't revalidate the entry for the server id " << serverId
                 << " to the real " << real
                 << " in server_id_map, error: " << folly::errnoStr(errno);
      lbStats_.bpfFailedCalls++;
    }
  }
}

bool VsoshlbLb::initSimulator() {
  if (!progsLoaded_) {
    LOG(ERROR) << "bpf programs are not loaded";
    return false;
  }
  simulator_ = std::make_unique<VsoshlbSimulator>(getVsoshlbProgFd());
  return true;
}

} // namespace vsoshlb

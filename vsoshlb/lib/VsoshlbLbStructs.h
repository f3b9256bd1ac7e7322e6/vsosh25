
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "vsoshlb/lib/CHHelpers.h"
#include "vsoshlb/lib/MonitoringStructs.h"

namespace vsoshlb {

constexpr uint32_t kDefaultPriority = 2307;

namespace {
constexpr uint32_t kDefaultVsoshlbPos = 2;
constexpr uint32_t kDefaultMaxVips = 512;
constexpr uint32_t kDefaultMaxReals = 4096;
constexpr uint32_t kLbDefaultChRingSize = 65537;
constexpr uint32_t kDefaultMaxLpmSrcSize = 3000000;
constexpr uint32_t kDefaultMaxDecapDstSize = 6;
constexpr uint32_t kDefaultNumOfPages = 2;
constexpr uint32_t kDefaultMonitorQueueSize = 4096;
constexpr uint32_t kDefaultMonitorPcktLimit = 0;
constexpr uint32_t kDefaultMonitorSnapLen = 128;
constexpr unsigned int kDefaultLruSize = 8000000;
constexpr uint32_t kDefaultGlobalLruSize = 100000;
constexpr uint32_t kNoFlags = 0;
constexpr uint32_t kUnspecifiedInterfaceIndex = 0;
std::string kNoExternalMap;
std::string kDefaultHcInterface;
std::string kAddressNotSpecified;
} // namespace

/**
  /**
  uint32_t num;

  /**
  uint32_t refCount;
  uint8_t flags;
};

/**

struct NewReal {
  std::string address;
  uint32_t weight;
  uint8_t flags;
};

/**
struct QuicReal {
  std::string address;
  uint32_t id;
};

/**
enum class AddressType {
  INVALID,
  HOST,
  NETWORK,
};

/**
enum class PcapStorageFormat {
  FILE,
  IOBUF,
  PIPE,
};

/**
struct VsoshlbMonitorConfig {
  uint32_t nCpus;
  uint32_t pages{kDefaultNumOfPages};
  int mapFd;
  uint32_t queueSize{kDefaultMonitorQueueSize};
  uint32_t pcktLimit{kDefaultMonitorPcktLimit};
  uint32_t snapLen{kDefaultMonitorSnapLen};
  std::set<monitoring::EventId> events{monitoring::kAllEventIds};
  std::string path{"/tmp/vsoshlb_pcap"};
  PcapStorageFormat storage{PcapStorageFormat::FILE};
  uint32_t bufferSize{0};
};

/**
struct VsoshlbConfig {
  std::string mainInterface;
  std::string v4TunInterface = kDefaultHcInterface;
  std::string v6TunInterface = kDefaultHcInterface;
  std::string balancerProgPath;
  std::string healthcheckingProgPath;
  std::vector<uint8_t> defaultMac;
  uint32_t priority = kDefaultPriority;
  std::string rootMapPath = kNoExternalMap;
  uint32_t rootMapPos = kDefaultVsoshlbPos;
  bool enableHc = true;
  bool tunnelBasedHCEncap = true;
  uint32_t maxVips = kDefaultMaxVips;
  uint32_t maxReals = kDefaultMaxReals;
  uint32_t chRingSize = kLbDefaultChRingSize;
  bool testing = false;
  uint64_t LruSize = kDefaultLruSize;
  std::vector<int32_t> forwardingCores;
  std::vector<int32_t> numaNodes;
  uint32_t maxLpmSrcSize = kDefaultMaxLpmSrcSize;
  uint32_t maxDecapDst = kDefaultMaxDecapDstSize;
  std::string hcInterface = kDefaultHcInterface;
  uint32_t xdpAttachFlags = kNoFlags;
  struct VsoshlbMonitorConfig monitorConfig;
  bool memlockUnlimited = true;
  std::string vsoshlbSrcV4 = kAddressNotSpecified;
  std::string vsoshlbSrcV6 = kAddressNotSpecified;
  std::vector<uint8_t> localMac;
  HashFunction hashFunction = HashFunction::Maglev;
  bool flowDebug = false;
  uint32_t globalLruSize = kDefaultGlobalLruSize;
  bool useRootMap = true;
  bool enableCidV3 = false;
  uint32_t mainInterfaceIndex = kUnspecifiedInterfaceIndex;
  uint32_t hcInterfaceIndex = kUnspecifiedInterfaceIndex;
  bool cleanupOnShutdown = true;
};

/**
struct VsoshlbMonitorStats {
  uint32_t limit{0};
  uint32_t amount{0};
  uint32_t bufferFull{0};
};

/**
struct VsoshlbBpfMapStats {
  uint32_t maxEntries{0};
  uint32_t currentEntries{0};
};

/**
struct VsoshlbLbStats {
  uint64_t bpfFailedCalls{0};
  uint64_t addrValidationFailed{0};
};

/**
struct HealthCheckProgStats {
  uint64_t packetsProcessed{0};
  uint64_t packetsDropped{0};
  uint64_t packetsSkipped{0};
  uint64_t packetsTooBig{0};
};

/**
struct VsoshlbFeatures {
  bool srcRouting{false};
  bool inlineDecap{false};
  bool introspection{false};
  bool gueEncap{false};
  bool directHealthchecking{false};
  bool localDeliveryOptimization{false};
  bool flowDebug{false};
};

/**
enum class VsoshlbFeatureEnum : uint8_t {
  SrcRouting = 1 << 0,
  InlineDecap = 1 << 1,
  Introspection = 1 << 2,
  GueEncap = 1 << 3,
  DirectHealthchecking = 1 << 4,
  LocalDeliveryOptimization = 1 << 5,
  FlowDebug = 1 << 6,
};

/**

class VipKey {
 public:
  std::string address;
  uint16_t port;
  uint8_t proto;

  bool operator==(const VipKey& other) const {
    return (
        address == other.address && port == other.port && proto == other.proto);
  }
};

struct VipKeyHasher {
  std::size_t operator()(const VipKey& k) const {
    return ((std::hash<std::string>()(k.address) ^
             (std::hash<uint16_t>()(k.port) << 1)) >>
            1) ^
        (std::hash<uint8_t>()(k.proto) << 1);
  }
};

} // namespace vsoshlb

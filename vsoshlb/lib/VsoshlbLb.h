
#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <folly/IPAddress.h>
#include <folly/container/F14Map.h>

#include "vsoshlb/lib/BalancerStructs.h"
#include "vsoshlb/lib/BaseBpfAdapter.h"
#include "vsoshlb/lib/BpfAdapter.h"
#include "vsoshlb/lib/CHHelpers.h"
#include "vsoshlb/lib/IpHelpers.h"
#include "vsoshlb/lib/VsoshlbLbStructs.h"
#include "vsoshlb/lib/VsoshlbSimulator.h"
#include "vsoshlb/lib/MonitoringStructs.h"
#include "vsoshlb/lib/Vip.h"

namespace vsoshlb {

class VsoshlbMonitor;

/**
constexpr int kMacAddrPos = 0;
constexpr int kIpv4TunPos = 1;
constexpr int kIpv6TunPos = 2;
constexpr int kMainIntfPos = 3;
constexpr int kHcIntfPos = 4;
constexpr int kIntrospectionGkPos = 5;

/**
constexpr uint32_t kLruCntrOffset = 0;
constexpr uint32_t kLruMissOffset = 1;
constexpr uint32_t kLruFallbackOffset = 3;
constexpr uint32_t kIcmpTooBigOffset = 4;
constexpr uint32_t kLpmSrcOffset = 5;
constexpr uint32_t kInlineDecapOffset = 6;
constexpr uint32_t kGlobalLruOffset = 8;
constexpr uint32_t kChDropOffset = 9;
constexpr uint32_t kDecapCounterOffset = 10;
constexpr uint32_t kQuicIcmpOffset = 11;
constexpr uint32_t kIcmpPtbV6Offset = 12;
constexpr uint32_t kIcmpPtbV4Offset = 13;

/**
constexpr int kFallbackLruSize = 1024;
constexpr int kMapNoFlags = 0;
constexpr int kNoNuma = -1;

constexpr uint8_t V6DADDR = 1;
constexpr int kDeleteXdpProg = -1;
constexpr int kMacBytes = 6;
constexpr int kCtlMapSize = 16;
constexpr int kLruPrototypePos = 0;
constexpr int kMaxForwardingCores = 128;
constexpr int kFirstElem = 0;
constexpr int kError = -1;
constexpr uint32_t kMaxQuicIdV2 = 0x00fffffe; // 2^24-2
constexpr uint32_t kDefaultStatsIndex = 0;
constexpr folly::StringPiece kEmptyString = "";
constexpr uint32_t kSrcV4Pos = 0;
constexpr uint32_t kSrcV6Pos = 1;
constexpr uint32_t kRecirculationIndex = 0;
constexpr uint32_t kHcSrcMacPos = 0;
constexpr uint32_t kHcDstMacPos = 1;

namespace VsoshlbLbMaps {
constexpr auto ch_rings = "ch_rings";
constexpr auto ctl_array = "ctl_array";
constexpr auto decap_dst = "decap_dst";
constexpr auto event_pipe = "event_pipe";
constexpr auto fallback_cache = "fallback_cache";
constexpr auto fallback_glru = "fallback_glru";
constexpr auto flow_debug_lru = "flow_debug_lru";
constexpr auto flow_debug_maps = "flow_debug_maps";
constexpr auto global_lru = "global_lru";
constexpr auto global_lru_maps = "global_lru_maps";
constexpr auto hc_ctrl_map = "hc_ctrl_map";
constexpr auto hc_key_map = "hc_key_map";
constexpr auto hc_pckt_macs = "hc_pckt_macs";
constexpr auto hc_pckt_srcs_map = "hc_pckt_srcs_map";
constexpr auto hc_reals_map = "hc_reals_map";
constexpr auto hc_stats_map = "hc_stats_map";
constexpr auto vsoshlb_lru = "vsoshlb_lru";
constexpr auto lpm_src_v4 = "lpm_src_v4";
constexpr auto lru_mapping = "lru_mapping";
constexpr auto lru_miss_stats = "lru_miss_stats";
constexpr auto pckt_srcs = "pckt_srcs";
constexpr auto per_hckey_stats = "per_hckey_stats";
constexpr auto reals = "reals";
constexpr auto server_id_map = "server_id_map";
constexpr auto stats = "stats";
constexpr auto vip_map = "vip_map";
constexpr auto vip_miss_stats = "vip_miss_stats";
} // namespace VsoshlbLbMaps

namespace {
/**
enum class VsoshlbMonitorState {
  DISABLED,
  ENABLED,
};

/**
constexpr folly::StringPiece kBalancerProgName = "balancer_ingress";
constexpr folly::StringPiece kHealthcheckerProgName = "healthcheck_encap";
} // namespace

/**
class VsoshlbLb {
 public:
  class RealsIdCallback {
   public:
    virtual ~RealsIdCallback() {}

    virtual void onRealAdded(const folly::IPAddress& real, uint32_t id) = 0;

    virtual void onRealDeleted(const folly::IPAddress& real, uint32_t id) = 0;
  };

  VsoshlbLb() = delete;
  /**
  explicit VsoshlbLb(
      const VsoshlbConfig& config,
      std::unique_ptr<BaseBpfAdapter>&& bpfAdapter);

  ~VsoshlbLb();

  /**
  void loadBpfProgs();

  /**
  bool reloadBalancerProg(
      const std::string& path,
      std::optional<VsoshlbConfig> config = std::nullopt);

  /**
  void attachBpfProgs();

  /**
  bool changeMac(const std::vector<uint8_t> newMac);

  /**
  std::vector<uint8_t> getMac();

  /**
  std::map<int, uint32_t> getIndexOfNetworkInterfaces();

  /**
  bool addVip(const VipKey& vip, const uint32_t flags = 0);

  /**
  bool addHcKey(const VipKey& hcKey);

  /**
  bool delVip(const VipKey& vip);

  /**
  bool delHcKey(const VipKey& hcKey);

  /**
  std::vector<VipKey> getAllVips();

  /**
  bool modifyVip(const VipKey& vip, uint32_t flag, bool set = true);

  /**
  bool changeHashFunctionForVip(const VipKey& vip, HashFunction func);

  /**
  uint32_t getVipFlags(const VipKey& vip);

  /**
  bool addRealForVip(const NewReal& real, const VipKey& vip);

  /**
  bool delRealForVip(const NewReal& real, const VipKey& vip);

  /**
  bool modifyReal(const std::string& real, uint8_t flags, bool set = true);

  /**
  bool modifyRealsForVip(
      const ModifyAction action,
      const std::vector<NewReal>& reals,
      const VipKey& vip);

  /**
  std::vector<NewReal> getRealsForVip(const VipKey& vip);

  /**
  int64_t getIndexForReal(const std::string& real);

  /**
  void modifyQuicRealsMapping(
      const ModifyAction action,
      const std::vector<QuicReal>& reals);

  /**
  std::vector<QuicReal> getQuicRealsMapping();

  /**
  int addSrcRoutingRule(
      const std::vector<std::string>& srcs,
      const std::string& dst);

  /**
  int addSrcRoutingRule(
      const std::vector<folly::CIDRNetwork>& srcs,
      const std::string& dst);

  /**
  bool delSrcRoutingRule(const std::vector<std::string>& srcs);

  /**
  bool delSrcRoutingRule(const std::vector<folly::CIDRNetwork>& srcs);

  /**
  bool addInlineDecapDst(const std::string& dst);

  /**
  bool delInlineDecapDst(const std::string& dst);

  /**
  std::vector<std::string> getInlineDecapDst();

  /**
  bool clearAllSrcRoutingRules();

  /**
  std::unordered_map<std::string, std::string> getSrcRoutingRule();

  /**
  std::unordered_map<folly::CIDRNetwork, std::string> getSrcRoutingRuleCidr();

  /**
  const std::unordered_map<folly::CIDRNetwork, uint32_t>& getSrcRoutingMap() {
    return lpmSrcMapping_;
  }

  /**
  const std::unordered_map<uint32_t, std::string> getNumToRealMap();

  /**
  uint32_t getSrcRoutingRuleSize();

  /**
  lb_stats getStatsForVip(const VipKey& vip);

  /**
  lb_stats getDecapStatsForVip(const VipKey& vip);

  /**
  uint64_t getPacketsProcessedForHcKey(const VipKey& hcKey);

  /**
  lb_stats getLruStats();

  /**
  lb_stats getLruMissStats();

  /*
    @ return true if vip lru miss logging succeed else return false
    helper fucntion to start logging of VipLruMissStats
  bool logVipLruMissStats(VipKey& vip);

  /*
    @return lru miss stats for the vip logged via logVipLruMissStats
    key - real IP address, value - packet count
  std::unordered_map<std::string, int32_t> getVipLruMissStats(VipKey& vip);

  /**
  lb_stats getLruFallbackStats();

  /**
  lb_stats getIcmpTooBigStats();

  /**
  lb_stats getChDropStats();

  /**
  lb_tpr_packets_stats getTcpServerIdRoutingStats();

  /**
  lb_stable_rt_packets_stats getUdpStableRoutingStats();

  /**
  lb_stats getSrcRoutingStats();

  /**
  lb_stats getInlineDecapStats();

  /**
  lb_stats getGlobalLruStats();

  /**
  lb_stats getDecapStats();

  /**
  lb_stats getQuicIcmpStats();

  /**
  lb_stats getIcmpPtbV6Stats();

  /**
  lb_stats getIcmpPtbV4Stats();

  /**
  lb_stats getRealStats(uint32_t index);

  /**
  std::vector<int64_t> getPerCorePacketsStats();

  /**
  bool addHealthcheckerDst(const uint32_t somark, const std::string& dst);

  /**
  bool delHealthcheckerDst(const uint32_t somark);

  /**
  std::unordered_map<uint32_t, std::string> getHealthcheckersDst();

  /**
  int getVsoshlbProgFd() {
    return bpfAdapter_->getProgFdByName(kBalancerProgName.toString());
  }

  /**
  int getHealthcheckerProgFd() {
    return bpfAdapter_->getProgFdByName(kHealthcheckerProgName.toString());
  }

  /**
  bool stopVsoshlbMonitor();

  /**
  bool restartVsoshlbMonitor(
      uint32_t limit,
      std::optional<PcapStorageFormat> storage = std::nullopt);

  /**
  std::unique_ptr<folly::IOBuf> getVsoshlbMonitorEventBuffer(
      monitoring::EventId event);

  /**
  VsoshlbMonitorStats getVsoshlbMonitorStats();

  /**
  VsoshlbLbStats getVsoshlbLbStats() {
    return lbStats_;
  }

  /**
  HealthCheckProgStats getStatsForHealthCheckProgram();

  /**
  VsoshlbBpfMapStats getBpfMapStats(const std::string& map);

  /*

  std::string simulatePacket(const std::string& inPacket);

  /**
  const std::string getRealForFlow(const VsoshlbFlow& flow);

  struct LruEntry {
    std::string realAddress;
    uint32_t realPos{0};
    uint64_t atime{0};
    int64_t atime_delta_sec{0};
    std::string sourceMap;
    std::string srcAddress;
    uint16_t srcPort{0};
  };
  struct LruEntries {
    std::string error;
    std::vector<LruEntry> entries;
  };

  // VIP part from struct flow_key
  struct LruVipKey {
    union {
      uint32_t dst;
      uint32_t dstv6[4];
    };
    uint16_t port;
    uint8_t proto;

    bool operator==(const LruVipKey& other) const {
      return (proto == other.proto) && (port == other.port) &&
          (dstv6[0] == other.dstv6[0]) && (dstv6[1] == other.dstv6[1]) &&
          (dstv6[2] == other.dstv6[2]) && (dstv6[3] == other.dstv6[3]);
    }
  };
  struct LruVipKeyHash {
    std::size_t operator()(const LruVipKey& k) const {
      std::size_t h = 0;
      h = folly::hash::hash_combine(h, k.proto);
      h = folly::hash::hash_combine(h, k.port);
      for (int i = 0; i < 4; i++) {
        h = folly::hash::hash_combine(h, k.dstv6[i]);
      }
      return h;
    }
  };

  struct VipLruStats {
    std::string vip;
    int count{0};
    int staleRealsCount{0};
    int atimeZeroCount{0};
    int atimeUnder30secCount{0};
    int atime30to60secCount{0};
    int atimeOver60secCount{0};

    void add(const VipLruStats& other) {
      count += other.count;
      staleRealsCount += other.staleRealsCount;
      atimeZeroCount += other.atimeZeroCount;
      atimeUnder30secCount += other.atimeUnder30secCount;
      atime30to60secCount += other.atime30to60secCount;
      atimeOver60secCount += other.atimeOver60secCount;
    }
  };
  struct LruStatsResponse {
    std::unordered_map<LruVipKey, VipLruStats, LruVipKeyHash> perVipStats;
    std::string error;
  };

  /**
  LruEntries
  searchLru(const VipKey& dstVip, const std::string& srcIp, uint16_t srcPort);
  LruEntries listLru(const VipKey& dstVip, int limit);
  LruStatsResponse analyzeLru();

  /*
  std::vector<std::string>
  deleteLru(const VipKey& dstVip, const std::string& srcIp, uint16_t srcPort);

  struct PurgeResponse {
    int deletedCount{0};
    std::string error;
  };
  PurgeResponse purgeVipLru(const VipKey& dstVip);

  /**
  bool addSrcIpForPcktEncap(const folly::IPAddress& src);

  /**
  std::shared_ptr<VsoshlbMonitor> getVsoshlbMonitor() {
    return monitor_;
  }

  /**
  bool hasFeature(VsoshlbFeatureEnum feature);

  /**
  bool installFeature(
      VsoshlbFeatureEnum feature,
      const std::string& prog_path = "");

  /**
  bool removeFeature(
      VsoshlbFeatureEnum feature,
      const std::string& prog_path = "");

  /**
  void setRealsIdCallback(RealsIdCallback* callback);

  /**
  void unsetRealsIdCallback();

  /**
  std::vector<int> getGlobalLruMapsFds();

  /**
  int getBpfMapFdByName(const std::string& mapName) {
    return bpfAdapter_->getMapFdByName(mapName);
  }

  /**
  lb_quic_packets_stats getLbQuicPacketsStats();

  /**
  lb_tpr_packets_stats getLbTprPacketsStats();

  /**
  lb_stats getSidRoutingStatsForVip(const VipKey& vip);

  /**
  void invalidateServerIds(const std::vector<int32_t>& serverIds);

  /**
  void revalidateServerIds(const std::vector<QuicReal>& quicReals);

 private:
  /**
  bool updateVipMap(
      const ModifyAction action,
      const VipKey& vip,
      vip_meta* meta = nullptr);

  /**
  bool updateHcKeyMap(
      const ModifyAction action,
      const VipKey& hcKey,
      uint32_t hcKeyId = 0);

  /**
  bool
  updateRealsMap(const folly::IPAddress& real, uint32_t num, uint8_t flags = 0);

  /**
  lb_stats getLbStats(uint32_t position, const std::string& map = "stats");

  /**
  vip_definition vipKeyToVipDefinition(const VipKey& vipKey);

  /**
  void decreaseRefCountForReal(const folly::IPAddress& real);

  /**
  uint32_t increaseRefCountForReal(
      const folly::IPAddress& real,
      uint8_t flags = 0);

  /**
  void initialSanityChecking(bool flowDebug = false, bool globalLru = false);

  /**
  void initLrus(bool flowDebug = false, bool globalLru = false);

  /**
  void attachLrus(bool flowDebug = false, bool globalLru = false);

  /**
  void initGlobalLruMapForCore(int core, int size, int flags, int numaNode);

  /**
  void initGlobalLruPrototypeMap();

  /**
  void attachGlobalLru(int core);

  /**
  void startIntrospectionRoutines();

  /**
  int createLruMap(
      int size = kFallbackLruSize,
      int flags = kMapNoFlags,
      int numaNode = kNoNuma,
      int cpu = 0);

  /**
  int createFlowDebugLru(
      int size = kFallbackLruSize,
      int flags = kMapNoFlags,
      int numaNode = kNoNuma);

  /**
  void initFlowDebugMapForCore(int core, int size, int flags, int numaNode);

  /**
  void initFlowDebugPrototypeMap();

  /**
  void attachFlowDebugLru(int core);

  /**
  void featureDiscovering();

  /**
  AddressType validateAddress(
      const std::string& addr,
      bool allowNetAddr = false);

  /**
  bool modifyLpmSrcRule(
      ModifyAction action,
      const folly::CIDRNetwork& src,
      uint32_t rnum);

  /**
  bool modifyLpmMap(
      const std::string& lpmMapNamePrefix,
      ModifyAction action,
      const folly::CIDRNetwork& addr,
      void* value);

  /**
  bool modifyDecapDst(
      ModifyAction action,
      const folly::IPAddress& dst,
      const uint32_t flags = 0);

  /**
  bool changeVsoshlbMonitorForwardingState(VsoshlbMonitorState state);

  /*
  void setupGueEnvironment();

  /*
  void setupHcEnvironment();

  /**
  void enableRecirculation();

  /**
  void programHashRing(
      const std::vector<RealPos>& chPositions,
      const uint32_t vipNum);

  bool initSimulator();

  folly::Expected<VsoshlbLb::LruEntry, std::string> lookupLruMap(
      int mapFd,
      flow_key& key);
  bool listLruMap(
      int mapFd,
      const std::string& sourceMap,
      flow_key& filterKey,
      bool isIPv6,
      int limit,
      LruEntries& lruEntries);
  void analyzeLruMap(int mapFd, LruStatsResponse& lruStats);

  PurgeResponse purgeVipLruMap(int mapFd, const flow_key& key);

  static std::optional<flow_key> flowKeyFromParams(
      const VipKey& dstVip,
      const std::string& srcIp,
      uint16_t srcPort);

  void fillAtimeDelta(LruEntries& entries);

  /**
  VsoshlbConfig config_;

  /**
  std::unique_ptr<BaseBpfAdapter> bpfAdapter_;

  /**
  std::shared_ptr<VsoshlbMonitor> monitor_{nullptr};

  /**
  std::deque<uint32_t> vipNums_;
  std::deque<uint32_t> realNums_;
  std::deque<uint32_t> hcKeyNums_;

  /**
  std::vector<ctl_value> ctlValues_;

  /**
  std::unordered_map<uint32_t, folly::IPAddress> hcReals_;


  /**
  std::unordered_map<uint32_t, folly::IPAddress> quicMapping_;
  /**
  folly::F14FastMap<uint32_t, folly::IPAddress> numToReals_;

  std::unordered_map<VipKey, Vip, VipKeyHasher> vips_;

  std::optional<VipKey> lruMissStatsVip_;

  /**
  std::unordered_map<VipKey, uint32_t, VipKeyHasher> hckeys_;

  /**
  std::unordered_map<folly::CIDRNetwork, uint32_t> lpmSrcMapping_;

  /**
  std::unordered_set<folly::IPAddress> decapDsts_;

  /**
  bool standalone_;

  /**
  int rootMapFd_;

  /**
  bool progsLoaded_{false};

  /**
  bool progsAttached_{false};

  /**
  struct VsoshlbFeatures features_;
  /**
  std::vector<int32_t> forwardingCores_;

  /**
  std::vector<int32_t> numaNodes_;

  /**
  std::vector<int> lruMapsFd_;

  /**
  std::vector<int> flowDebugMapsFd_;

  /**
  std::vector<int> globalLruMapsFd_;

  /**
  int globalLruFallbackFd_{-1};

  /**
  VsoshlbLbStats lbStats_;

  /**
  bool introspectionStarted_{false};

  /**
  bool progsReloaded_{false};

  /**
  RealsIdCallback* realsIdCallback_{nullptr};

  /**

  std::unordered_set<uint64_t> invalidServerIds_;

  std::unique_ptr<VsoshlbSimulator> simulator_;
};

} // namespace vsoshlb

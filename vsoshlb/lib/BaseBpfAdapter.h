
#pragma once
#include <folly/Function.h>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <linux/perf_event.h>
}

namespace vsoshlb {

constexpr int TC_INGRESS = 0xfffffff2;
constexpr int TC_EGRESS = 0xfffffff3;

// from bpf.h (list could be outdated)
constexpr unsigned int kBpfMapTypeUnspec = 0;
constexpr unsigned int kBpfMapTypeHash = 1;
constexpr unsigned int kBpfMapTypeArray = 2;
constexpr unsigned int kBpfMapTypeProgArray = 3;
constexpr unsigned int kBpfMapTypePerfEventArray = 4;
constexpr unsigned int kBpfMapTypePercpuHash = 5;
constexpr unsigned int kBpfMapTypePercpuArray = 6;
constexpr unsigned int kBpfMapTypeStackTrace = 7;
constexpr unsigned int kBpfMapTypeCgroupArray = 8;
constexpr unsigned int kBpfMapTypeLruHash = 9;
constexpr unsigned int kBpfMapTypeLruPercpuHash = 10;
constexpr unsigned int kBpfMapTypeLpmTrie = 11;
constexpr unsigned int kBpfMapTypeArrayOfMaps = 12;
constexpr unsigned int kBpfMapTypeHashOfMaps = 13;

class BaseBpfAdapter {
 public:
  BaseBpfAdapter(bool set_limits, bool enableBatchOpsIfSupported);

  virtual ~BaseBpfAdapter() {}

  /**
  virtual int loadBpfProg(
      const std::string& bpf_prog,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC,
      bool use_names = false) = 0;

  /**
  virtual int reloadBpfProg(
      const std::string& bpf_prog,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC) = 0;

  /**
  virtual int loadBpfProg(
      const char* buf,
      int buf_size,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC,
      bool use_names = false,
      const char* objName = "buffer") = 0;

  /**
  virtual int getMapFdByName(const std::string& name) = 0;

  /**
  virtual bool isMapInProg(
      const std::string& progName,
      const std::string& name) = 0;

  /**
  static int createBpfMap(
      unsigned int type,
      unsigned int key_size,
      unsigned int value_size,
      unsigned int max_entries,
      unsigned int map_flags,
      int numa_node = -1);

  /**
  static int createNamedBpfMap(
      const std::string& name,
      unsigned int type,
      unsigned int key_size,
      unsigned int value_size,
      unsigned int max_entries,
      unsigned int map_flags,
      int numa_node = -1);

  /**
  virtual int setInnerMapPrototype(const std::string& name, int map_fd) = 0;

  /**
  virtual int getProgFdByName(const std::string& name) = 0;

  /**
  static int pinBpfObject(int fd, const std::string& path);

  /**
  static int getPinnedBpfObject(const std::string& path);

  /**
  static int getBpfMapInfo(int fd, struct bpf_map_info* info);

  /**
  virtual int getBpfMapMaxSize(const std::string& name);

  /**
  virtual int getBpfMapUsedSize(const std::string& name);

  /**
  static int getInterfaceIndex(const std::string& interface_name);

  /**
  static int attachBpfProgToTc(
      const int prog_fd,
      const std::string& interface_name,
      const int direction,
      const std::string& bpf_name,
      const uint32_t priority = 2307);

  /**
  static int attachXdpProg(
      const int prog_fd,
      const std::string& interface_name,
      const uint32_t flags = 0);

  /**
  static int detachXdpProg(
      const std::string& interface_name,
      const uint32_t flags = 0);

  /**
  static int detachXdpProg(const int ifindex, const uint32_t flags = 0);

  /**
  static int bpfUpdateMap(
      int map_fd,
      void* key,
      void* value,
      unsigned long long flags = 0);

  /**
  int bpfUpdateMapBatch(int map_fd, void* keys, void* values, uint32_t count);

  /**
  static int bpfMapLookupElement(
      int map_fd,
      void* key,
      void* value,
      unsigned long long flags = 0);

  /**
  static int bpfMapDeleteElement(int map_fd, void* key);

  /**
  static int bpfMapGetNextKey(int map_fd, void* key, void* next_key);

  /**
  static int bpfMapGetFdOfInnerMap(int outer_map_fd, void* key);

  /**
  static int bpfMapGetFdById(uint32_t map_id);

  /**
  static int bpfProgGetFdById(uint32_t prog_id);

  /**
  static int addTcBpfFilter(
      const int prog_fd,
      const unsigned int ifindex,
      const std::string& bpf_name,
      const uint32_t priority,
      const int direction = TC_INGRESS,
      const uint32_t handle = 0);

  /**
  static int modifyXdpProg(
      const int prog_fd,
      const unsigned int ifindex,
      const uint32_t flags = 0);

  /**
  static int replaceTcBpfFilter(
      const int prog_fd,
      const unsigned int ifindex,
      const std::string& bpf_name,
      const uint32_t priority,
      const int direction = TC_INGRESS,
      const uint32_t handle = 0);

  /**
  static int deleteTcBpfFilter(
      const int prog_fd,
      const unsigned int ifindex,
      const std::string& bpf_name,
      const uint32_t priority,
      const int direction = TC_INGRESS,
      const uint32_t handle = 0);

  /**
  static int testXdpProg(
      const int prog_fd,
      const int repeat,
      void* data,
      uint32_t data_size,
      void* data_out,
      uint32_t* size_out = nullptr,
      uint32_t* retval = nullptr,
      uint32_t* duration = nullptr,
      void* ctx_in = nullptr,
      uint32_t ctx_size_in = 0,
      void* ctx_out = nullptr,
      uint32_t* ctx_size_out = nullptr);

  /**
  static int attachCgroupProg(
      int prog_fd,
      const std::string& cgroup,
      enum bpf_attach_type type,
      unsigned int flags);

  /**
  static int detachCgroupProg(
      const std::string& cgroup,
      enum bpf_attach_type type);

  /**
  static int detachCgroupProgByPrefix(
      const std::string& cgroup,
      enum bpf_attach_type type,
      const std::string& progPrefix);

  /**
  static int detachCgroupProg(
      int prog_fd,
      const std::string& cgroup,
      enum bpf_attach_type type);

  /**
  static std::vector<uint32_t> getCgroupProgsIds(
      const std::string& cgroup,
      enum bpf_attach_type type);

  /**
  static int getBpfProgInfo(int progFd, ::bpf_prog_info& info);

  /**
  static bpf_prog_info getBpfProgInfo(int progFd);

  /**
  virtual int updateSharedMap(const std::string& name, int fd) = 0;

  /**
  static int getPossibleCpus();

  /**
  static bool perfEventUnmmap(struct perf_event_mmap_page** header, int pages);

  /**
  static bool openPerfEvent(
      int cpu,
      int map_fd,
      int wakeUpNumEvents,
      int pages,
      struct perf_event_mmap_page** header,
      int& event_fd);

  /**
  static void handlePerfEvent(
      folly::Function<void(const char* data, size_t size)> eventHandler,
      struct perf_event_mmap_page* header,
      std::string& buffer,
      int pageSize,
      int pages,
      int cpu);

  /**
  static bool isMapInBpfObject(
      const std::string& path,
      const std::string& mapName);

  /**
  static int64_t getKtimeNs();

 protected:
  /**
  static int modifyTcBpfFilter(
      const int cmd,
      const unsigned int flags,
      const uint32_t priority,
      const int prog_fd,
      const unsigned int ifindex,
      const std::string& bpf_name,
      const int direction = TC_INGRESS,
      const uint32_t handle = 0);

  /**
  static int addClsActQD(const unsigned int ifindex);

  /**
  static int genericAttachBpfProgToTc(
      const int prog_fd,
      const unsigned int ifindex,
      const std::string& bpf_name,
      uint32_t priority,
      const int direction = TC_INGRESS,
      const uint32_t handle = 0);

  /**
  static int getDirFd(const std::string& path);

  /**
  static struct perf_event_mmap_page* perfEventMmap(int event_fd, int pages);

  /**
  bool batchOpsAreSupported();

  // enable/disable the print of dbg messages from libbpf
  void setPrintBpfDbgFlag(bool flag);

  /**
  bool batchOpsEnabled_{false};
};

} // namespace vsoshlb

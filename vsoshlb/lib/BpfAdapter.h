
#pragma once
#include <folly/Function.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "BaseBpfAdapter.h"
#include "BpfLoader.h"

extern "C" {
#include <bpf/bpf.h>
#include <linux/perf_event.h>
}

namespace vsoshlb {

class BpfAdapter : public BaseBpfAdapter {
 public:
  explicit BpfAdapter(
      bool set_limits = true,
      bool enableBatchOpsIfSupported = false);

  // BpfAdapter is not thread safe.  Discourage unsafe use by disabling copy
  // construction/assignment.
  BpfAdapter(BpfAdapter const&) = delete;
  BpfAdapter& operator=(BpfAdapter const&) = delete;

  int loadBpfProg(
      const std::string& bpf_prog,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC,
      bool use_names = false) override;

  int reloadBpfProg(
      const std::string& bpf_prog,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC) override;

  int loadBpfProg(
      const char* buf,
      int buf_size,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC,
      bool use_names = false,
      const char* objName = "buffer") override;

  int getMapFdByName(const std::string& name) override;

  bool isMapInProg(const std::string& progName, const std::string& name)
      override;

  int setInnerMapPrototype(const std::string& name, int map_fd) override;

  int getProgFdByName(const std::string& name) override;

  int updateSharedMap(const std::string& name, int fd) override;

 private:
  BpfLoader loader_;
};

} // namespace vsoshlb


#include <glog/logging.h>
#include <cstdint>

#include "vsoshlb/decap/XdpDecap.h"

namespace vsoshlb {

XdpDecap::XdpDecap(const XdpDecapConfig& config) : config_(config) {
  if (!config_.mapPath.empty()) {
    isStandalone_ = false;
  } else {
    // sanity checking that interface w/ specified name exists
    // so we wont need to do it later every time when we would want to access it
    auto ifindex = bpfAdapter_.getInterfaceIndex(config_.interface);
    if (!ifindex) {
      LOG(FATAL) << "Can not resolve to ifindex interface: "
                 << config_.interface;
      return;
    }
  }
}

XdpDecap::~XdpDecap() {
  if (isAttached_) {
    if (!config_.detachOnExit) {
      LOG(INFO) << "Exiting w/o detach";
      return;
    }
    if (isStandalone_) {
      auto res = bpfAdapter_.detachXdpProg(config_.interface);
      if (res) {
        LOG(ERROR) << "Was not able to detach XdpDecap";
      }
    } else {
      auto prog_fd = bpfAdapter_.getPinnedBpfObject(config_.mapPath);
      if (prog_fd >= 0) {
        auto res = bpfAdapter_.bpfMapDeleteElement(prog_fd, &config_.progPos);
        if (res) {
          LOG(ERROR) << "Was not able to detach XdpDecap from prog array";
        }
      }
    }
  }
}

void XdpDecap::loadXdpDecap() {
  if (isLoaded_) {
    LOG(ERROR) << "Trying to load alrady attached XdpDecap";
    return;
  }
  auto res = bpfAdapter_.loadBpfProg(config_.progPath);
  if (res) {
    LOG(FATAL) << "Was not able to load XdpDecap program from "
               << config_.progPath;
    // LOG(FATAL) would terminate. return for readability
    return;
  }
  // sanity checking
  // check that program w/ expected name has been loaded
  if (bpfAdapter_.getProgFdByName("xdpdecap") < 0) {
    LOG(FATAL) << "Was not able to find xdp prog w/ name xdpdecap in "
               << config_.progPath;
    return;
  }

  if (bpfAdapter_.getMapFdByName("decap_counters") < 0) {
    LOG(FATAL) << "Was not able to find bpf map w/ name stats in "
               << config_.progPath;
    return;
  }
  isLoaded_ = true;
}

void XdpDecap::attachXdpDecap() {
  if (!isLoaded_ || isAttached_) {
    LOG(FATAL) << "trying to attach non loaded or already attached "
               << "XdpDecap program";
    return;
  }
  auto prog_fd = bpfAdapter_.getProgFdByName("xdpdecap");
  if (isStandalone_) {
    if (bpfAdapter_.attachXdpProg(prog_fd, config_.interface)) {
      LOG(FATAL) << "Was not able to attach XdpDecap to interface "
                 << config_.interface;
      return;
    }
  } else {
    auto map_fd = bpfAdapter_.getPinnedBpfObject(config_.mapPath);
    if (map_fd < 0) {
      LOG(FATAL) << "Was not able to get a fd of pinned bpf map "
                 << config_.mapPath;
      return;
    }
    if (bpfAdapter_.bpfUpdateMap(map_fd, &config_.progPos, &prog_fd)) {
      LOG(FATAL) << "Was not able to update pinned bpf map " << config_.mapPath
                 << " with elem on position " << config_.progPos;
      return;
    }
  }
  isAttached_ = true;
}

decap_stats XdpDecap::getXdpDecapStats() {
  struct decap_stats stats = {};
  uint32_t key = 0;

  if (!isLoaded_) {
    LOG(ERROR) << "Trying to get stats for not loaded XdpDecap program";
    return stats;
  }
  auto nr_cpus = bpfAdapter_.getPossibleCpus();
  if (nr_cpus < 0) {
    LOG(ERROR) << "Can not figure out number of online cpus";
    return stats;
  }

  struct decap_stats percpu_stats[nr_cpus];

  if (!bpfAdapter_.bpfMapLookupElement(
          bpfAdapter_.getMapFdByName("decap_counters"), &key, &percpu_stats)) {
    for (auto& stat : percpu_stats) {
      stats.decap_v4 += stat.decap_v4;
      stats.decap_v6 += stat.decap_v6;
      stats.total += stat.total;
      stats.tpr_misrouted += stat.tpr_misrouted;
      stats.tpr_total += stat.tpr_total;
    }
  } else {
    LOG(ERROR) << "Error while trying to get decap stats";
  }
  return stats;
}

void XdpDecap::setSeverId(int id) {
  auto map_fd = bpfAdapter_.getMapFdByName("tpr_server_id");
  uint32_t key = 0;
  uint32_t value = id;
  if (bpfAdapter_.bpfUpdateMap(map_fd, &key, &value)) {
    LOG(FATAL) << "Was not able to update pinned bpf map " << config_.mapPath
               << " with elem on position " << config_.progPos;
    return;
  }
}

} // namespace vsoshlb


#pragma once

#include <folly/Expected.h>
#include <glog/logging.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <vsoshlb/tpr/TPRStatsPoller.h>
#include <vsoshlb/tpr/TPRTypes.h>
#include <vsoshlb/tpr/bpf_util/BpfSkeleton.h>

#ifdef VSOSHLB_CMAKE_BUILD
#include "tpr.skel.h" // @manual
#else
#include <vsoshlb/tpr/bpf/tpr.skel.h>
#endif

namespace vsoshlb_tpr {

class TcpPktRouter {
 public:
  explicit TcpPktRouter(
      RunningMode mode,
      const std::string& cgroupPath,
      bool kdeEnabled,
      std::optional<uint32_t> serverPort = std::nullopt);

  virtual ~TcpPktRouter();

  /**
  folly::Expected<folly::Unit, std::system_error> init(bool pollStats = false);

  /**
  folly::Expected<folly::Unit, std::system_error> shutdown();

  /**
  bool isInitialized() const noexcept {
    return isInitialized_;
  }

  /**
  bool setServerIdV6(uint32_t id);

  /**
  uint32_t getServerIdV6() const noexcept {
    return v6Id_;
  }

  RunningMode getMode() {
    return mode_;
  }

  /**
  folly::Expected<tcp_router_stats, std::system_error> collectTPRStats();

  /**
  folly::Expected<int, std::system_error> getBpfProgramFd() noexcept;

  folly::Expected<uint32_t, std::system_error> getServerIdFromSkSidStoreMap(
      int socketFd) noexcept;

 protected:
  virtual std::unique_ptr<TPRStatsPoller> createStatsPoller(
      folly::EventBase* evb,
      int statsMapFd);

  RunningMode mode_;
  std::string cgroupPath_;

 private:
  folly::Expected<folly::Unit, std::system_error> updateServerInfo() noexcept;

  bool isInitialized_{false};
  uint32_t v6Id_;
  bool kdeEnabled_;
  std::optional<uint32_t> serverPort_;
  /**
  std::unique_ptr<TPRStatsPoller> statsPoller_;
  BpfSkeleton<tpr_bpf> skel_;
  int progFd_{-1};
};

} // namespace vsoshlb_tpr

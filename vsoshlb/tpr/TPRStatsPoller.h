
#pragma once

#include <folly/Expected.h>
#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/EventBase.h>
#include <vsoshlb/tpr/TPRTypes.h>
#include <atomic>
#include <memory>
#include <string>

namespace vsoshlb_tpr {

/**
class TPRStatsPoller : public folly::AsyncTimeout {
 public:
  explicit TPRStatsPoller(
      RunningMode mode,
      folly::EventBase* evb,
      int statsMapFd);

  ~TPRStatsPoller() override;

  void timeoutExpired() noexcept override;

  /**
  void shutdown();

  /**
  folly::Expected<folly::Unit, std::system_error> runStatsPoller();

  /**
  folly::Expected<tcp_router_stats, std::system_error> collectTPRStats(
      int numCpus);

  /**
  static folly::Expected<int, std::system_error> getCpuCount();

 protected:
  virtual void incrementCounter(const std::string& name);
  virtual void setCounter(const std::string& name, int64_t val);
  virtual void setStatsCounters(const tcp_router_stats& stats);

 private:
  /**
  void updateStatsPeriodically();

 protected:
  RunningMode mode_;

 private:
  /**
  folly::EventBase* evb_;

  /**
  std::string statsPrefix_ = "";

  /**
  int statsMapFd_{-1};

  /**
  int numCpus_{0};
  /** flag to indicate that the server is in the shutdown phase
  std::atomic<bool> shutdown_{false};
};

} // namespace vsoshlb_tpr

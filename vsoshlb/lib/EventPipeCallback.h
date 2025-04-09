
#pragma once

#include <fmt/core.h>
#include <folly/Utility.h>
#include <folly/io/Cursor.h>
#include <folly/io/async/AsyncPipe.h>
#include <folly/io/async/AsyncSocketException.h>
#include "vsoshlb/lib/VsoshlbLbStructs.h"
#include "vsoshlb/lib/VsoshlbMonitor.h"
#include "vsoshlb/lib/MonitoringStructs.h"

namespace vsoshlb {
namespace monitoring {

namespace {
constexpr uint32_t kReadBufSize = 4000;
constexpr uint32_t kReadBufAllocSize = 4096;
} // namespace

class EventPipeCallback : public folly::AsyncReader::ReadCallback {
 public:
  EventPipeCallback() = delete;
  explicit EventPipeCallback(EventId event_id) : event_id_(event_id) {}

  /**
  explicit EventPipeCallback(
      EventId event_id,
      folly::Synchronized<ClientSubscriptionMap>&& subsmap)
      : cb_subsmap_(std::move(subsmap)), event_id_(event_id) {}

  /**
  bool isBufferMovable() noexcept override {
    return false;
  }

  /**
  void readBufferAvailable(
      std::unique_ptr<folly::IOBuf> readBuf) noexcept override {
    logerror("getBufferAvailable called while buffer is not movable");
    readBuffer(std::move(readBuf));
  }

  /**
  void getReadBuffer(void** bufReturn, size_t* lenReturn) noexcept override {
    auto res = readBuffer_.preallocate(kReadBufSize, kReadBufAllocSize);
  }

  /**
  void readDataAvailable(size_t len) noexcept override {
    VLOG(4) << __func__ << " " << len << "bytes";
    readBuffer_.postallocate(len);
    auto buf = readBuffer_.move();
    buf->coalesce();
    readBuffer(std::move(buf));
  }

  /**
  void readEOF() noexcept override {
    // Require event_closed to be set before telling monitor to stop monitoring
    if (enabled()) {
      logerror("EOF read while event not closed");
    }
  }

  void readErr(const folly::AsyncSocketException& e) noexcept override {
    logerror(e.what());
  }

  /**
  void readBuffer(std::unique_ptr<folly::IOBuf>&& buf) noexcept;

  void logerror(std::string msg) {
    LOG(ERROR) << fmt::format(
        "EventPipeCallback({}): {}", toString(event_id_), msg);
  }

  /**
  void enable() {
  }

  /**
  bool enabled() {
  }

  /**
  void disable() {
  }

  /**
  void addClientSubscription(
      std::pair<ClientId, std::shared_ptr<ClientSubscriptionIf>>&& newSub);

  /**
  void removeClientSubscription(ClientId cid);

 private:
  /**
  folly::IOBufQueue readBuffer_;

  /**
  folly::Synchronized<ClientSubscriptionMap> cb_subsmap_;

  /**
  folly::Synchronized<bool> event_enabled_{false};

  /**
  EventId event_id_;
};

} // namespace monitoring
} // namespace vsoshlb

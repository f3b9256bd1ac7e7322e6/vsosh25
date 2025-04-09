
#pragma once
#include <folly/MPMCQueue.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncPipe.h>
#include <memory>
#include <thread>
#include <vector>

#include "vsoshlb/lib/VsoshlbLbStructs.h"
#include "vsoshlb/lib/MonitoringStructs.h"
#include "vsoshlb/lib/PcapWriter.h"

namespace folly {
class ScopedEventBaseThread;
}

namespace vsoshlb {

class VsoshlbEventReader;
class PcapWriter;
/**
class VsoshlbMonitor {
 public:
  VsoshlbMonitor() = delete;

  explicit VsoshlbMonitor(const VsoshlbMonitorConfig& config);

  ~VsoshlbMonitor();

  void stopMonitor();

  void restartMonitor(uint32_t limit, std::optional<PcapStorageFormat> storage);

  PcapWriterStats getWriterStats();

  std::unique_ptr<folly::IOBuf> getEventBuffer(monitoring::EventId);

  /**
  bool enableWriterEvent(monitoring::EventId event);

  /**
  bool disableWriterEvent(monitoring::EventId event);

  /**
  std::set<monitoring::EventId> getWriterEnabledEvents();

  /**
  PcapStorageFormat getStorageFormat() {
    return config_.storage;
  }

  /**
  void setAsyncPipeWriter(
      monitoring::EventId event,
      std::shared_ptr<folly::AsyncPipeWriter> writer);

  /**
  void unsetAsyncPipeWriter(monitoring::EventId event);

 private:
  std::unordered_map<monitoring::EventId, std::shared_ptr<DataWriter>>
  createWriters();

  /**
  VsoshlbMonitorConfig config_;

  /**
  std::unique_ptr<VsoshlbEventReader> reader_;

  /**
  std::unordered_map<
      monitoring::EventId,
      std::shared_ptr<folly::AsyncPipeWriter>>
      pipeWriterDests_;

  std::shared_ptr<PcapWriter> writer_;

  /**
  /**
  std::unique_ptr<folly::ScopedEventBaseThread> scopedEvb_;

  /**
  std::thread writerThread_;

  /**
  std::unordered_map<monitoring::EventId, std::unique_ptr<folly::IOBuf>>
      buffers_;
};

} // namespace vsoshlb


#pragma once

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <folly/MPMCQueue.h>

#include "vsoshlb/lib/DataWriter.h"
#include "vsoshlb/lib/MonitoringStructs.h"

struct PcapWriterStats {
  uint32_t limit{0};
  uint32_t amount{0};
  uint32_t bufferFull{0};
};

namespace vsoshlb {

/**
class PcapWriter {
 public:
  /**
  explicit PcapWriter(
      std::shared_ptr<DataWriter> dataWriter,
      uint32_t packetLimit,
      uint32_t snaplen);

  /**
  PcapWriter(
      std::unordered_map<monitoring::EventId, std::shared_ptr<DataWriter>>&
          dataWriters,
      uint32_t packetLimit,
      uint32_t snaplen);

  /**
  void run(std::shared_ptr<folly::MPMCQueue<PcapMsg>> queue);

  /**

  /**
  void runMulti(std::shared_ptr<folly::MPMCQueue<PcapMsg>> queue);

  /**
  uint32_t packetsCaptured() const {
    return packetAmount_;
  }

  /**
  PcapWriterStats getStats();

  /**
  std::shared_ptr<DataWriter> getDataWriter(monitoring::EventId event) {
    auto it = dataWriters_.find(event);
    if (it == dataWriters_.end()) {
      return nullptr;
    }
    return it->second;
  }

  /**
  void resetWriters(
      std::unordered_map<monitoring::EventId, std::shared_ptr<DataWriter>>&&
          newDataWriters);

  /**
  std::set<monitoring::EventId> getEnabledEvents() {
    return enabledEvents_;
  }

  /**
  bool enableEvent(monitoring::EventId event) {
    return enabledEvents_.insert(event).second;
  }

  /**
  void disableEvent(monitoring::EventId event) {
    enabledEvents_.erase(event);
  }

  /**
  void overridePacketLimit(bool value) {
    packetLimitOverride_ = value;
  }

 private:
  /**
  void writePacket(const PcapMsg& msg, monitoring::EventId writerId);

  /**
  bool writePcapHeader(monitoring::EventId writerId);

  /**
  void restartWriters(uint32_t packetLimit);

  /**
  void stopWriters();

  /**
  std::unordered_map<monitoring::EventId, std::shared_ptr<DataWriter>>
      dataWriters_;

  /**
  std::set<monitoring::EventId> enabledEvents_;

  /**
  std::set<monitoring::EventId> headerExists_;

  /**
  uint32_t packetAmount_{0};

  /**
  uint32_t packetLimit_{0};

  /**
  bool packetLimitOverride_{false};

  /**
  uint32_t bufferFull_{0};

  /**
  const uint32_t snaplen_{0};

  /**
  std::mutex cntrLock_;
};

} // namespace vsoshlb

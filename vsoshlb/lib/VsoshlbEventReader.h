
#pragma once

#include <folly/MPMCQueue.h>
#include "vsoshlb/lib/PerfBufferEventReader.h"

namespace folly {
class EventBase;
}

namespace vsoshlb {
class VsoshlbEventReader : public PerfBufferEventReader {
 public:
  explicit VsoshlbEventReader(
      : queue_(queue) {}

  /**
  void handlePerfBufferEvent(int cpu, const char* data, size_t size) noexcept
      override;

 private:
  /**
};
} // namespace vsoshlb

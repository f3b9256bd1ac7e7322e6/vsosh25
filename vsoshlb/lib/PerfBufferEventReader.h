
#pragma once
#include <bpf/libbpf.h>
#include <folly/io/async/EventHandler.h>

namespace vsoshlb {

/**
class PerfBufferEventReader {
 public:
  PerfBufferEventReader() = default;
  virtual ~PerfBufferEventReader();

  /**
  bool open(int bpfPerfMap, folly::EventBase* evb, size_t pageCount);

  /**
  virtual void
  handlePerfBufferEvent(int cpu, const char* data, size_t size) noexcept = 0;

  /**
  virtual void handlePerfBufferLoss(

 private:
  /**
  class CpuBufferHandler : public folly::EventHandler {
   public:
    /**
    CpuBufferHandler(
        folly::EventBase* evb,
        struct perf_buffer* pb,
        int fd,
        size_t idx);

    /**
    void handlerReady(uint16_t events) noexcept override;

   private:
    /**
    struct perf_buffer* pb_{nullptr};
    int bufFd_;
    size_t bufIdx_;
  };

  /**
  struct perf_buffer* pb_{nullptr};

  /**
  std::vector<std::unique_ptr<CpuBufferHandler>> cpuBufferHandlers_;
};

} // namespace vsoshlb

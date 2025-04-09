#pragma once

#include <folly/io/async/AsyncPipe.h>
#include <folly/io/async/AsyncSocketException.h>
#include <cstdint>
#include "vsoshlb/lib/DataWriter.h"

namespace vsoshlb {

class PipeWriteCallback : public folly::AsyncWriter::WriteCallback {
 public:
  void writeSuccess() noexcept override {
    event_writes_++;
  }

  void writeErr(size_t, const folly::AsyncSocketException& e) noexcept
      override {
    LOG(ERROR) << "PipeWriter error: " << e.what();
    event_errs_++;
  }

  void reset() {
    event_writes_ = 0;
    event_errs_ = 0;
  }

  uint32_t event_writes_{0};
  uint32_t event_errs_{0};
};

/**
class PipeWriter : public DataWriter {
 public:
  /**
  explicit PipeWriter();

  /**
  void writeData(const void* ptr, std::size_t size) override;

  /**
  void writeHeader(const void* ptr, std::size_t size) override;

  /**
    return true;
  }

  /**
  bool restart() override {
    VLOG(4) << "Retsarting pipe writer";
    enabled_ = true;
    return true;
  }

  /**
  bool stop() override {
    VLOG(4) << "Stopping pipe writer";
    enabled_ = false;
    return true;
  }

  /**
  void setWriterDestination(std::shared_ptr<folly::AsyncPipeWriter> pipeWriter);

  /**
  void unsetWriterDestination();

  /**
  uint32_t getWrites() {
    return writeCallback_.event_writes_;
  }

  /**
  uint32_t getErrs() {
    return writeCallback_.event_errs_;
  }

 private:
  /**
  std::shared_ptr<folly::AsyncPipeWriter> pipe_;

  /**
  bool enabled_{true};

  /**
  PipeWriteCallback writeCallback_;

  /**
  std::unique_ptr<folly::IOBuf> headerBuf_{nullptr};
};

} // namespace vsoshlb

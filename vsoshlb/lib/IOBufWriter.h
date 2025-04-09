#pragma once

#include <folly/io/IOBuf.h>
#include <memory>
#include <string>

#include "vsoshlb/lib/DataWriter.h"

namespace vsoshlb {

/**
class IOBufWriter : public DataWriter {
 public:
  /**
  explicit IOBufWriter(folly::IOBuf* iobuf);

  void writeData(const void* ptr, std::size_t size) override;

  bool available(std::size_t amount) override;

  bool restart() override;

  bool stop() override {
    return true;
  }

 private:
  folly::IOBuf* iobuf_;
};

} // namespace vsoshlb

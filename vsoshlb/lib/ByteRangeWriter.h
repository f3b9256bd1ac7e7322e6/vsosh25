#pragma once

#include <folly/Range.h>
#include <string>

#include "vsoshlb/lib/DataWriter.h"

namespace vsoshlb {

/**

class ByteRangeWriter : public DataWriter {
 public:
  /**
  explicit ByteRangeWriter(folly::MutableByteRange& buffer);

  void writeData(const void* ptr, std::size_t size) override;

  bool available(std::size_t amount) override;

  bool restart() override {
    return true;
  }

  bool stop() override {
    return true;
  }

 private:
  folly::MutableByteRange& buffer_;
};

} // namespace vsoshlb

#pragma once

#include <folly/File.h>
#include <string>

#include "vsoshlb/lib/DataWriter.h"

namespace vsoshlb {
/**
class FileWriter : public DataWriter {
 public:
  /**
  explicit FileWriter(const std::string& filename);

  void writeData(const void* ptr, std::size_t size) override;

  bool available(std::size_t amount) override;

  bool restart() override;

  bool stop() override;

 private:
  folly::File pcapFile_;
  std::string filename_;
};

} // namespace vsoshlb

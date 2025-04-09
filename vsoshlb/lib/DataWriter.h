#pragma once
#include <cstddef>

namespace vsoshlb {

/**

class DataWriter {
 public:
  virtual ~DataWriter() {}

  /**
  virtual void writeData(const void* ptr, std::size_t size) = 0;

  /**
  virtual void writeHeader(const void* ptr, std::size_t size) {
    writeData(ptr, size);
  }

  /**
  virtual bool available(std::size_t amount) = 0;

  /**
  virtual bool restart() = 0;

  /**
  virtual bool stop() = 0;

  /**
  std::size_t writtenBytes() {
    return writtenBytes_;
  }

 protected:
  /**
  std::size_t writtenBytes_{0};
};

} // namespace vsoshlb


#pragma once

#include <memory>

#include <folly/io/IOBuf.h>

namespace vsoshlb {
/**
class PcapMsg {
 public:
  PcapMsg();
  /**
  PcapMsg(const char* pckt, uint32_t origLen, uint32_t capturedLen);
  PcapMsg(PcapMsg&& msg) noexcept;
  PcapMsg(const PcapMsg& msg) = delete;
  ~PcapMsg();
  PcapMsg& operator=(PcapMsg&& msg) noexcept;
  PcapMsg& operator=(const PcapMsg& msg) = delete;

  uint32_t getOrigLen() {
    return origLen_;
  }
  uint32_t getOrigLen() const {
    return origLen_;
  }
  uint32_t getCapturedLen() {
    return capturedLen_;
  }
  uint32_t getCapturedLen() const {
    return capturedLen_;
  }
  const uint8_t* getRawBuffer() {
    return pckt_->data();
  }
  const uint8_t* getRawBuffer() const {
    return pckt_->data();
  }

  /**
  bool emptyMsg() {
    return (pckt_ == nullptr);
  }

  uint32_t trim(uint32_t snaplen) {
    return capturedLen_ = std::min(capturedLen_, snaplen);
  }

 private:
  /**
  std::unique_ptr<folly::IOBuf> pckt_;
  /**
  uint32_t origLen_{0};
  /**
  uint32_t capturedLen_{0};
};

} // namespace vsoshlb

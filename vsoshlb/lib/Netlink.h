
#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace vsoshlb {
class NetlinkMessage {
 public:
  /**
  static NetlinkMessage TC(
      unsigned seq,
      int cmd,
      unsigned flags,
      uint32_t priority,
      int prog_fd,
      unsigned ifindex,
      const std::string& bpf_name,
      int direction,
      const uint32_t handle = 0);

  /**
  static NetlinkMessage QD(unsigned ifindex);

  /**
  static NetlinkMessage
  XDP(unsigned seq, int prog_fd, unsigned ifindex, uint32_t flags);

  const uint8_t* data() const {
    return buf_.data();
  }

  size_t size() const {
    return buf_.size();
  }

  unsigned seq() const;

 private:
  NetlinkMessage();
  std::vector<uint8_t> buf_;
};
} // namespace vsoshlb

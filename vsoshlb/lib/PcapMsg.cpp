
#include "vsoshlb/lib/PcapMsg.h"

namespace vsoshlb {

PcapMsg::PcapMsg() {
  pckt_ = nullptr;
}

PcapMsg::PcapMsg(const char* pckt, uint32_t origLen, uint32_t capturedLen)
    : origLen_(origLen), capturedLen_(capturedLen) {
  if (pckt != nullptr) {
    pckt_ = folly::IOBuf::copyBuffer(pckt, capturedLen);
  }
}

PcapMsg::~PcapMsg() {}

PcapMsg& PcapMsg::operator=(PcapMsg&& msg) noexcept {
  pckt_ = std::move(msg.pckt_);
  origLen_ = msg.origLen_;
  capturedLen_ = msg.capturedLen_;
}

PcapMsg::PcapMsg(PcapMsg&& msg) noexcept
    : pckt_(std::move(msg.pckt_)),
      origLen_(msg.origLen_),
      capturedLen_(msg.capturedLen_) {}
} // namespace vsoshlb

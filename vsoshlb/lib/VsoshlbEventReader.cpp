#include "vsoshlb/lib/VsoshlbEventReader.h"

#include "vsoshlb/lib/BalancerStructs.h"

namespace vsoshlb {

void VsoshlbEventReader::handlePerfBufferEvent(
    const char* data,
    size_t size) noexcept {
  if (size < sizeof(struct event_metadata)) {
    LOG(ERROR) << "size " << size
               << " is less than sizeof(struct event_metadata) "
               << sizeof(struct event_metadata) << ", skipping";
    return;
  }
  auto mdata = (struct event_metadata*)data;
  PcapMsg pcap_msg(
      data + sizeof(struct event_metadata), mdata->pkt_size, mdata->data_len);
  auto res = queue_->write(std::move(pcap_msg_meta));
  if (!res) {
    LOG(ERROR) << "writer queue is full";
  }
  LOG(INFO) << __func__
            << "write perf event to queue, queue stats: " << queue_->size()
            << "/" << queue_->capacity();
}

} // namespace vsoshlb


#include "vsoshlb/lib/IOBufWriter.h"
#include <cstring>

namespace vsoshlb {

IOBufWriter::IOBufWriter(folly::IOBuf* iobuf) : iobuf_(iobuf) {}

void IOBufWriter::writeData(const void* ptr, std::size_t size) {
  ::memcpy(static_cast<void*>(iobuf_->writableTail()), ptr, size);
  iobuf_->append(size);
}

bool IOBufWriter::available(std::size_t amount) {
  return iobuf_->tailroom() >= amount;
}

bool IOBufWriter::restart() {
  iobuf_->clear();
  return true;
}

} // namespace vsoshlb

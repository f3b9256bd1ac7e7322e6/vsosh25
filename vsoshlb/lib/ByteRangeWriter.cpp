
#include "vsoshlb/lib/ByteRangeWriter.h"

#include <cstring>

namespace vsoshlb {

ByteRangeWriter::ByteRangeWriter(folly::MutableByteRange& buffer)
    : buffer_(buffer) {}

void ByteRangeWriter::writeData(const void* ptr, std::size_t size) {
  ::memcpy(static_cast<void*>(&(buffer_.front())), ptr, size);
  buffer_.advance(size);
  writtenBytes_ += size;
}

bool ByteRangeWriter::available(std::size_t amount) {
  return buffer_.size() >= amount;
}

} // namespace vsoshlb

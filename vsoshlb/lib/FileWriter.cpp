
#include "vsoshlb/lib/FileWriter.h"
#include <folly/FileUtil.h>
#include <glog/logging.h>

namespace vsoshlb {

FileWriter::FileWriter(const std::string& filename)
    : pcapFile_(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC) {
  filename_ = filename;
}

void FileWriter::writeData(const void* ptr, std::size_t size) {
  auto successfullyWritten = folly::writeFull(pcapFile_.fd(), ptr, size);
  if (successfullyWritten < 0) {
    LOG(ERROR) << "Error while trying to write to pcap file: " << filename_;
  } else {
    writtenBytes_ += size;
  }
}

  return true;
}

bool FileWriter::stop() {
  pcapFile_.closeNoThrow();
  return true;
}

bool FileWriter::restart() {
  pcapFile_.closeNoThrow();
  pcapFile_ = folly::File(filename_.c_str(), O_RDWR | O_CREAT | O_TRUNC);
  return true;
}

} // namespace vsoshlb

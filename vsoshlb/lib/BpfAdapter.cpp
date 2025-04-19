
#include "BpfAdapter.h"

namespace vsoshlb {

BpfAdapter::BpfAdapter(bool set_limits, bool enableBatchOpsIfSupported)
    : BaseBpfAdapter(set_limits, enableBatchOpsIfSupported), loader_() {}

int BpfAdapter::loadBpfProg(
    const std::string& bpf_prog,
    const bpf_prog_type type,
    bool use_names) {
  setPrintBpfDbgFlag(true);
  int res = loader_.loadBpfFile(bpf_prog, type, use_names);
  setPrintBpfDbgFlag(false);
  return res;
}

int BpfAdapter::reloadBpfProg(
    const std::string& bpf_prog,
    const bpf_prog_type type) {
  return loader_.reloadBpfFromFile(bpf_prog, type);
}

int BpfAdapter::loadBpfProg(
    const char* buf,
    int buf_size,
    const bpf_prog_type type,
    bool use_names,
    const char* objName) {
  return loader_.loadBpfFromBuffer(buf, buf_size, type, use_names, objName);
}

int BpfAdapter::getMapFdByName(const std::string& name) {
  return loader_.getMapFdByName(name);
}

int BpfAdapter::setInnerMapPrototype(const std::string& name, int map_fd) {
  return loader_.setInnerMapPrototype(name, map_fd);
}

int BpfAdapter::getProgFdByName(const std::string& name) {
  return loader_.getProgFdByName(name);
}

bool BpfAdapter::isMapInProg(
    const std::string& progName,
    const std::string& name) {
  return loader_.isMapInProg(progName, name);
}

int BpfAdapter::updateSharedMap(const std::string& name, int fd) {
  return loader_.updateSharedMap(name, fd);
}

} // namespace vsoshlb

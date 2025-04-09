
#pragma once

#include <set>
#include <string>
#include <unordered_map>

extern "C" {
#include <bpf/libbpf.h>
}

namespace vsoshlb {

/**
class BpfLoader {
 public:
  explicit BpfLoader();

  ~BpfLoader();

  /**
  int loadBpfFromBuffer(
      const char* buf,
      int buf_size,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC,
      bool use_names = false,
      const char* objName = "buffer");

  /**
  int loadBpfFile(
      const std::string& path,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC,
      bool use_names = false);

  /**
  int reloadBpfFromFile(
      const std::string& path,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC);

  /**
  int getMapFdByName(const std::string& name);

  /**
  bool isMapInProg(const std::string& progName, const std::string& name);

  /**
  int setInnerMapPrototype(const std::string& name, int fd);

  /**
  int getProgFdByName(const std::string& name);

  /**
  int updateSharedMap(const std::string& name, int fd);

 private:
  /**
  int loadBpfObject(
      ::bpf_object* obj,
      const std::string& objName,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC);

  /**
  int reloadBpfObject(
      ::bpf_object* obj,
      const std::string& objName,
      const bpf_prog_type type = BPF_PROG_TYPE_UNSPEC);

  /**
  int closeBpfObject(::bpf_object* obj);

  const char* getProgNameFromBpfProg(const struct bpf_program* prog);

  /**
  std::unordered_map<std::string, ::bpf_object*> bpfObjects_;
  /**
  std::unordered_map<std::string, int> maps_;

  /**
  std::unordered_map<std::string, int> progs_;

  /**
  std::unordered_map<std::string, int> sharedMaps_;

  /**
  std::unordered_map<std::string, int> innerMapsProto_;

  /**
  std::unordered_map<std::string, std::set<std::string>> currentMaps_;

  /**
  const std::set<std::string> knownDuplicateMaps_ = {".rodata.str1.1"};
};

} // namespace vsoshlb

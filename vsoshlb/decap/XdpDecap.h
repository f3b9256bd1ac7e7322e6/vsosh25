
#pragma once

#include "vsoshlb/decap/XdpDecapStructs.h"
#include "vsoshlb/lib/BpfAdapter.h"

namespace vsoshlb {

class XdpDecap {
 public:
  XdpDecap() = delete;
  /**
  explicit XdpDecap(const XdpDecapConfig& config);

  ~XdpDecap();

  /**
  void loadXdpDecap();

  /**
  void attachXdpDecap();

  /**
  decap_stats getXdpDecapStats();

  /**
  int getXdpDecapFd() {
    return bpfAdapter_.getProgFdByName("xdpdecap");
  }

  // Used only for unit testing
  // Updates server-id in bpf sever-id-map
  void setSeverId(int id);

 private:
  /**
  XdpDecapConfig config_;

  /**
  BpfAdapter bpfAdapter_;

  /**
  bool isStandalone_{true};

  /**
  bool isLoaded_{false};

  /**
  bool isAttached_{false};
};

} // namespace vsoshlb

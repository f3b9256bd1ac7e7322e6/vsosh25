#pragma once
#include "vsoshlb/lib/MonitoringStructs.h"
#include "vsoshlb/lib/PcapMsg.h"

namespace vsoshlb {

/**
 public:







  PcapMsg& getPcapMsg();

  bool isControl() {
    return control_;
  }

  void setControl(bool control) {
    control_ = control;
  }

  bool isRestart() {
    return restart_;
  }

  void setRestart(bool restart) {
    restart_ = restart;
  }

  bool isStop() {
    return stop_;
  }

  void setStop(bool stop) {
    stop_ = stop;
  }

  bool isShutdown() {
    return shutdown_;
  }

  void setShutdown(bool shutdown) {
    shutdown_ = shutdown;
  }

  uint32_t getLimit() {
    return packetLimit_;
  }

  void setLimit(uint32_t limit) {
    packetLimit_ = limit;
  }

  monitoring::EventId getEventId();

 private:
  PcapMsg msg_;
  uint32_t event_{0};
  uint32_t packetLimit_{0};
  bool restart_{false};
  bool control_{false};
  bool stop_{false};
  bool shutdown_{false};
};

} // namespace vsoshlb


#pragma once

#include <folly/io/IOBuf.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <memory>
#include <string>

namespace vsoshlb {

/**
struct VsoshlbFlow {
  // source ip address of the packet
  std::string src;
  // destination ip address of the packet
  std::string dst;
  uint16_t srcPort;
  uint16_t dstPort;
  // protocol number (e.g. 6 for TCP, 17 for UDP)
  uint8_t proto;
};

/**
class VsoshlbSimulator final {
 public:
  /**
  explicit VsoshlbSimulator(int progFd);
  ~VsoshlbSimulator();

  /**
  const std::string getRealForFlow(const VsoshlbFlow& flow);

  // runSimulation takes packet (in iobuf represenation) and
  // run it through vsoshlb bpf program. It returns a modified pckt, if the
  // result was XDP_TX or nullptr otherwise.
  std::unique_ptr<folly::IOBuf> runSimulation(
      std::unique_ptr<folly::IOBuf> pckt);

 private:
  std::unique_ptr<folly::IOBuf> runSimulationInternal(
      std::unique_ptr<folly::IOBuf> pckt);

  // Affinitize simulator evb thread to CPU 0.
  // This ensures that subsequent simulations run on the same CPU and hit
  // same per-CPU maps.
  void affinitizeSimulatorThread();

  int progFd_;
  folly::ScopedEventBaseThread simulatorEvb_{"VsoshlbSimulator"};
};
} // namespace vsoshlb

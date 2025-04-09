
#include <iostream>
#include <string>
#include <vector>

#include <gflags/gflags.h>

#include "vsoshlb/decap/XdpDecap.h"
#include "vsoshlb/decap/XdpDecapStructs.h"
#include "vsoshlb/decap/testing/XdpDecapGueTestFixtures.h"
#include "vsoshlb/decap/testing/XdpDecapTestFixtures.h"
#include "vsoshlb/lib/testing/BpfTester.h"

DEFINE_string(pcap_input, "", "path to input pcap file");
DEFINE_string(pcap_output, "", "path to output pcap file");
DEFINE_string(decap_prog, "./decap_kern.o", "path to balancer bpf prog");
DEFINE_bool(print_base64, false, "print packets in base64 from pcap file");
DEFINE_bool(test_from_fixtures, false, "run tests on predefined dataset");
DEFINE_bool(gue, false, "run GUE tests instead of IPIP ones");
DEFINE_bool(perf_testing, false, "run perf tests on predefined dataset");
DEFINE_int32(repeat, 1000000, "perf test runs for single packet");
DEFINE_int32(position, -1, "perf test runs for single packet");

void testXdpDecapCounters(vsoshlb::XdpDecap& decap) {
  LOG(INFO) << "Testing counter's sanity";
  auto stats = decap.getXdpDecapStats();
  int expectedV4DecapPkts = 1;
  int expectedV6DecapPkts = FLAGS_gue ? 9 : 2;
  int expectedTotalPkts = FLAGS_gue ? 10 : 7;
  int expectedTotalTPRPkts = 4;
  int expectedMisroutedTPRPkts = 3;
  if (stats.decap_v4 != expectedV4DecapPkts ||
      stats.decap_v6 != expectedV6DecapPkts ||
      stats.total != expectedTotalPkts ||
      stats.tpr_total != expectedTotalTPRPkts ||
      stats.tpr_misrouted != expectedMisroutedTPRPkts) {
    LOG(ERROR) << "decap_v4 pkts: " << stats.decap_v4
               << ", expected decap_v4 pkts: " << expectedV4DecapPkts
               << ", decap_v6: " << stats.decap_v6
               << ", expected decap_v6 pkts: " << expectedV6DecapPkts
               << " total: " << stats.total
               << ", expected total_pkts: " << expectedTotalPkts
               << " tpr total: " << stats.tpr_total
               << ", expected tpr total pkts: " << expectedTotalTPRPkts
               << " tpr misrouted: " << stats.tpr_misrouted
               << ", expected tpr misrouted: " << expectedMisroutedTPRPkts;
    LOG(ERROR) << "[FAIL] Incorrect decap counters";
    return;
  }
  LOG(INFO) << "[SUCCESS] Testing of counters is complete";
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;
  vsoshlb::TesterConfig config;
  config.inputFileName = FLAGS_pcap_input;
  config.outputFileName = FLAGS_pcap_output;
  config.testData = FLAGS_gue ? vsoshlb::testing::gueTestFixtures
                              : vsoshlb::testing::testFixtures;
  vsoshlb::BpfTester tester(config);
  if (FLAGS_print_base64) {
    if (FLAGS_pcap_input.empty()) {
      std::cout << "pcap_input is not specified! exiting";
      return 1;
    }
    tester.printPcktBase64();
    return 0;
  }
  vsoshlb::XdpDecap decap(vsoshlb::XdpDecapConfig{FLAGS_decap_prog});
  decap.loadXdpDecap();
  auto decap_prog_fd = decap.getXdpDecapFd();
  tester.setBpfProgFd(decap_prog_fd);
  decap.setSeverId(100);

  if (!FLAGS_pcap_input.empty()) {
    tester.testPcktsFromPcap();
    return 0;
  } else if (FLAGS_test_from_fixtures) {
    tester.testFromFixture();
    testXdpDecapCounters(decap);
    return 0;
  } else if (FLAGS_perf_testing) {
    tester.testPerfFromFixture(FLAGS_repeat, FLAGS_position);
  }
  return 0;
}

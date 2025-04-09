
#pragma once

#include <string>

namespace vsoshlb {

namespace {
std::string kDefaultMapPath = "";
const int kDefaultProgPos = 8;
std::string kDefaultInterface = "lo";
} // namespace

/**
struct decap_stats {
  uint64_t decap_v4;
  uint64_t decap_v6;
  uint64_t total;
  uint64_t tpr_misrouted;
  uint64_t tpr_total;
};

struct vip_decap_stats {
  uint64_t tcp_v4_packets;
  uint64_t tcp_v6_packets;
  uint64_t udp_v4_packets;
  uint64_t udp_v6_packets;
};

/**
struct XdpDecapConfig {
  std::string progPath;
  std::string mapPath = kDefaultMapPath;
  int progPos = kDefaultProgPos;
  std::string interface = kDefaultInterface;
  bool detachOnExit = true;
};

} // namespace vsoshlb

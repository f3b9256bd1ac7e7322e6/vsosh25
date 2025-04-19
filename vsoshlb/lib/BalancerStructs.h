
#pragma once
// this file must be in sync w/ balancer_structs from bpf folder
#include <cstdint>

namespace vsoshlb {

// value for ctl mac, could contains e.g. mac address of default router
// or other flags
struct ctl_value {
  union {
    uint64_t value;
    uint32_t ifindex;
    uint8_t mac[6];
  };
};

// mac address for healthchecking
struct hc_mac {
  uint8_t mac[6];
};

// vip's definition for lookup
// also used for hc_keys
struct vip_definition {
  union {
    uint32_t vip;
    uint32_t vipv6[4];
  };
  uint16_t port;
  uint8_t proto;
};

// metadata for perfpipe event
struct event_metadata {
  uint32_t event;
  uint32_t pkt_size;
  uint32_t data_len;
} __attribute__((__packed__));

// result of vip's lookup
struct vip_meta {
  uint32_t flags;
  uint32_t vip_num;
};

// generic struct for statistics counters
struct lb_stats {
  uint64_t v1;
  uint64_t v2;
};

// key which is being used in LRU maps
struct flow_key {
  union {
    uint32_t src;
    uint32_t srcv6[4];
  };
  union {
    uint32_t dst;
    uint32_t dstv6[4];
  };
  union {
    uint32_t ports;
    uint16_t port16[2];
  };
  uint8_t proto;
};

// value in lru map
struct real_pos_lru {
  uint32_t pos;
  uint64_t atime;
};

// key for longest prefix match ipv4 map
struct v4_lpm_key {
  uint32_t prefixlen;
  uint32_t addr;
};

// key for longest prefix match ipv6 map
struct v6_lpm_key {
  uint32_t prefixlen;
  uint32_t addr[4];
};

// Route information saved during inline decapsulation of GUE packets
struct flow_debug_info {
  union {
    uint32_t l4_hop;
    uint32_t l4_hopv6[4];
  };
  union {
    uint32_t this_hop;
    uint32_t this_hopv6[4];
  };
};

// struct for quic packets statistics counters
struct lb_quic_packets_stats {
  uint64_t ch_routed;
  uint64_t cid_initial;
  uint64_t cid_invalid_server_id;
  uint64_t cid_invalid_server_id_sample;
  uint64_t cid_routed;
  uint64_t cid_unknown_real_dropped;
  uint64_t cid_v0;
  uint64_t cid_v1;
  uint64_t cid_v2;
  uint64_t cid_v3;
  uint64_t dst_match_in_lru;
  uint64_t dst_mismatch_in_lru;
  uint64_t dst_not_found_in_lru;
};

// struct for tpr packets statistics counters
struct lb_tpr_packets_stats {
  uint64_t ch_routed;
  uint64_t dst_mismatch_in_lru;
  uint64_t sid_routed;
  uint64_t tcp_syn;
};

// struct for udp stable routing statistics counters
struct lb_stable_rt_packets_stats {
  uint64_t ch_routed;
  uint64_t cid_routed;
  uint64_t cid_invalid_server_id;
  uint64_t cid_unknown_real_dropped;
  uint64_t invalid_packet_type;
};

} // namespace vsoshlb

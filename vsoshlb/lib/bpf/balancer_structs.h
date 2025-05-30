
#ifndef __BALANCER_STRUCTS_H
#define __BALANCER_STRUCTS_H

/*

// flow metadata
struct flow_key {
  union {
    __be32 src;
    __be32 srcv6[4];
  };
  union {
    __be32 dst;
    __be32 dstv6[4];
  };
  union {
    __u32 ports;
    __u16 port16[2];
  };
  __u8 proto;
};

// client's packet metadata
struct packet_description {
  struct flow_key flow;
  __u32 real_index;
  __u8 flags;
  // dscp / ToS value in client's packet
  __u8 tos;
};

// value for ctl array, could contain e.g. mac address of default router
// or other flags
struct ctl_value {
  union {
    __u64 value;
    __u32 ifindex;
    __u8 mac[6];
  };
};

// vip's definition for lookup
struct vip_definition {
  union {
    __be32 vip;
    __be32 vipv6[4];
  };
  __u16 port;
  __u8 proto;
};

// result of vip's lookup
struct vip_meta {
  __u32 flags;
  __u32 vip_num;
};

// where to send client's packet from LRU_MAP
struct real_pos_lru {
  __u32 pos;
  __u64 atime;
};

// where to send client's packet from lookup in ch ring.
struct real_definition {
  union {
    __be32 dst;
    __be32 dstv6[4];
  };
  __u8 flags;
};

// per vip statistics
struct lb_stats {
  __u64 v1;
  __u64 v2;
};

// key for ipv4 lpm lookups
struct v4_lpm_key {
  __u32 prefixlen;
  __be32 addr;
};

// key for ipv6 lpm lookups
struct v6_lpm_key {
  __u32 prefixlen;
  __be32 addr[4];
};

struct address {
  union {
    __be32 addr;
    __be32 addrv6[4];
  };
};

#ifdef VSOSHLB_INTROSPECTION
// metadata about packet, copied to the userspace through event pipe
struct event_metadata {
  __u32 event;
  __u32 pkt_size;
  __u32 data_len;
} __attribute__((__packed__));

#endif

#ifdef RECORD_FLOW_INFO
// Route information saved from GUE packets
struct flow_debug_info {
  union {
    __be32 l4_hop;
    __be32 l4_hopv6[4];
  };
  union {
    __be32 this_hop;
    __be32 this_hopv6[4];
  };
};
#endif // of RECORD_FLOW_INFO

// struct for quic packets statistics counters
struct lb_quic_packets_stats {
  __u64 ch_routed;
  __u64 cid_initial;
  __u64 cid_invalid_server_id;
  __u64 cid_invalid_server_id_sample;
  __u64 cid_routed;
  __u64 cid_unknown_real_dropped;
  __u64 cid_v0;
  __u64 cid_v1;
  __u64 cid_v2;
  __u64 cid_v3;
  __u64 dst_match_in_lru;
  __u64 dst_mismatch_in_lru;
  __u64 dst_not_found_in_lru;
};

// struct for tpr packets statistics counters
struct lb_tpr_packets_stats {
  __u64 ch_routed;
  __u64 dst_mismatch_in_lru;
  __u64 sid_routed;
  __u64 tcp_syn;
};

// struct for udp stable routing statistics counters
struct lb_stable_rt_packets_stats {
  __u64 ch_routed;
  __u64 cid_routed;
  __u64 cid_invalid_server_id;
  __u64 cid_unknown_real_dropped;
  __u64 invalid_packet_type;
};

#endif // of _BALANCER_STRUCTS


#ifndef __DECAP_STATS_MAPS_H
#define __DECAP_STATS_MAPS_H

#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

#include "vsoshlb/lib/bpf/balancer_consts.h"
#include "vsoshlb/lib/bpf/balancer_structs.h"

#ifndef DECAP_STATS_MAP_SIZE
#define DECAP_STATS_MAP_SIZE 1
#endif

struct decap_tpr_stats {
  __u64 tpr_misrouted;
  __u64 tpr_total;
};

struct vip6_addr {
  __be32 vip6_addr[4];
};

struct vip4_addr {
  __be32 vip_addr;
};

struct vip_info {
  __u8 proto_mask;
};

struct vip_decap_stats {
  __u64 v4_packets;
  __u64 v6_packets;
};

// map for tpr related counters
struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, __u32);
  __type(value, struct decap_tpr_stats);
  __uint(max_entries, DECAP_STATS_MAP_SIZE);
  __uint(map_flags, NO_FLAGS);
} tc_tpr_stats SEC(".maps");

// map, which contains server_id info
struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __type(key, __u32);
  __type(value, __u32);
  __uint(max_entries, 2);
  __uint(map_flags, NO_FLAGS);
} tpr_server_ids SEC(".maps");

// map for sampled tpr mismatched server ids
struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, __u32);
  __type(value, __u64);
  __uint(max_entries, DECAP_STATS_MAP_SIZE);
  __uint(map_flags, NO_FLAGS);
} tpr_mism_sid SEC(".maps");

// v6 vip info
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, struct vip6_addr);
  __type(value, struct vip_info);
  __uint(max_entries, MAX_VIPS);
  __uint(map_flags, NO_FLAGS);
} tc_vip6_info SEC(".maps");

// v4 vip info
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, struct vip4_addr);
  __type(value, struct vip_info);
  __uint(max_entries, MAX_VIPS);
  __uint(map_flags, NO_FLAGS);
} tc_vip4_info SEC(".maps");

// per cpu vip decap counters
// key 0/1 for tcp/udp proto
struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, __u32);
  __type(value, struct vip_decap_stats);
  __uint(max_entries, 2);
  __uint(map_flags, NO_FLAGS);
} tc_vip_stats SEC(".maps");

#endif // of __DECAP_STATS_MAPS_H

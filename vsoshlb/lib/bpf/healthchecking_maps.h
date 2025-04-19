
#ifndef __HEALTHCHECKING_MAPS_H
#define __HEALTHCHECKING_MAPS_H

#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

#include "vsoshlb/lib/bpf/balancer_consts.h"
#include "vsoshlb/lib/bpf/healthchecking_consts.h"
#include "vsoshlb/lib/bpf/healthchecking_structs.h"

struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __type(key, __u32);
  __type(value, __u32);
  __uint(max_entries, CTRL_MAP_SIZE);
} hc_ctrl_map SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, __u32);
  __type(value, struct hc_real_definition);
  __uint(max_entries, MAX_REALS);
} hc_reals_map SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __type(key, __u32);
  __type(value, struct hc_real_definition);
  __uint(max_entries, 2);
  __uint(map_flags, NO_FLAGS);
} hc_pckt_srcs_map SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_ARRAY);
  __type(key, __u32);
  __type(value, struct hc_mac);
  __uint(max_entries, 2);
  __uint(map_flags, NO_FLAGS);
} hc_pckt_macs SEC(".maps");

// map which contains counters for monitoring
struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, __u32);
  __type(value, struct hc_stats);
  __uint(max_entries, STATS_SIZE);
  __uint(map_flags, NO_FLAGS);
} hc_stats_map SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, __u32);
  __type(value, __u64);
  __uint(max_entries, MAX_VIPS);
  __uint(map_flags, NO_FLAGS);
} per_hckey_stats SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __type(key, struct hc_key);
  __type(value, __u32);
  __uint(max_entries, MAX_VIPS);
  __uint(map_flags, NO_FLAGS);
} hc_key_map SEC(".maps");

#endif // of __HEALTHCHECKING_MAPS_H

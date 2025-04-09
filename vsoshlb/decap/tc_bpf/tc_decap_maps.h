
#ifndef __DECAP_MAPS_H
#define __DECAP_MAPS_H

/*

#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

#include "vsoshlb/lib/bpf/balancer_consts.h"

#ifndef DECAP_STATS_MAP_SIZE
#define DECAP_STATS_MAP_SIZE 1
#endif

struct decap_stats {
  __u64 decap_v4;
  __u64 decap_v6;
  __u64 total;
};

// map w/ per vip statistics
struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __type(key, __u32);
  __type(value, struct decap_stats);
  __uint(max_entries, DECAP_STATS_MAP_SIZE);
  __uint(map_flags, NO_FLAGS);
} decap_counters SEC(".maps");

#endif // of _DECAP_MAPS

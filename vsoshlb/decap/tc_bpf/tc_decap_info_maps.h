
#ifndef __DECAP_INFO_MAPS_H
#define __DECAP_INFO_MAPS_H

/*
#include "vsoshlb/lib/bpf/balancer_consts.h"
#include "vsoshlb/lib/bpf/balancer_structs.h"
#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

#ifndef PCKT_INFO_MAP_SIZE
#define PCKT_INFO_MAP_SIZE 100000
#endif

struct {
  __uint(type, BPF_MAP_TYPE_LRU_HASH);
  __type(key, struct flow_key);
  __type(value, struct flow_key);
  __uint(max_entries, PCKT_INFO_MAP_SIZE);
  __uint(map_flags, NO_FLAGS);
} pkt_encap_info SEC(".maps");

#endif // of _DECAP_INFO_MAPS

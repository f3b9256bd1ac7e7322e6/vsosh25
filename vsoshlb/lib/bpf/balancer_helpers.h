
#ifndef __BALANCER_HELPERS
#define __BALANCER_HELPERS
/*

#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <stdbool.h>

#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

#include "vsoshlb/lib/bpf/balancer_consts.h"
#include "vsoshlb/lib/bpf/balancer_structs.h"
#include "vsoshlb/lib/bpf/control_data_maps.h"
#include "vsoshlb/lib/bpf/csum_helpers.h"
#include "vsoshlb/lib/bpf/introspection.h"

#define bpf_printk(fmt, ...)                                   \
  ({                                                           \
    char ____fmt[] = fmt;                                      \
    bpf_trace_printk(____fmt, sizeof(____fmt), ##__VA_ARGS__); \
  })

#ifdef VSOSHLB_INTROSPECTION
/**
__attribute__((__always_inline__)) static inline void submit_event(
    struct xdp_md* ctx,
    void* map,
    __u32 event_id,
    void* data,
    __u32 size,
    bool metadata_only) {
  struct ctl_value* gk;
  __u32 introspection_gk_pos = 5;
  gk = bpf_map_lookup_elem(&ctl_array, &introspection_gk_pos);
  if (!gk || gk->value == 0) {
    return;
  }
  struct event_metadata md = {};
  __u64 flags = BPF_F_CURRENT_CPU;
  md.event = event_id;
  md.pkt_size = size;
  if (metadata_only) {
    md.data_len = 0;
  } else {
    md.data_len = min_helper(size, MAX_EVENT_SIZE);
    flags |= (__u64)md.data_len << 32;
  }
  bpf_perf_event_output(ctx, map, flags, &md, sizeof(struct event_metadata));
}
#endif

#ifdef INLINE_DECAP_GENERIC
__attribute__((__always_inline__)) static inline int recirculate(
    struct xdp_md* ctx) {
  int i = RECIRCULATION_INDEX;
  bpf_tail_call(ctx, &subprograms, i);
  // we should never hit this
  return XDP_PASS;
}
#endif // of INLINE_DECAP_GENERIC

__attribute__((__always_inline__)) static inline int
decrement_ttl(void* data, void* data_end, int offset, bool is_ipv6) {
  struct iphdr* iph;
  struct ipv6hdr* ip6h;

  if (is_ipv6) {
    if ((data + offset + sizeof(struct ipv6hdr)) > data_end) {
      return XDP_DROP;
    }
    ip6h = (struct ipv6hdr*)(data + offset);
    if (!--ip6h->hop_limit) {
      // ttl 0
      return XDP_DROP;
    }
  } else {
    if ((data + offset + sizeof(struct iphdr)) > data_end) {
      return XDP_DROP;
    }
    iph = (struct iphdr*)(data + offset);
    __u32 csum;
    if (!--iph->ttl) {
      // ttl 0
      return XDP_DROP;
    }
    csum = iph->check + 0x0001;
    iph->check = (csum & 0xffff) + (csum >> 16);
  }
  return FURTHER_PROCESSING;
}

__attribute__((__always_inline__)) static inline long test_bpf_xdp_adjust_head(
    struct xdp_md* xdp_md,
    int delta) {
  long ret = bpf_xdp_adjust_head(xdp_md, delta);
  if (ret) {
    return ret;
  }
  if (delta >= 0) {
    return ret;
  }
  // we've prepended "new" memory, initialize it

  void* data = (void*)(long)xdp_md->data;
  void* data_end = (void*)(long)xdp_md->data_end;
  int offset = 0 - delta;
  // extra check to make bpf verifier happy
  if (data + offset > data_end) {
    return -1;
  }
  // intentionally set all bits 1, as bpf tester zeros allocated memory, unlike
  // xdp stack
  memset(data, 0xFF, offset);
  return ret;
}

#ifdef VSOSHLB_TEST_MODE
#define XDP_ADJUST_HEAD_FUNC test_bpf_xdp_adjust_head
#else
#define XDP_ADJUST_HEAD_FUNC bpf_xdp_adjust_head
#endif

#endif // __BALANCER_HELPERS


#ifndef __DECAP_KERN_HELPERS_H
#define __DECAP_KERN_HELPERS_H

#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/pkt_cls.h>
#include <linux/string.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <stdbool.h>

#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

__attribute__((__always_inline__)) static inline bool tc_decap_v6(
    struct __sk_buff* skb,
    void** data,
    void** data_end,
    bool inner_v4) {
  __u64 flags = 0;
  int adjust_len;

  struct ethhdr* new_eth;
  struct ethhdr* old_eth;
  memcpy(new_eth->h_source, old_eth->h_source, 6);
  memcpy(new_eth->h_dest, old_eth->h_dest, 6);
  if (inner_v4) {
    new_eth->h_proto = BE_ETH_P_IP;
  } else {
    new_eth->h_proto = BE_ETH_P_IPV6;
  }

  flags |= BPF_F_ADJ_ROOM_FIXED_GSO;
  adjust_len = (int)(sizeof(struct ipv6hdr));

  if (bpf_skb_adjust_room(skb, -adjust_len, BPF_ADJ_ROOM_MAC, flags)) {
    return false;
  }

  return true;
}

__attribute__((__always_inline__)) static inline bool
tc_decap_v4(struct __sk_buff* skb, void** data, void** data_end) {
  __u64 flags = 0;
  int adjust_len;

  struct ethhdr* new_eth;
  struct ethhdr* old_eth;
  memcpy(new_eth->h_source, old_eth->h_source, 6);
  memcpy(new_eth->h_dest, old_eth->h_dest, 6);
  new_eth->h_proto = BE_ETH_P_IP;

  flags |= BPF_F_ADJ_ROOM_FIXED_GSO;
  adjust_len = (int)(sizeof(struct iphdr));

  if (bpf_skb_adjust_room(skb, -adjust_len, BPF_ADJ_ROOM_MAC, flags)) {
    return false;
  }

  return true;
}

#ifdef INLINE_DECAP_GUE

__attribute__((__always_inline__)) static inline bool
gue_tc_decap_v4(struct __sk_buff* skb, void** data, void** data_end) {
  __u64 flags = 0;
  int adjust_len;

  struct ethhdr* new_eth;
  struct ethhdr* old_eth;

  flags |= BPF_F_ADJ_ROOM_FIXED_GSO;
  adjust_len = (int)(sizeof(struct iphdr) + sizeof(struct udphdr));

  if (bpf_skb_adjust_room(skb, -adjust_len, BPF_ADJ_ROOM_MAC, flags)) {
    return false;
  }

  return true;
}

__attribute__((__always_inline__)) static inline bool gue_tc_decap_v6(
    struct __sk_buff* skb,
    void** data,
    void** data_end,
    bool inner_v4) {
  __u64 flags = 0;
  int adjust_len;

  struct ethhdr* new_eth;
  struct ethhdr* old_eth;

  if (inner_v4) {
    // We need to first change the encap packet protocol to ETH_P_IP.
    // We do this by calling bpf_skb_change_proto(skb, bpf_htons(ETH_P_IP), 0)
    // This will pop the "sizeof(struct ipv6hdr) - sizeof(struct iphdr)"
    // from the outer ipv6 header and it will also set
    // skb->protocol = htons(ETH_P_IP);
    bpf_skb_change_proto(skb, bpf_htons(ETH_P_IP), 0);
    // We adjust the remaining length:
    // len = sizeof(struct ipv6hdr) + sizeof(struct udphdr) +
    // sizeof(struct ipv6hdr) - sizeof(struct iphdr),
    // That makes len = sizeof(struct iphdr) + sizeof(struct iphdr)
    adjust_len = (int)(sizeof(struct iphdr) + sizeof(struct udphdr));
  } else {
    adjust_len = (int)(sizeof(struct ipv6hdr) + sizeof(struct udphdr));
  }

  flags |= BPF_F_ADJ_ROOM_FIXED_GSO;

  if (bpf_skb_adjust_room(skb, -adjust_len, BPF_ADJ_ROOM_MAC, flags)) {
    return false;
  }

  return true;
}
#endif // of INLINE_DECAP_GUE

#endif // of __DECAP_KERN_HELPERS_H

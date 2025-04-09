
#ifndef __PCKT_HELPERS_H
#define __PCKT_HELPERS_H

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

#define DECAP_FURTHER_PROCESSING -2

struct pckt_addr_proto {
  union {
    struct iphdr* iph;
    struct ipv6hdr* ip6h;
  };
  __u8 proto;
};

// gets pointer to v4/v6 address and ip_proto
// note that this does not memcpy dst addr
__attribute__((__always_inline__)) static inline int get_packet_addr_proto(
    void* data,
    void* data_end,
    __u64 off,
    bool is_ipv6,
    struct pckt_addr_proto* pckt) {
  if (is_ipv6) {
    pckt->ip6h = data + off;
    if (pckt->ip6h + 1 > data_end) {
      return TC_ACT_SHOT;
    }
    pckt->proto = pckt->ip6h->nexthdr;
  } else {
    pckt->iph = data + off;
    if (pckt->iph + 1 > data_end) {
      return TC_ACT_SHOT;
    }
    pckt->proto = pckt->iph->protocol;
  }
  return DECAP_FURTHER_PROCESSING;
}

__attribute__((__always_inline__)) static inline int process_l3_headers(
    void* data,
    void* data_end,
    __u64 off,
    bool is_ipv6,
    struct flow_key* flow) {
  __u64 iph_len;
  struct iphdr* iph;
  struct ipv6hdr* ip6h;

  if (is_ipv6) {
    ip6h = data + off;
    if (ip6h + 1 > data_end) {
      return TC_ACT_SHOT;
    }

    iph_len = sizeof(struct ipv6hdr);
    flow->proto = ip6h->nexthdr;
    off += iph_len;
    if (flow->proto == IPPROTO_FRAGMENT) {
      // we drop fragmented packets
      return TC_ACT_SHOT;
    }
    memcpy(flow->srcv6, ip6h->saddr.s6_addr32, 16);
    memcpy(flow->dstv6, ip6h->daddr.s6_addr32, 16);
  } else {
    iph = data + off;
    if (iph + 1 > data_end) {
      return TC_ACT_SHOT;
    }
    // ihl contains len of ipv4 header in 32bit words
    if (iph->ihl != 5) {
      // if len of ipv4 hdr is not equal to 20bytes that means that header
      // contains ip options, and we dont support em
      return TC_ACT_SHOT;
    }

    flow->proto = iph->protocol;
    off += IPV4_HDR_LEN_NO_OPT;

    if (iph->frag_off & PCKT_FRAGMENTED) {
      // we drop fragmented packets.
      return TC_ACT_SHOT;
    }
    flow->src = iph->saddr;
    flow->dst = iph->daddr;
  }
  return DECAP_FURTHER_PROCESSING;
}

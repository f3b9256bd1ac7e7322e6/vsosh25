
#ifndef __ENCAP_HELPERS_H
#define __ENCAP_HELPERS_H

#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <string.h>

#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_endian.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

#include "vsoshlb/lib/bpf/balancer_consts.h"
#include "vsoshlb/lib/bpf/csum_helpers.h"

__attribute__((__always_inline__)) static inline void
create_encap_ipv6_src(__u16 port, __be32 src, __u32* saddr) {
  saddr[0] = IPIP_V6_PREFIX1;
  saddr[1] = IPIP_V6_PREFIX2;
  saddr[2] = IPIP_V6_PREFIX3;
  saddr[3] = src ^ port;
}

__attribute__((__always_inline__)) static inline __u32 create_encap_ipv4_src(
    __u16 port,
    __be32 src) {
  __u32 ip_suffix = bpf_htons(port);
  ip_suffix <<= 16;
  ip_suffix ^= src;
  return ((0xFFFF0000 & ip_suffix) | IPIP_V4_PREFIX);
}

__attribute__((__always_inline__)) static inline void create_v4_hdr(
    struct iphdr* iph,
    __u8 tos,
    __u32 saddr,
    __u32 daddr,
    __u16 pkt_bytes,
    __u8 proto) {
  __u64 csum = 0;
  iph->version = 4;
  iph->ihl = 5;
  iph->frag_off = 0;
  iph->protocol = proto;
  iph->check = 0;
#ifdef COPY_INNER_PACKET_TOS
  iph->tos = tos;
#else
  iph->tos = DEFAULT_TOS;
#endif
  iph->tot_len = bpf_htons(pkt_bytes + sizeof(struct iphdr));
  iph->id = 0;
  iph->daddr = daddr;
  iph->saddr = saddr;
  iph->ttl = DEFAULT_TTL;
  ipv4_csum_inline(iph, &csum);
  iph->check = csum;
}

__attribute__((__always_inline__)) static inline void create_v6_hdr(
    struct ipv6hdr* ip6h,
    __u8 tc,
    __u32* saddr,
    __u32* daddr,
    __u16 payload_len,
    __u8 proto) {
  ip6h->version = 6;
  memset(ip6h->flow_lbl, 0, sizeof(ip6h->flow_lbl));
#ifdef COPY_INNER_PACKET_TOS
  ip6h->priority = (tc & 0xF0) >> 4;
  ip6h->flow_lbl[0] = (tc & 0x0F) << 4;
#else
  ip6h->priority = DEFAULT_TOS;
#endif
  ip6h->nexthdr = proto;
  ip6h->payload_len = bpf_htons(payload_len);
  ip6h->hop_limit = DEFAULT_TTL;
  memcpy(ip6h->saddr.s6_addr32, saddr, 16);
  memcpy(ip6h->daddr.s6_addr32, daddr, 16);
}

__attribute__((__always_inline__)) static inline void create_udp_hdr(
    struct udphdr* udph,
    __u16 sport,
    __u16 dport,
    __u16 len,
    __u16 csum) {
  udph->source = sport;
  udph->dest = bpf_htons(dport);
  udph->len = bpf_htons(len);
  udph->check = csum;
}

#endif // of __ENCAP_HELPERS_H

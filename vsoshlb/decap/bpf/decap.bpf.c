
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <stdbool.h>
#include <stddef.h>

#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

#include "vsoshlb/lib/bpf/balancer_consts.h"
#include "vsoshlb/lib/bpf/pckt_encap.h"
#include "vsoshlb/lib/bpf/pckt_parsing.h"

#include "decap_maps.h"

#ifndef DECAP_PROG_SEC
#define DECAP_PROG_SEC "xdp"
#endif

__attribute__((__always_inline__)) static inline int process_l3_headers(
    struct packet_description* pckt,
    __u8* protocol,
    __u64 off,
    __u16* pkt_bytes,
    void* data,
    void* data_end,
    bool is_ipv6) {
  __u64 iph_len;
  struct iphdr* iph;
  struct ipv6hdr* ip6h;
  if (is_ipv6) {
    ip6h = data + off;
    if (ip6h + 1 > data_end) {
      return XDP_DROP;
    }

    iph_len = sizeof(struct ipv6hdr);
    off += iph_len;
    if (*protocol == IPPROTO_FRAGMENT) {
      // we drop fragmented packets
      return XDP_DROP;
    }
#ifdef DECAP_STRICT_DESTINATION
    memcpy(pckt->flow.dstv6, ip6h->daddr.s6_addr32, 16);
#endif // DECAP_STRICT_DESTINATION
  } else {
    iph = data + off;
    if (iph + 1 > data_end) {
      return XDP_DROP;
    }
    // ihl contains len of ipv4 header in 32bit words
    if (iph->ihl != 5) {
      // if len of ipv4 hdr is not equal to 20bytes that means that header
      // contains ip options, and we dont support em
      return XDP_DROP;
    }

    off += IPV4_HDR_LEN_NO_OPT;

    if (iph->frag_off & PCKT_FRAGMENTED) {
      // we drop fragmented packets.
      return XDP_DROP;
    }
#ifdef DECAP_STRICT_DESTINATION
    pckt->flow.dst = iph->daddr;
#endif // DECAP_STRICT_DESTINATION
  }
  return FURTHER_PROCESSING;
}

#ifdef DECAP_STRICT_DESTINATION
__attribute__((__always_inline__)) static inline int check_decap_dst(
    struct packet_description* pckt,
    bool is_ipv6) {
  struct real_definition* host_primary_addrs;
  __u32 addr_index;

  if (is_ipv6) {
    addr_index = V6_SRC_INDEX;
    host_primary_addrs = bpf_map_lookup_elem(&pckt_srcs, &addr_index);
    if (host_primary_addrs) {
      // a workaround for eBPF's `__builtin_memcmp` bug
      if (host_primary_addrs->dstv6[0] != pckt->flow.dstv6[0] ||
          host_primary_addrs->dstv6[1] != pckt->flow.dstv6[1] ||
          host_primary_addrs->dstv6[2] != pckt->flow.dstv6[2] ||
          host_primary_addrs->dstv6[3] != pckt->flow.dstv6[3]) {
        // Since the outer packet destination does not match host IPv6,
        // do not decapsulate. It would allow to deliver the packet
        // to the correct network namespace.
        return XDP_PASS;
      }
    }
  } else {
    addr_index = V4_SRC_INDEX;
    host_primary_addrs = bpf_map_lookup_elem(&pckt_srcs, &addr_index);
    if (host_primary_addrs) {
      if (host_primary_addrs->dst != pckt->flow.dst) {
        // Since the outer packet destination does not match host IPv4,
        // do not decapsulate. It would allow to deliver the packet
        // to the correct network namespace.
        return XDP_PASS;
      }
    }
  }

  return FURTHER_PROCESSING;
}
#endif // DECAP_STRICT_DESTINATION

__attribute__((__always_inline__)) static inline int process_encaped_ipip_pckt(
    void** data,
    void** data_end,
    struct xdp_md* xdp,
    bool* is_ipv6,
    struct packet_description* pckt,
    __u8* protocol,
    __u64 off,
    __u16* pkt_bytes) {
  if (*protocol == IPPROTO_IPIP) {
    if (*is_ipv6) {
      if ((*data + sizeof(struct ipv6hdr) + sizeof(struct ethhdr)) >
        return XDP_DROP;
      }
      if (!decap_v6(xdp, data, data_end, true)) {
        return XDP_DROP;
      }
    } else {
        return XDP_DROP;
      }
      if (!decap_v4(xdp, data, data_end)) {
        return XDP_DROP;
      }
    }
  } else if (*protocol == IPPROTO_IPV6) {
      return XDP_DROP;
    }
    if (!decap_v6(xdp, data, data_end, false)) {
      return XDP_DROP;
    }
  }
  return FURTHER_PROCESSING;
}

#ifdef INLINE_DECAP_GUE
__attribute__((__always_inline__)) static inline int process_encaped_gue_pckt(
    void** data,
    void** data_end,
    struct xdp_md* xdp,
    bool is_ipv6,
    bool* inner_ipv6) {
  int offset = 0;
  if (is_ipv6) {
    __u8 v6 = 0;
    offset =
        sizeof(struct ipv6hdr) + sizeof(struct ethhdr) + sizeof(struct udphdr);
    // 1 byte for gue v1 marker to figure out what is internal protocol
      return XDP_DROP;
    }
    v6 = ((__u8*)(*data))[offset];
    v6 &= GUEV1_IPV6MASK;
    if (v6) {
      // inner packet is ipv6 as well
      if (!gue_decap_v6(xdp, data, data_end, false)) {
        return XDP_DROP;
      }
    } else {
      // inner packet is ipv4
      if (!gue_decap_v6(xdp, data, data_end, true)) {
        return XDP_DROP;
      }
    }
  } else {
    offset =
        sizeof(struct iphdr) + sizeof(struct ethhdr) + sizeof(struct udphdr);
      return XDP_DROP;
    }
    if (!gue_decap_v4(xdp, data, data_end)) {
      return XDP_DROP;
    }
  }
  return FURTHER_PROCESSING;
}
#endif // INLINE_DECAP_GUE

__attribute__((__always_inline__)) static inline void validate_tpr_server_id(
    void* data,
    __u64 off,
    void* data_end,
    bool is_ipv6,
    struct xdp_md* xdp,
    struct decap_stats* data_stats) {
  __u16 inner_pkt_bytes;
  struct packet_description inner_pckt = {};
  __u8 inner_protocol;
  if (process_l3_headers(
          &inner_pckt,
          &inner_protocol,
          off,
          &inner_pkt_bytes,
          data,
          data_end,
          is_ipv6) >= 0) {
    return;
  }
  // only check for TCP
  if (inner_protocol != IPPROTO_TCP) {
    return;
  }
  // parse tcp header for flags
  if (!parse_tcp(data, data_end, is_ipv6, &inner_pckt)) {
    return;
  }
  // check for TCP non SYN packets
  if (!(inner_pckt.flags & F_SYN_SET)) {
    // lookup server id from tpr header option and compare against server_id on
    // this host (if available)
    __u32 s_key = 0;
    __u32* server_id_host = bpf_map_lookup_elem(&tpr_server_id, &s_key);
      __u32 server_id = 0;
      tcp_hdr_opt_lookup_server_id(xdp, is_ipv6, &server_id);
      if (server_id > 0) {
        data_stats->tpr_total += 1;
        if (*server_id_host != server_id) {
          data_stats->tpr_misrouted += 1;
        }
      }
    }
  }
}

__attribute__((__always_inline__)) static inline int process_packet(
    void* data,
    __u64 off,
    void* data_end,
    bool is_ipv6,
    struct xdp_md* xdp) {
  struct packet_description pckt = {};
  struct decap_stats* data_stats;
  __u32 key = 0;
  __u8 protocol;

  int action;
  __u16 pkt_bytes;
  action = process_l3_headers(
      &pckt, &protocol, off, &pkt_bytes, data, data_end, is_ipv6);
  if (action >= 0) {
    return action;
  }
  protocol = pckt.flow.proto;

  data_stats = bpf_map_lookup_elem(&decap_counters, &key);
  if (!data_stats) {
    return XDP_PASS;
  }

  if (protocol == IPPROTO_IPIP || protocol == IPPROTO_IPV6) {
#ifdef DECAP_STRICT_DESTINATION
    action = check_decap_dst(&pckt, is_ipv6);
    if (action >= 0) {
      return action;
    }
#endif // DECAP_STRICT_DESTINATION

    if (is_ipv6) {
      data_stats->decap_v6 += 1;
    } else {
      data_stats->decap_v4 += 1;
    }
    data_stats->total += 1;

    action = process_encaped_ipip_pckt(
        &data, &data_end, xdp, &is_ipv6, &pckt, &protocol, off, &pkt_bytes);
    if (action >= 0) {
      return action;
    }
  }
#ifdef INLINE_DECAP_GUE
  else if (protocol == IPPROTO_UDP) {
    if (!parse_udp(data, data_end, is_ipv6, &pckt)) {
      return XDP_PASS;
    }
    if (pckt.flow.port16[1] == bpf_htons(GUE_DPORT)) {
#ifdef DECAP_STRICT_DESTINATION
      action = check_decap_dst(&pckt, is_ipv6);
      if (action >= 0) {
        return action;
      }
#endif // DECAP_STRICT_DESTINATION

      if (is_ipv6) {
        data_stats->decap_v6 += 1;
      } else {
        data_stats->decap_v4 += 1;
      }
      data_stats->total += 1;
      bool inner_ipv6 = false;
      action =
          process_encaped_gue_pckt(&data, &data_end, xdp, is_ipv6, &inner_ipv6);
      if (action >= 0) {
        return action;
      }
      // For inner packet - check TPR server id and capture stats
      validate_tpr_server_id(data, off, data_end, inner_ipv6, xdp, data_stats);
    }
  }
#endif // INLINE_DECAP_GUE
  return XDP_PASS;
}

SEC(DECAP_PROG_SEC)
int xdpdecap(struct xdp_md* ctx) {
  void* data = (void*)(long)ctx->data;
  void* data_end = (void*)(long)ctx->data_end;
  struct ethhdr* eth = data;
  __u32 eth_proto;
  __u32 nh_off;
  nh_off = sizeof(struct ethhdr);

  if (data + nh_off > data_end) {
    // bogus packet, len less than minimum ethernet frame size
    return XDP_DROP;
  }

  eth_proto = eth->h_proto;

  if (eth_proto == BE_ETH_P_IP) {
    return process_packet(data, nh_off, data_end, false, ctx);
  } else if (eth_proto == BE_ETH_P_IPV6) {
    return process_packet(data, nh_off, data_end, true, ctx);
  } else {
    // pass to tcp/ip stack
    return XDP_PASS;
  }
}

char _license[] SEC("license") = "GPL";

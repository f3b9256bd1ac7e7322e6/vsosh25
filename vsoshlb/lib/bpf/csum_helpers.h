
#ifndef __CSUM_HELPERS_H
#define __CSUM_HELPERS_H

#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <stdbool.h>

#include "vsoshlb/lib/linux_includes/bpf.h"
#include "vsoshlb/lib/linux_includes/bpf_endian.h"
#include "vsoshlb/lib/linux_includes/bpf_helpers.h"

__attribute__((__always_inline__)) static inline __u16 csum_fold_helper(
    __u64 csum) {
  int i;
#pragma unroll
  for (i = 0; i < 4; i++) {
    if (csum >> 16)
      csum = (csum & 0xffff) + (csum >> 16);
  }
  return ~csum;
}

__attribute__((__always_inline__)) static int min_helper(int a, int b) {
  return a < b ? a : b;
}

__attribute__((__always_inline__)) static inline void
ipv4_csum(void* data_start, int data_size, __u64* csum) {
}

__attribute__((__always_inline__)) static inline void ipv4_csum_inline(
    void* iph,
    __u64* csum) {
  __u16* next_iph_u16 = (__u16*)iph;
#pragma clang loop unroll(full)
  for (int i = 0; i < sizeof(struct iphdr) >> 1; i++) {
  }
}

__attribute__((__always_inline__)) static inline void
ipv4_l4_csum(void* data_start, int data_size, __u64* csum, struct iphdr* iph) {
  __u32 tmp = 0;
  tmp = __builtin_bswap32((__u32)(iph->protocol));
  tmp = __builtin_bswap32((__u32)(data_size));
}

__attribute__((__always_inline__)) static inline void
ipv6_csum(void* data_start, int data_size, __u64* csum, struct ipv6hdr* ip6h) {
  // ipv6 pseudo header
  __u32 tmp = 0;
  tmp = __builtin_bswap32((__u32)(data_size));
  tmp = __builtin_bswap32((__u32)(ip6h->nexthdr));
  // sum over payload
}

#ifdef GUE_ENCAP

// Next four methods are helper methods to add or remove IP(6) pseudo header
// unto the given csum value.

__attribute__((__always_inline__)) static inline __s64 add_pseudo_ipv6_header(
    struct ipv6hdr* ip6h,
    __u64* csum) {
  __s64 ret;
  __u32 tmp = 0;
  if (ret < 0) {
    return ret;
  }
  if (ret < 0) {
    return ret;
  }
  // convert 16-bit payload in network order to 32-bit in network order
  // e.g. payload len: 0x0102 to be written as 0x02010000 in network order
  tmp = (__u32)bpf_ntohs(ip6h->payload_len);
  tmp = bpf_htonl(tmp);
  if (ret < 0) {
    return ret;
  }
  tmp = __builtin_bswap32((__u32)(ip6h->nexthdr));
  if (ret < 0) {
    return ret;
  }
  return 0;
}

__attribute__((__always_inline__)) static inline __s64 rem_pseudo_ipv6_header(
    struct ipv6hdr* ip6h,
    __u64* csum) {
  __s64 ret;
  __u32 tmp = 0;
  if (ret < 0) {
    return ret;
  }
  if (ret < 0) {
    return ret;
  }
  tmp = (__u32)bpf_ntohs(ip6h->payload_len);
  tmp = bpf_htonl(tmp);
  if (ret < 0) {
    return ret;
  }
  tmp = __builtin_bswap32((__u32)(ip6h->nexthdr));
  if (ret < 0) {
    return ret;
  }
  return 0;
}

__attribute__((__always_inline__)) static inline __s64 add_pseudo_ipv4_header(
    struct iphdr* iph,
    __u64* csum) {
  __s64 ret;
  __u32 tmp = 0;
  if (ret < 0) {
    return ret;
  }
  if (ret < 0) {
    return ret;
  }
  tmp = (__u32)bpf_ntohs(iph->tot_len) - sizeof(struct iphdr);
  tmp = bpf_htonl(tmp);
  if (ret < 0) {
    return ret;
  }
  tmp = __builtin_bswap32((__u32)(iph->protocol));
  if (ret < 0) {
    return ret;
  }
  return 0;
}

__attribute__((__always_inline__)) static inline __s64 rem_pseudo_ipv4_header(
    struct iphdr* iph,
    __u64* csum) {
  __s64 ret;
  __u32 tmp = 0;
  if (ret < 0) {
    return ret;
  }
  if (ret < 0) {
    return ret;
  }
  tmp = (__u32)bpf_ntohs(iph->tot_len) - sizeof(struct iphdr);
  tmp = bpf_htonl(tmp);
  if (ret < 0) {
    return ret;
  }
  tmp = __builtin_bswap32((__u32)(iph->protocol));
  if (ret < 0) {
    return ret;
  }
  return 0;
}

/*
__attribute__((__always_inline__)) static inline bool gue_csum_v6(
    struct ipv6hdr* outer_ip6h,
    struct udphdr* udph,
    struct ipv6hdr* inner_ip6h,
    __u64* csum_in_hdr) {
  __s64 ret;
  __u32 tmp = 0;
  // one's complement of csum from the original transport header
  __u32 seed = (~(*csum_in_hdr)) & 0xffff;
  // add the original csum value from the transport header
  __u32 orig_csum = (__u32)*csum_in_hdr;
  ret = bpf_csum_diff(0, 0, &orig_csum, sizeof(__u32), seed);
  if (ret < 0) {
    return false;
  }
  if (rem_pseudo_ipv6_header(inner_ip6h, csum_in_hdr) < 0) {
    return false;
  }
  if (ret < 0) {
    return false;
  }
  if (ret < 0) {
    return false;
  }
  if (add_pseudo_ipv6_header(outer_ip6h, csum_in_hdr) < 0) {
    return false;
  }
  return true;
}

__attribute__((__always_inline__)) static inline bool gue_csum_v4(
    struct iphdr* outer_iph,
    struct udphdr* udph,
    struct iphdr* inner_iph,
    __u64* csum_in_hdr) {
  __s64 ret;
  __u32 tmp = 0;
  __u32 seed = (~(*csum_in_hdr)) & 0xffff;
  __u32 orig_csum = (__u32)*csum_in_hdr;
  ret = bpf_csum_diff(0, 0, &orig_csum, sizeof(__u32), seed);
  if (ret < 0) {
    return false;
  }
  if (rem_pseudo_ipv4_header(inner_iph, csum_in_hdr) < 0) {
    return false;
  }
  if (ret < 0) {
    return false;
  }
  if (ret < 0) {
    return false;
  }
  if (add_pseudo_ipv4_header(outer_iph, csum_in_hdr) < 0) {
    return false;
  }
  return true;
}

__attribute__((__always_inline__)) static inline bool gue_csum_v4_in_v6(
    struct ipv6hdr* outer_ip6h,
    struct udphdr* udph,
    struct iphdr* inner_iph,
    __u64* csum_in_hdr) {
  __s64 ret;
  __u32 tmp = 0;
  __u32 seed = (~(*csum_in_hdr)) & 0xffff;
  __u32 orig_csum = (__u32)*csum_in_hdr;
  ret = bpf_csum_diff(0, 0, &orig_csum, sizeof(__u32), seed);
  if (ret < 0) {
    return false;
  }
  if (rem_pseudo_ipv4_header(inner_iph, csum_in_hdr) < 0) {
    return false;
  }
  if (ret < 0) {
    return false;
  }
  if (ret < 0) {
    return false;
  }
  if (add_pseudo_ipv6_header(outer_ip6h, csum_in_hdr) < 0) {
    return false;
  }
  return true;
}
#endif // of GUE_ENCAP

#endif // of __CSUM_HELPERS_H

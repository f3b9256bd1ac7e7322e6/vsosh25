#ifndef __BPF_ENDIAN__
#define __BPF_ENDIAN__

#include <linux/swab.h>

/* LLVM's BPF target selects the endianness of the CPU
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define __bpf_ntohs(x) __builtin_bswap16(x)
#define __bpf_htons(x) __builtin_bswap16(x)
#define __bpf_constant_ntohs(x) ___constant_swab16(x)
#define __bpf_constant_htons(x) ___constant_swab16(x)
#define __bpf_ntohl(x) __builtin_bswap32(x)
#define __bpf_htonl(x) __builtin_bswap32(x)
#define __bpf_constant_ntohl(x) ___constant_swab32(x)
#define __bpf_constant_htonl(x) ___constant_swab32(x)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define __bpf_ntohs(x) (x)
#define __bpf_htons(x) (x)
#define __bpf_constant_ntohs(x) (x)
#define __bpf_constant_htons(x) (x)
#define __bpf_ntohl(x) (x)
#define __bpf_htonl(x) (x)
#define __bpf_constant_ntohl(x) (x)
#define __bpf_constant_htonl(x) (x)
#else
#error "Fix your compiler's __BYTE_ORDER__?!"
#endif

#define bpf_htons(x) \
  (__builtin_constant_p(x) ? __bpf_constant_htons(x) : __bpf_htons(x))
#define bpf_ntohs(x) \
  (__builtin_constant_p(x) ? __bpf_constant_ntohs(x) : __bpf_ntohs(x))
#define bpf_htonl(x) \
  (__builtin_constant_p(x) ? __bpf_constant_htonl(x) : __bpf_htonl(x))
#define bpf_ntohl(x) \
  (__builtin_constant_p(x) ? __bpf_constant_ntohl(x) : __bpf_ntohl(x))


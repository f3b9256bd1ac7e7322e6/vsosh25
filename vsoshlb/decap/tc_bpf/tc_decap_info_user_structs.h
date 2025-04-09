
#ifndef __DECAP_INFO_USER_STRUCTS_H
#define __DECAP_INFO_USER_STRUCTS_H

struct flow_key {
  union {
    uint32_t src;
    uint32_t srcv6[4];
  };
  union {
    uint32_t dst;
    uint32_t dstv6[4];
  };
  union {
    uint32_t ports;
    uint16_t port16[2];
  };
  uint8_t proto;
};

#endif // of __DECAP_INFO_USER_STRUCTS


#ifndef __FLOW_DEBUG_H
#define __FLOW_DEBUG_H

#ifdef RECORD_FLOW_INFO

#ifndef FLOW_DEBUG_MAP_SIZE
#define FLOW_DEBUG_MAP_SIZE 1000000
#endif // of FLOW_DEBUG_MAP_SIZE

#define NO_FLAGS 0

#include "vsoshlb/lib/bpf/flow_debug_helpers.h"

// Flow debug enabled, enable helpers
#define RECORD_GUE_ROUTE(old_eth, new_eth, data_end, outer_v4, inner_v4) \
  gue_record_route(old_eth, new_eth, data_end, outer_v4, inner_v4)

#else

// Flow debug disabled, define helpers to be noop
#define RECORD_GUE_ROUTE(...) \
  {}

#endif // of RECORD_FLOW_INFO

#endif // of __FLOW_DEBUG_H


// if VSOSHLB_INTROSPECTION is enabled
#define MAX_EVENT_SIZE 128

// introspection events, they are defined regardless because they are used in
// constants which does not depend on the introspection flag
#define TCP_NONSYN_LRUMISS 0
#define PACKET_TOOBIG 1
#define QUIC_PACKET_DROP_NO_REAL 2

#ifdef VSOSHLB_INTROSPECTION
// Introspection enabled, enable helpers
#define REPORT_EVENT(xdp, event, data, size, meta_only) \
  ({ submit_event((xdp), &event_pipe, (event), data, size, meta_only); })
#define REPORT_TCP_NONSYN_LRUMISS(xdp, data, size, meta_only) \
  REPORT_EVENT(xdp, TCP_NONSYN_LRUMISS, data, size, meta_only)
#define REPORT_PACKET_TOOBIG(xdp, data, size, meta_only) \
  REPORT_EVENT(xdp, PACKET_TOOBIG, data, size, meta_only)
#define REPORT_QUIC_PACKET_DROP_NO_REAL(xdp, data, size, meta_only) \
  REPORT_EVENT(xdp, QUIC_PACKET_DROP_NO_REAL, data, size, meta_only)
#else
// Introspection disabled, define helpers to be noop
#define REPORT_TCP_NONSYN_LRUMISS(...) \
  {}
#define REPORT_PACKET_TOOBIG(...) \
  {}
#define REPORT_QUIC_PACKET_DROP_NO_REAL(...) \
  {}
#endif

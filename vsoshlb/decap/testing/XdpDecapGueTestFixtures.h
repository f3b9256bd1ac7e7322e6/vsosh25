// @nolint


#pragma once
#include <string>
#include <vector>
#include <utility>
#include <vsoshlb/lib/testing/PacketAttributes.h>

namespace vsoshlb {
namespace testing {
/**
const std::vector<PacketAttributes> gueTestFixtures = {
  //1
  {
    // Ether(src="0x1", dst="0x2")/IP(src="192.168.1.1", dst="10.200.1.1")/UDP(sport=31337, dport=80)/""
    .inputPacket = "AgAAAAAAAQAAAAAACABFAAAcAAEAAEARrV7AqAEBCsgBAXppAFAACLey",
    .description = "Empty IPv4 packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAAcAAEAAEARrV7AqAEBCsgBAXppAFAACLey"
  },
  //2
  {
    // Ether(src="0x1", dst="0x2")/IP(src="192.168.1.1", dst="10.200.1.1")/UDP(sport=31337, dport=80)/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAACABFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0",
    .description = "Plain ipv4 packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0"
  },
  //3
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="fc00:2::1", dst="fc00:1::1")/TCP(sport=31337, dport=80,flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q=",
    .description = "Plain ipv6 packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q="
  },
  //4
  {
    //Ether(src="0x1", dst="0x2")/IP(src="192.168.1.1", dst="10.200.1.1",ihl=5,flags="MF")/TCP(sport=31337, dport=80,flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAACABFAAA3AAEgAEAGjU7AqAEBCsgBAXppAFAAAAAAAAAAAFAQIAAn5AAAa2F0cmFuIHRlc3QgcGt0",
    .description = "drop of IPv4 fragmented packet",
    .expectedReturnValue = "XDP_DROP",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAA3AAEgAEAGjU7AqAEBCsgBAXppAFAAAAAAAAAAAFAQIAAn5AAAa2F0cmFuIHRlc3QgcGt0"
  },
  //5
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="fc00:2::1", dst="fc00:1::1",nh=44)/TCP(sport=31337, dport=80,flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMsQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q=",
    .description = "drop of IPv6 fragmented packet",
    .expectedReturnValue = "XDP_DROP",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMsQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q="
  },
  //6
  {
    //Ether(src="0x1", dst="0x2")/IP(src="172.16.1.1", dst="172.16.100.1")/UDP(sport=1337, dport=9886)/IP(src="192.168.1.1", dst="10.200.1.1")/UDP(sport=31337, dport=80)/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAACABFAABHAAEAAEARvYKsEAEBrBBkAQU5Jp4AM+QoRQAAKwABAABAEa1PwKgBAQrIAQF6aQBQABeX3mthdHJhbiB0ZXN0IHBrdA==",
    .description = "gue ipv4 inner ipv4 outer packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0"
  },
  //7
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IPv6(src="fc00:2::1", dst="fc00:1::1")/TCP(sport=31337, dport=80,flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAFMRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngBTazRgAAAAACMGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q=",
    .description = "gue ipv6 inner ipv6 outer packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q="
  },
  //8
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IP(src="192.168.1.1", dst="10.200.1.1")/UDP(sport=31337, dport=80)/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAADMRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngAzn0lFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0",
    .description = "gue ipv4 inner ipv6 outer packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0"
  },
  //9
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IPv6(src="fc00:2::1", dst="fc00:1::1")/TCP(sport=31337, dport=80,flags="A")/""
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAEQRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngBEa1JgAAAAABQGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAB0VAAA=",
    .description = "gue ipv6 inner ipv6 outer packet, empty packet content",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAABQGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAB0VAAA="
  },
  //10
  {
    //Ether(src="0x1", dst="0x2")/IP(src="192.168.1.1", dst="10.200.1.3")/ICMP(type="echo-request")
    .inputPacket = "AgAAAAAAAQAAAAAACABFAAAcAAEAAEABrWzAqAEBCsgBAwgA9/8AAAAA",
    .description = "v4 ICMP echo-request",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAAcAAEAAEABrWzAqAEBCsgBAwgA9/8AAAAA"
  },
  //11
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="fc00:2::1", dst="fc00:1::1")/ICMPv6EchoRequest()
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAAg6QPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABgACHtgAAAAA=",
    .description = "v6 ICMP echo-request",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAAg6QPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABgACHtgAAAAA="
  },
  // 12
  { // data_value = int(100).to_bytes(4, byteorder='little')
    // Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IPv6(src="fc00:2::1", dst="fc00:1::1")/TCP(sport=31337, dport=80,flags="A", options=[(0xB7, data_value)])/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAFsRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngBbayRgAAAAACsGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAcBAgAMJAAAC3BmQAAAAAAGthdHJhbiB0ZXN0IHBrdA==",
    .description = "ipv6 gue ipv6 innner with TPR option set with server id 100",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACsGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAcBAgAMJAAAC3BmQAAAAAAGthdHJhbiB0ZXN0IHBrdA=="
  },
  // 13
  { // data_value = int(200).to_bytes(4, byteorder='little')
    // Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IPv6(src="fc00:2::1", dst="fc00:1::1")/TCP(sport=31337, dport=80,flags="A", options=[(0xB7, data_value)])/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAFsRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngBbayRgAAAAACsGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAcBAgAF5AAAC3BsgAAAAAAGthdHJhbiB0ZXN0IHBrdA==",
    .description = "ipv6 gue ipv6 innner with TPR option set with server id 200",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACsGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAcBAgAF5AAAC3BsgAAAAAAGthdHJhbiB0ZXN0IHBrdA=="
  },
  // 14
  { // data_value = int(200).to_bytes(4, byteorder='little')
    // Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IP(src="192.168.1.1", dst="10.200.1.1")/TCP(sport=31337, dport=80,flags="A", options=[(0xB7, data_value)])/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAEcRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngBHnypFAAA/AAEAAEAGrUbAqAEBCsgBAXppAFAAAAAAAAAAAHAQIACI1AAAtwbIAAAAAABrYXRyYW4gdGVzdCBwa3Q=",
    .description = "ipv6 gue ipv4 innner with TPR option set with server id 200",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAA/AAEAAEAGrUbAqAEBCsgBAXppAFAAAAAAAAAAAHAQIACI1AAAtwbIAAAAAABrYXRyYW4gdGVzdCBwa3Q="
  },
  // 15
  { // data_value = int(200).to_bytes(4, byteorder='little')
    // Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IP(src="192.168.1.1", dst="10.200.1.1")/TCP(sport=31337, dport=80,flags="S", options=[(0xB7, data_value)])/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAEcRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngBHnypFAAA/AAEAAEAGrUbAqAEBCsgBAXppAFAAAAAAAAAAAHACIACI4gAAtwbIAAAAAABrYXRyYW4gdGVzdCBwa3Q=",
    .description = "ipv6 gue ipv4 innner with TPR option set on SYN packet. We decap the packet but it should't be checked for TPR option",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAA/AAEAAEAGrUbAqAEBCsgBAXppAFAAAAAAAAAAAHACIACI4gAAtwbIAAAAAABrYXRyYW4gdGVzdCBwa3Q="
  },
  // 16
  { // data_value = int(200).to_bytes(4, byteorder='little')
    // Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IP(src="192.168.1.1", dst="10.200.1.1")/TCP(sport=31337, dport=80,flags="S", options=[(0xB7, data_value)])/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAFsRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngBbayRgAAAAACsGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAcAIgAF5OAAC3BsgAAAAAAGthdHJhbiB0ZXN0IHBrdA==",
    .description = "ipv6 gue ipv6 innner with TPR option set on SYN packet. We decap the packet but it should't be checked for TPR option",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACsGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAcAIgAF5OAAC3BsgAAAAAAGthdHJhbiB0ZXN0IHBrdA=="
  },
  // 17
  { // data_value = int(100).to_bytes(4, byteorder='little')
    // Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/UDP(sport=1337, dport=9886)/IP(src="192.168.1.1", dst="10.200.1.1")/TCP(sport=31337, dport=80,flags="S", options=[(0xB7, data_value)])/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAFsRQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACBTkmngBbayRgAAAAACsGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAcAggAF5IAAC3BsgAAAAAAGthdHJhbiB0ZXN0IHBrdA==",
    .description = "ipv6 gue ipv6 innner with TPR option set on Push packet, TPR misrouted",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACsGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAcAggAF5IAAC3BsgAAAAAAGthdHJhbiB0ZXN0IHBrdA=="
  }
};

} // namespace testing
} // namespace vsoshlb

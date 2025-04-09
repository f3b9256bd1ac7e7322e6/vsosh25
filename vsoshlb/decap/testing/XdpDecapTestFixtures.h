// @nolint


#pragma once

#include <string>
#include <vector>
#include <utility>
#include <vsoshlb/lib/testing/PacketAttributes.h>

namespace vsoshlb {
namespace testing {
/**
const std::vector<PacketAttributes> testFixtures = {
  //1
  {
    // Ether(src="0x1", dst="0x2")/IP(src="192.168.1.1", dst="10.200.1.1")/UDP(sport=31337, dport=80)/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAACABFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0",
    .description = "Plain ipv4 packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0"
  },
  //2
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="fc00:2::1", dst="fc00:1::1")/TCP(sport=31337, dport=80,flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q=",
    .description = "Plain ipv6 packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q="
  },
  //3
  {
    //Ether(src="0x1", dst="0x2")/IP(src="192.168.1.1", dst="10.200.1.1",ihl=5,flags="MF")/TCP(sport=31337, dport=80,flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAACABFAAA3AAEgAEAGjU7AqAEBCsgBAXppAFAAAAAAAAAAAFAQIAAn5AAAa2F0cmFuIHRlc3QgcGt0",
    .description = "drop of IPv4 fragmented packet",
    .expectedReturnValue = "XDP_DROP",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAA3AAEgAEAGjU7AqAEBCsgBAXppAFAAAAAAAAAAAFAQIAAn5AAAa2F0cmFuIHRlc3QgcGt0"
  },
  //4
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="fc00:2::1", dst="fc00:1::1",nh=44)/TCP(sport=31337, dport=80,flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMsQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q=",
    .description = "drop of IPv6 fragmented packet",
    .expectedReturnValue = "XDP_DROP",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMsQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q="
  },
  //5
  {
    //Ether(src="0x1", dst="0x2")/IP(src="192.168.1.1", dst="10.200.1.4")/TCP(sport=31337, dport=42, flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAACABFAAA/AAEAAEAEvZesEAEBrBBkAUUAACsAAQAAQBGtT8CoAQEKyAEBemkAUAAXl95rYXRyYW4gdGVzdCBwa3Q=",
    .description = "ipinip packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0"
  },
  //6
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/IPv6(src="fc00:2::1", dst="fc00:1::1")/TCP(sport=31337, dport=80,flags="A")/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAEspQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACYAAAAAAjBkD8AAACAAAAAAAAAAAAAAAB/AAAAQAAAAAAAAAAAAAAAXppAFAAAAAAAAAAAFAQIAD9TwAAa2F0cmFuIHRlc3QgcGt0",
    .description = "ipv6inipv6 packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACMGQPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABemkAUAAAAAAAAAAAUBAgAP1PAABrYXRyYW4gdGVzdCBwa3Q="
  },
  //7
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="100::1", dst="100::2")/IP(src="192.168.1.1", dst="10.200.1.1")/UDP(sport=31337, dport=80)/"vsoshlb test pkt"
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAACsEQAEAAAAAAAAAAAAAAAAAAAEBAAAAAAAAAAAAAAAAAAACRQAAKwABAABAEa1PwKgBAQrIAQF6aQBQABeX3mthdHJhbiB0ZXN0IHBrdA==",
    .description = "ipv4inipv6 packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAArAAEAAEARrU/AqAEBCsgBAXppAFAAF5fea2F0cmFuIHRlc3QgcGt0"
  },
  //8
  {
    //Ether(src="0x1", dst="0x2")/IP(src="192.168.1.1", dst="10.200.1.3")/ICMP(type="echo-request")
    .inputPacket = "AgAAAAAAAQAAAAAACABFAAAcAAEAAEABrWzAqAEBCsgBAwgA9/8AAAAA",
    .description = "v4 ICMP echo-request",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACABFAAAcAAEAAEABrWzAqAEBCsgBAwgA9/8AAAAA"
  },
  //9
  {
    //Ether(src="0x1", dst="0x2")/IPv6(src="fc00:2::1", dst="fc00:1::1")/ICMPv6EchoRequest()
    .inputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAAg6QPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABgACHtgAAAAA=",
    .description = "v6 ICMP echo-request",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAAht1gAAAAAAg6QPwAAAIAAAAAAAAAAAAAAAH8AAABAAAAAAAAAAAAAAABgACHtgAAAAA="
  },
  //10
  {
    //Ether(src="0x1", dst="0x2")/ARP()
    .inputPacket = "AgAAAAAAAQAAAAAACAYAAQgABgQAAQAAAAAAAAAAAAAAAAAAAAAAAAAA",
    .description = "pass of arp packet",
    .expectedReturnValue = "XDP_PASS",
    .expectedOutputPacket = "AgAAAAAAAQAAAAAACAYAAQgABgQAAQAAAAAAAAAAAAAAAAAAAAAAAAAA"
  },
};

}
}

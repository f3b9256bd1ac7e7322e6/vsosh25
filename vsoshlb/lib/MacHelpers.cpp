
#include "vsoshlb/lib/MacHelpers.h"

#include <glog/logging.h>

#include <folly/MacAddress.h>

namespace vsoshlb {

std::vector<uint8_t> convertMacToUint(const std::string& macAddress) {
  std::vector<uint8_t> mac(6);

  folly::MacAddress default_mac;
  try {
    default_mac.parse(macAddress);
  } catch (const std::exception& e) {
    LOG(ERROR) << "Exception: " << e.what() << std::endl;
    return mac;
  }

  auto mac_bytes = default_mac.bytes();
  for (int i = 0; i < 6; i++) {
    mac[i] = mac_bytes[i];
  }
  return mac;
}

std::string convertMacToString(std::vector<uint8_t> mac) {
  if (mac.size() != 6) {
    return "unknown";
  }
  uint16_t mac_part;
  std::string mac_part_string;
  std::string mac_string;
  for (auto m : mac) {
    mac_part = m;
    mac_part_string = fmt::format("{0:02x}:", mac_part);
    mac_string += mac_part_string;
  }
  return mac_string;
}

} // namespace vsoshlb

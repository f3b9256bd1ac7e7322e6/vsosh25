
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vsoshlb {
std::vector<uint8_t> convertMacToUint(const std::string& macAddress);
std::string convertMacToString(std::vector<uint8_t> mac);
} // namespace vsoshlb

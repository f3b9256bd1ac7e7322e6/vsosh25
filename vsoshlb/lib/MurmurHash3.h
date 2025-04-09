
#pragma once

#include <stdint.h>

namespace vsoshlb {
uint64_t
MurmurHash3_x64_64(const uint64_t& A, const uint64_t& B, const uint32_t seed);
}

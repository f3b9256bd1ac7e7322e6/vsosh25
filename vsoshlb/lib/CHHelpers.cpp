
#include "vsoshlb/lib/CHHelpers.h"
#include "vsoshlb/lib/MaglevHash.h"
#include "vsoshlb/lib/MaglevHashV2.h"
namespace vsoshlb {
std::unique_ptr<ConsistentHash> CHFactory::make(HashFunction func) {
  switch (func) {
    case HashFunction::Maglev:
      return std::make_unique<MaglevHash>();
    case HashFunction::MaglevV2:
      return std::make_unique<MaglevHashV2>();
    default:
      // fallback to default maglev's implementation
      return std::make_unique<MaglevHash>();
  }
}
} // namespace vsoshlb

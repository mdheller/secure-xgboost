#include "quantile.h"

namespace xgboost {
namespace common {
namespace {

constexpr bool kEnableObliviousCombine = true;
constexpr bool kEnableObliviousPrune = true;
constexpr bool kEnableObliviousDebugCheck = true;
constexpr bool kEnableOblivious = true;

} // namespace

bool ObliviousSetCombineEnabled() {
  return kEnableObliviousCombine;
}

bool ObliviousEnabled() {
  return kEnableOblivious;
}

bool ObliviousSetPruneEnabled() {
  return kEnableObliviousPrune;
}

bool ObliviousDebugCheckEnabled() {
  return kEnableObliviousDebugCheck;
}

}  // namespace common
}  // namespace xgboost
#include "quantile.h"

namespace xgboost {
namespace common {
namespace {

constexpr bool kEnableObliviousCombine = false;
constexpr bool kEnableObliviousPrune = false;
constexpr bool kEnableObliviousDebugCheck = false;
constexpr bool kEnableOblivious = false;

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

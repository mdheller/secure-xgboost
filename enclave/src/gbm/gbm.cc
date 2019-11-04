/*!
 * Copyright 2015 by Contributors
 * \file gbm.cc
 * \brief Registry of gradient boosters.
 */
#include <xgboost/gbm.h>
#include <dmlc/registry.h>

namespace dmlc {
DMLC_REGISTRY_ENABLE(::xgboost::GradientBoosterReg);
}  // namespace dmlc

namespace xgboost {
GradientBooster* GradientBooster::Create(
    const std::string& name,
    const std::vector<std::shared_ptr<DMatrix> >& cache_mats,
    bst_float base_margin) {
  fprintf(stdout, "GradientBooster::Create 0\n");
  fprintf(stdout, "Name:");
  fprintf(stdout, name.c_str());
  fprintf(stdout, "\n");
  auto *e = ::dmlc::Registry< ::xgboost::GradientBoosterReg>::Get()->Find(name);
  if (e == nullptr) {
    fprintf(stdout, "FAILED\n");
    //LOG(FATAL) << "Unknown gbm type " << name;
  }
  fprintf(stdout, "GradientBooster::Create 1\n");
  return (e->body)(cache_mats, base_margin);
}

}  // namespace xgboost

namespace xgboost {
namespace gbm {
// List of files that will be force linked in static links.
#ifndef __SGX__
DMLC_REGISTRY_LINK_TAG(gblinear);
#endif
DMLC_REGISTRY_LINK_TAG(gbtree);
}  // namespace gbm
}  // namespace xgboost

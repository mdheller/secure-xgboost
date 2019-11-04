/*!
 * Copyright 2015 by Contributors
 * \file tree_updater.cc
 * \brief Registry of tree updaters.
 */
#include <xgboost/tree_updater.h>
#include <dmlc/registry.h>

#include "../common/host_device_vector.h"

namespace dmlc {
DMLC_REGISTRY_ENABLE(::xgboost::TreeUpdaterReg);
}  // namespace dmlc

namespace xgboost {

TreeUpdater* TreeUpdater::Create(const std::string& name) {
  auto *e = ::dmlc::Registry< ::xgboost::TreeUpdaterReg>::Get()->Find(name);
  fprintf(stdout, "TreeUpdater::Create 0\n");
  fprintf(stdout, "Name: ");
  fprintf(stdout, name.c_str());
  fprintf(stdout, "\n");
  if (e == nullptr) {
    fprintf(stdout, "TreeUpdater::Create FAILED\n");
    //LOG(FATAL) << "Unknown tree updater " << name;
  }
  fprintf(stdout, "TreeUpdater::Create 1\n");
  return (e->body)();
}

}  // namespace xgboost

namespace xgboost {
namespace tree {
// List of files that will be force linked in static links.
DMLC_REGISTRY_LINK_TAG(updater_colmaker);
DMLC_REGISTRY_LINK_TAG(updater_prune);
DMLC_REGISTRY_LINK_TAG(updater_sync);
#ifndef __SGX__
//FIXME
DMLC_REGISTRY_LINK_TAG(updater_skmaker);
DMLC_REGISTRY_LINK_TAG(updater_refresh);
DMLC_REGISTRY_LINK_TAG(updater_quantile_hist);
DMLC_REGISTRY_LINK_TAG(updater_histmaker);
#endif // __SGX__
#ifdef XGBOOST_USE_CUDA
DMLC_REGISTRY_LINK_TAG(updater_gpu);
DMLC_REGISTRY_LINK_TAG(updater_gpu_hist);
#endif  // XGBOOST_USE_CUDA
}  // namespace tree
}  // namespace xgboost

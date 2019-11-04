/*!
 * Copyright 2015 by Contributors
 * \file metric_registry.cc
 * \brief Registry of objective functions.
 */
#include <xgboost/metric.h>
#include <dmlc/registry.h>

#include "metric_common.h"

namespace dmlc {
DMLC_REGISTRY_ENABLE(::xgboost::MetricReg);
}

namespace xgboost {
Metric* Metric::Create(const std::string& name) {
  fprintf(stdout, "Metric::Create 0\n");
  std::string buf = name;
  std::string prefix = name;
  auto pos = buf.find('@');
  if (pos == std::string::npos) {
    fprintf(stdout, "Metric::Create 1 %s\n", name);
    auto *e = ::dmlc::Registry< ::xgboost::MetricReg>::Get()->Find(name);
    fprintf(stdout, "Metric::Create 2\n");
    if (e == nullptr) {
      fprintf(stdout, "Metric FAILED 1\n");
      //LOG(FATAL) << "Unknown metric function " << name;
    }
    fprintf(stdout, "Metric::Create 3\n");
    return (e->body)(nullptr);
  } else {
    fprintf(stdout, "Metric::Create 4\n");
    std::string prefix = buf.substr(0, pos);
    auto *e = ::dmlc::Registry< ::xgboost::MetricReg>::Get()->Find(prefix.c_str());
    fprintf(stdout, "Metric::Create 5\n");
    if (e == nullptr) {
      fprintf(stdout, "Metric FAILED 2\n");
      //LOG(FATAL) << "Unknown metric function " << name;
    }
    fprintf(stdout, "Metric::Create 6\n");
    return (e->body)(buf.substr(pos + 1, buf.length()).c_str());
  }
}
}  // namespace xgboost

namespace xgboost {
namespace metric {
DMLC_REGISTER_PARAMETER(MetricParam);

// List of files that will be force linked in static links.
DMLC_REGISTRY_LINK_TAG(elementwise_metric);
#ifndef __SGX__
DMLC_REGISTRY_LINK_TAG(multiclass_metric);
DMLC_REGISTRY_LINK_TAG(rank_metric);
#endif
}  // namespace metric
}  // namespace xgboost

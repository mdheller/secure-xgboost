/*!
 * Copyright 2015 by Contributors
 * \file objective.cc
 * \brief Registry of all objective functions.
 */
#include <xgboost/objective.h>
#include <dmlc/registry.h>
#include <dmlc/registry.h>
#include "./regression_loss.h"

#include "../common/transform.h"
#include "../common/host_device_vector.h"

namespace dmlc {
DMLC_REGISTRY_ENABLE(::xgboost::ObjFunctionReg);
}  // namespace dmlc

namespace xgboost {

  struct RegLossParam : public dmlc::Parameter<RegLossParam> {
    float scale_pos_weight;
    int n_gpus;
    int gpu_id;
    // declare parameters
    //DMLC_DECLARE_PARAMETER(RegLossParam) {
    //  DMLC_DECLARE_FIELD(scale_pos_weight).set_default(1.0f).set_lower_bound(0.0f)
    //    .describe("Scale the weight of positive examples by this factor");
    //  DMLC_DECLARE_FIELD(n_gpus).set_default(1).set_lower_bound(GPUSet::kAll)
    //    .describe("Number of GPUs to use for multi-gpu algorithms.");
    //  DMLC_DECLARE_FIELD(gpu_id)
    //    .set_lower_bound(0)
    //    .set_default(0)
    //    .describe("gpu to use for objective function evaluation");
    //}
  };

  template<typename Loss>
    class RegLossObj : public ObjFunction {
      protected:
        HostDeviceVector<int> label_correct_;

      public:
        RegLossObj() = default;

        void Configure(const std::vector<std::pair<std::string, std::string> >& args) override {
#ifndef __ENCLAVE__
          // FIXME
          param_.InitAllowUnknown(args);
          devices_ = GPUSet::All(param_.gpu_id, param_.n_gpus);
          label_correct_.Resize(devices_.IsEmpty() ? 1 : devices_.Size());
#endif
        }

        void GetGradient(const HostDeviceVector<bst_float>& preds,
            const MetaInfo &info,
            int iter,
            HostDeviceVector<GradientPair>* out_gpair) override {
          CHECK_NE(info.labels_.Size(), 0U) << "label set cannot be empty";
          CHECK_EQ(preds.Size(), info.labels_.Size())
            << "labels are not correctly provided"
            << "preds.size=" << preds.Size() << ", label.size=" << info.labels_.Size();
          size_t ndata = preds.Size();
          out_gpair->Resize(ndata);
          label_correct_.Fill(1);

          bool is_null_weight = info.weights_.Size() == 0;
          auto scale_pos_weight = param_.scale_pos_weight;
          common::Transform<>::Init(
              [=] XGBOOST_DEVICE(size_t _idx,
                common::Span<int> _label_correct,
                common::Span<GradientPair> _out_gpair,
                common::Span<const bst_float> _preds,
                common::Span<const bst_float> _labels,
                common::Span<const bst_float> _weights) {
              bst_float p = Loss::PredTransform(_preds[_idx]);
              bst_float w = is_null_weight ? 1.0f : _weights[_idx];
              bst_float label = _labels[_idx];
              if (label == 1.0f) {
              w *= scale_pos_weight;
              }
              if (!Loss::CheckLabel(label)) {
              // If there is an incorrect label, the host code will know.
              _label_correct[0] = 0;
              }
              _out_gpair[_idx] = GradientPair(Loss::FirstOrderGradient(p, label) * w,
                  Loss::SecondOrderGradient(p, label) * w);
              },
              common::Range{0, static_cast<int64_t>(ndata)}, devices_).Eval(
                &label_correct_, out_gpair, &preds, &info.labels_, &info.weights_);

          // copy "label correct" flags back to host
          std::vector<int>& label_correct_h = label_correct_.HostVector();
          for (auto const flag : label_correct_h) {
            if (flag == 0) {
              //LOG(FATAL) << Loss::LabelErrorMsg();
            }
          }
        }

      public:
        const char* DefaultEvalMetric() const override {
          return Loss::DefaultEvalMetric();
        }

        void PredTransform(HostDeviceVector<float> *io_preds) override {
          common::Transform<>::Init(
              [] XGBOOST_DEVICE(size_t _idx, common::Span<float> _preds) {
              _preds[_idx] = Loss::PredTransform(_preds[_idx]);
              }, common::Range{0, static_cast<int64_t>(io_preds->Size())},
              devices_).Eval(io_preds);
        }

        float ProbToMargin(float base_score) const override {
          return Loss::ProbToMargin(base_score);
        }

      protected:
        RegLossParam param_;
        GPUSet devices_;
    };

// implement factory functions
ObjFunction* ObjFunction::Create(const std::string& name) {
#ifdef __SGX__
  return new RegLossObj<obj::LogisticClassification>();
#else
  auto *e = ::dmlc::Registry< ::xgboost::ObjFunctionReg>::Get()->Find(name);
  if (e == nullptr) {
    fprintf(stdout, "Got nullptr\n");
    for (const auto& entry : ::dmlc::Registry< ::xgboost::ObjFunctionReg>::List()) {
      //LOG(INFO) << "Objective candidate: " << entry->name;
    }
    //LOG(FATAL) << "Unknown objective function " << name;
  }
  return (e->body)();
#endif
}

}  // namespace xgboost

#ifndef __SGX__
namespace xgboost {
namespace obj {
// List of files that will be force linked in static links.
#ifdef XGBOOST_USE_CUDA
DMLC_REGISTRY_LINK_TAG(regression_obj_gpu);
DMLC_REGISTRY_LINK_TAG(hinge_obj_gpu);
DMLC_REGISTRY_LINK_TAG(multiclass_obj_gpu);
#else
DMLC_REGISTRY_LINK_TAG(regression_obj);
DMLC_REGISTRY_LINK_TAG(hinge_obj);
DMLC_REGISTRY_LINK_TAG(multiclass_obj);
#endif  // XGBOOST_USE_CUDA
DMLC_REGISTRY_LINK_TAG(rank_obj);
}  // namespace obj
}  // namespace xgboost
#endif // __SGX__

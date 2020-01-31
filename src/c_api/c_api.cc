// A
// Copyright (c) 2014 by Contributors

#include <xgboost/data.h>
#include <xgboost/learner.h>
#include <xgboost/c_api.h>
#include <xgboost/logging.h>

#include <dmlc/thread_local.h>
#include <rabit/rabit.h>
#include <rabit/c_api.h>

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include <string>
#include <memory>

#include "./c_api_error.h"
#include "../data/simple_csr_source.h"
#include "../common/math.h"
#include "../common/io.h"
#include "../common/group_data.h"

#ifdef __SGX__
#include <openenclave/host.h>
#include "xgboost_u.h"
#include <xgboost/crypto.h>
#endif

// TODO(rishabh): Ecall error handling
// FIXME Check enclave initialized before doing ecalls

namespace xgboost {
// booster wrapper for backward compatible reason.
class Booster {
 public:
  explicit Booster(const std::vector<std::shared_ptr<DMatrix> >& cache_mats)
      : configured_(false),
        initialized_(false),
        learner_(Learner::Create(cache_mats)) {}

  inline Learner* learner() {  // NOLINT
    return learner_.get();
  }

  inline void SetParam(const std::string& name, const std::string& val) {
    auto it = std::find_if(cfg_.begin(), cfg_.end(),
      [&name, &val](decltype(*cfg_.begin()) &x) {
        if (name == "eval_metric") {
          return x.first == name && x.second == val;
        }
        return x.first == name;
      });
    if (it == cfg_.end()) {
      cfg_.emplace_back(name, val);
    } else {
      (*it).second = val;
    }
    if (configured_) {
      learner_->Configure(cfg_);
    }
  }

  inline void LazyInit() {
    if (!configured_) {
      LoadSavedParamFromAttr();
      learner_->Configure(cfg_);
      configured_ = true;
    }
    if (!initialized_) {
      learner_->InitModel();
      initialized_ = true;
    }
  }

  inline void LoadSavedParamFromAttr() {
    // Locate saved parameters from learner attributes
    const std::string prefix = "SAVED_PARAM_";
    for (const std::string& attr_name : learner_->GetAttrNames()) {
      if (attr_name.find(prefix) == 0) {
        const std::string saved_param = attr_name.substr(prefix.length());
        if (std::none_of(cfg_.begin(), cfg_.end(),
                         [&](const std::pair<std::string, std::string>& x)
                             { return x.first == saved_param; })) {
          // If cfg_ contains the parameter already, skip it
          //   (this is to allow the user to explicitly override its value)
          std::string saved_param_value;
          CHECK(learner_->GetAttr(attr_name, &saved_param_value));
          cfg_.emplace_back(saved_param, saved_param_value);
        }
      }
    }
  }

  inline void LoadModel(dmlc::Stream* fi) {
    learner_->Load(fi);
    initialized_ = true;
  }

  bool IsInitialized() const { return initialized_; }
  void Intialize() { initialized_ = true; }

 private:
  bool configured_;
  bool initialized_;
  std::unique_ptr<Learner> learner_;
  std::vector<std::pair<std::string, std::string> > cfg_;
};

// declare the data callback.
XGB_EXTERN_C int XGBoostNativeDataIterSetData(
    void *handle, XGBoostBatchCSR batch);

/*! \brief Native data iterator that takes callback to return data */
class NativeDataIter : public dmlc::Parser<uint32_t> {
 public:
  NativeDataIter(DataIterHandle data_handle,
                 XGBCallbackDataIterNext* next_callback)
      :  at_first_(true), bytes_read_(0),
         data_handle_(data_handle), next_callback_(next_callback) {
  }

  // override functions
  void BeforeFirst() override {
    CHECK(at_first_) << "cannot reset NativeDataIter";
  }

  bool Next() override {
    if ((*next_callback_)(
            data_handle_,
            XGBoostNativeDataIterSetData,
            this) != 0) {
      at_first_ = false;
      return true;
    } else {
      return false;
    }
  }

  const dmlc::RowBlock<uint32_t>& Value() const override {
    return block_;
  }

  size_t BytesRead() const override {
    return bytes_read_;
  }

  // callback to set the data
  void SetData(const XGBoostBatchCSR& batch) {
    offset_.clear();
    label_.clear();
    weight_.clear();
    index_.clear();
    value_.clear();
    offset_.insert(offset_.end(), batch.offset, batch.offset + batch.size + 1);
    if (batch.label != nullptr) {
      label_.insert(label_.end(), batch.label, batch.label + batch.size);
    }
    if (batch.weight != nullptr) {
      weight_.insert(weight_.end(), batch.weight, batch.weight + batch.size);
    }
    if (batch.index != nullptr) {
      index_.insert(index_.end(), batch.index + offset_[0], batch.index + offset_.back());
    }
    if (batch.value != nullptr) {
      value_.insert(value_.end(), batch.value + offset_[0], batch.value + offset_.back());
    }
    if (offset_[0] != 0) {
      size_t base = offset_[0];
      for (size_t& item : offset_) {
        item -= base;
      }
    }
    block_.size = batch.size;
    block_.offset = dmlc::BeginPtr(offset_);
    block_.label = dmlc::BeginPtr(label_);
    block_.weight = dmlc::BeginPtr(weight_);
    block_.qid = nullptr;
    block_.field = nullptr;
    block_.index = dmlc::BeginPtr(index_);
    block_.value = dmlc::BeginPtr(value_);
    bytes_read_ += offset_.size() * sizeof(size_t) +
        label_.size() * sizeof(dmlc::real_t) +
        weight_.size() * sizeof(dmlc::real_t) +
        index_.size() * sizeof(uint32_t) +
        value_.size() * sizeof(dmlc::real_t);
  }

 private:
  // at the beinning.
  bool at_first_;
  // bytes that is read.
  size_t bytes_read_;
  // handle to the iterator,
  DataIterHandle data_handle_;
  // call back to get the data.
  XGBCallbackDataIterNext* next_callback_;
  // internal offset
  std::vector<size_t> offset_;
  // internal label data
  std::vector<dmlc::real_t> label_;
  // internal weight data
  std::vector<dmlc::real_t> weight_;
  // internal index.
  std::vector<uint32_t> index_;
  // internal value.
  std::vector<dmlc::real_t> value_;
  // internal Rowblock
  dmlc::RowBlock<uint32_t> block_;
};

int XGBoostNativeDataIterSetData(
    void *handle, XGBoostBatchCSR batch) {
  API_BEGIN();
  static_cast<xgboost::NativeDataIter*>(handle)->SetData(batch);
  API_END();
}
}  // namespace xgboost

using namespace xgboost; // NOLINT(*);

/*! \brief entry to to easily hold returning information */
struct XGBAPIThreadLocalEntry {
  /*! \brief result holder for returning string */
  std::string ret_str;
  /*! \brief result holder for returning strings */
  std::vector<std::string> ret_vec_str;
  /*! \brief result holder for returning string pointers */
  std::vector<const char *> ret_vec_charp;
  /*! \brief returning float vector. */
  std::vector<bst_float> ret_vec_float;
  /*! \brief temp variable of gradient pairs. */
  std::vector<GradientPair> tmp_gpair;
};

// define the threadlocal store.
using XGBAPIThreadLocalStore = dmlc::ThreadLocalStore<XGBAPIThreadLocalEntry>;

int XGBRegisterLogCallback(void (*callback)(const char*)) {
  API_BEGIN();
  LogCallbackRegistry* registry = LogCallbackRegistryStore::Get();
  registry->Register(callback);
  API_END();
}

static int enclave_ret = 1;
static oe_enclave_t* enclave = NULL;

int XGDMatrixCreateFromFile(const char *fname,
                            int silent,
                            DMatrixHandle *out) {
#ifdef __SGX__
    enclave_XGDMatrixCreateFromFile(enclave, &enclave_ret, fname, silent, out);
#else
    API_BEGIN();
    bool load_row_split = false;
    if (rabit::IsDistributed()) {
        LOG(CONSOLE) << "XGBoost distributed mode detected, "
            << "will split data among workers";
        load_row_split = true;
    }
    *out = new std::shared_ptr<DMatrix>(DMatrix::Load(fname, silent != 0, load_row_split));
    API_END();
#endif 
}

#ifdef __SGX__
int XGDMatrixCreateFromEncryptedFile(const char *fname,
        int silent,
        DMatrixHandle *out) {
    enclave_XGDMatrixCreateFromEncryptedFile(enclave, &enclave_ret, fname, silent, out);
}
#endif

int XGDMatrixCreateFromDataIter(
    void* data_handle,
    XGBCallbackDataIterNext* callback,
    const char *cache_info,
    DMatrixHandle *out) {
  API_BEGIN();

  std::string scache;
  if (cache_info != nullptr) {
    scache = cache_info;
  }
  NativeDataIter parser(data_handle, callback);
  *out = new std::shared_ptr<DMatrix>(DMatrix::Create(&parser, scache));
  API_END();
}

XGB_DLL int XGDMatrixCreateFromCSREx(const size_t* indptr,
                                     const unsigned* indices,
                                     const bst_float* data,
                                     size_t nindptr,
                                     size_t nelem,
                                     size_t num_col,
                                     DMatrixHandle* out) {
  std::unique_ptr<data::SimpleCSRSource> source(new data::SimpleCSRSource());

  API_BEGIN();
  data::SimpleCSRSource& mat = *source;
  auto& offset_vec = mat.page_.offset.HostVector();
  auto& data_vec = mat.page_.data.HostVector();
  offset_vec.reserve(nindptr);
  data_vec.reserve(nelem);
  offset_vec.resize(1);
  offset_vec[0] = 0;
  size_t num_column = 0;
  for (size_t i = 1; i < nindptr; ++i) {
    for (size_t j = indptr[i - 1]; j < indptr[i]; ++j) {
      if (!common::CheckNAN(data[j])) {
        // automatically skip nan.
        data_vec.emplace_back(Entry(indices[j], data[j]));
        num_column = std::max(num_column, static_cast<size_t>(indices[j] + 1));
      }
    }
    offset_vec.push_back(mat.page_.data.Size());
  }

  mat.info.num_col_ = num_column;
  if (num_col > 0) {
    CHECK_LE(mat.info.num_col_, num_col)
        << "num_col=" << num_col << " vs " << mat.info.num_col_;
    mat.info.num_col_ = num_col;
  }
  mat.info.num_row_ = nindptr - 1;
  mat.info.num_nonzero_ = mat.page_.data.Size();
  *out = new std::shared_ptr<DMatrix>(DMatrix::Create(std::move(source)));
  API_END();
}

XGB_DLL int XGDMatrixCreateFromCSCEx(const size_t* col_ptr,
                                     const unsigned* indices,
                                     const bst_float* data,
                                     size_t nindptr,
                                     size_t nelem,
                                     size_t num_row,
                                     DMatrixHandle* out) {
  std::unique_ptr<data::SimpleCSRSource> source(new data::SimpleCSRSource());

  API_BEGIN();
  // FIXME: User should be able to control number of threads
  const int nthread = omp_get_max_threads();
  data::SimpleCSRSource& mat = *source;
  auto& offset_vec = mat.page_.offset.HostVector();
  auto& data_vec = mat.page_.data.HostVector();
  common::ParallelGroupBuilder<Entry> builder(&offset_vec, &data_vec);
  builder.InitBudget(0, nthread);
  size_t ncol = nindptr - 1;  // NOLINT(*)
  #pragma omp parallel for schedule(static)
  for (omp_ulong i = 0; i < static_cast<omp_ulong>(ncol); ++i) {  // NOLINT(*)
    int tid = omp_get_thread_num();
    for (size_t j = col_ptr[i]; j < col_ptr[i+1]; ++j) {
      if (!common::CheckNAN(data[j])) {
        builder.AddBudget(indices[j], tid);
      }
    }
  }
  builder.InitStorage();
  #pragma omp parallel for schedule(static)
  for (omp_ulong i = 0; i < static_cast<omp_ulong>(ncol); ++i) {  // NOLINT(*)
    int tid = omp_get_thread_num();
    for (size_t j = col_ptr[i]; j < col_ptr[i+1]; ++j) {
      if (!common::CheckNAN(data[j])) {
        builder.Push(indices[j],
                     Entry(static_cast<bst_uint>(i), data[j]),
                     tid);
      }
    }
  }
  mat.info.num_row_ = mat.page_.offset.Size() - 1;
  if (num_row > 0) {
    CHECK_LE(mat.info.num_row_, num_row);
    // provision for empty rows at the bottom of matrix
    auto& offset_vec = mat.page_.offset.HostVector();
    for (uint64_t i = mat.info.num_row_; i < static_cast<uint64_t>(num_row); ++i) {
      offset_vec.push_back(offset_vec.back());
    }
    mat.info.num_row_ = num_row;
    CHECK_EQ(mat.info.num_row_, offset_vec.size() - 1);  // sanity check
  }
  mat.info.num_col_ = ncol;
  mat.info.num_nonzero_ = nelem;
  *out  = new std::shared_ptr<DMatrix>(DMatrix::Create(std::move(source)));
  API_END();
}

XGB_DLL int XGDMatrixCreateFromMat(const bst_float* data,
                                   xgboost::bst_ulong nrow,
                                   xgboost::bst_ulong ncol,
                                   bst_float missing,
                                   DMatrixHandle* out) {
  std::unique_ptr<data::SimpleCSRSource> source(new data::SimpleCSRSource());

  API_BEGIN();
  data::SimpleCSRSource& mat = *source;
  auto& offset_vec = mat.page_.offset.HostVector();
  auto& data_vec = mat.page_.data.HostVector();
  offset_vec.resize(1+nrow);
  bool nan_missing = common::CheckNAN(missing);
  mat.info.num_row_ = nrow;
  mat.info.num_col_ = ncol;
  const bst_float* data0 = data;

  // count elements for sizing data
  data = data0;
  for (xgboost::bst_ulong i = 0; i < nrow; ++i, data += ncol) {
    xgboost::bst_ulong nelem = 0;
    for (xgboost::bst_ulong j = 0; j < ncol; ++j) {
      if (common::CheckNAN(data[j])) {
        CHECK(nan_missing)
          << "There are NAN in the matrix, however, you did not set missing=NAN";
      } else {
        if (nan_missing || data[j] != missing) {
          ++nelem;
        }
      }
    }
    offset_vec[i+1] = offset_vec[i] + nelem;
  }
  data_vec.resize(mat.page_.data.Size() + offset_vec.back());

  data = data0;
  for (xgboost::bst_ulong i = 0; i < nrow; ++i, data += ncol) {
    xgboost::bst_ulong matj = 0;
    for (xgboost::bst_ulong j = 0; j < ncol; ++j) {
      if (common::CheckNAN(data[j])) {
      } else {
        if (nan_missing || data[j] != missing) {
          data_vec[offset_vec[i] + matj] = Entry(j, data[j]);
          ++matj;
        }
      }
    }
  }

  mat.info.num_nonzero_ = mat.page_.data.Size();
  *out  = new std::shared_ptr<DMatrix>(DMatrix::Create(std::move(source)));
  API_END();
}

void PrefixSum(size_t *x, size_t N) {
  size_t *suma;
#pragma omp parallel
  {
    const int ithread = omp_get_thread_num();
    const int nthreads = omp_get_num_threads();
#pragma omp single
    {
      suma = new size_t[nthreads+1];
      suma[0] = 0;
    }
    size_t sum = 0;
    size_t offset = 0;
#pragma omp for schedule(static)
    for (omp_ulong i = 0; i < N; i++) {
      sum += x[i];
      x[i] = sum;
    }
    suma[ithread+1] = sum;
#pragma omp barrier
    for (omp_ulong i = 0; i < static_cast<omp_ulong>(ithread+1); i++) {
      offset += suma[i];
    }
#pragma omp for schedule(static)
    for (omp_ulong i = 0; i < N; i++) {
      x[i] += offset;
    }
  }
  delete[] suma;
}

XGB_DLL int XGDMatrixCreateFromMat_omp(const bst_float* data,  // NOLINT
                                       xgboost::bst_ulong nrow,
                                       xgboost::bst_ulong ncol,
                                       bst_float missing, DMatrixHandle* out,
                                       int nthread) {
  // avoid openmp unless enough data to be worth it to avoid overhead costs
  if (nrow*ncol <= 10000*50) {
    return(XGDMatrixCreateFromMat(data, nrow, ncol, missing, out));
  }

  API_BEGIN();
  const int nthreadmax = std::max(omp_get_num_procs() / 2 - 1, 1);
  //  const int nthreadmax = omp_get_max_threads();
  if (nthread <= 0) nthread=nthreadmax;
  int nthread_orig = omp_get_max_threads();
  omp_set_num_threads(nthread);

  std::unique_ptr<data::SimpleCSRSource> source(new data::SimpleCSRSource());
  data::SimpleCSRSource& mat = *source;
  auto& offset_vec = mat.page_.offset.HostVector();
  auto& data_vec = mat.page_.data.HostVector();
  offset_vec.resize(1+nrow);
  mat.info.num_row_ = nrow;
  mat.info.num_col_ = ncol;

  // Check for errors in missing elements
  // Count elements per row (to avoid otherwise need to copy)
  bool nan_missing = common::CheckNAN(missing);
  std::vector<int> badnan;
  badnan.resize(nthread, 0);

#pragma omp parallel num_threads(nthread)
  {
    int ithread  = omp_get_thread_num();

    // Count elements per row
#pragma omp for schedule(static)
    for (omp_ulong i = 0; i < nrow; ++i) {
      xgboost::bst_ulong nelem = 0;
      for (xgboost::bst_ulong j = 0; j < ncol; ++j) {
        if (common::CheckNAN(data[ncol*i + j]) && !nan_missing) {
          badnan[ithread] = 1;
        } else if (common::CheckNAN(data[ncol * i + j])) {
        } else if (nan_missing || data[ncol * i + j] != missing) {
          ++nelem;
        }
      }
      offset_vec[i+1] = nelem;
    }
  }
  // Inform about any NaNs and resize data matrix
  for (int i = 0; i < nthread; i++) {
    CHECK(!badnan[i]) << "There are NAN in the matrix, however, you did not set missing=NAN";
  }

  // do cumulative sum (to avoid otherwise need to copy)
  PrefixSum(&offset_vec[0], offset_vec.size());
  data_vec.resize(mat.page_.data.Size() + offset_vec.back());

  // Fill data matrix (now that know size, no need for slow push_back())
#pragma omp parallel num_threads(nthread)
  {
#pragma omp for schedule(static)
    for (omp_ulong i = 0; i < nrow; ++i) {
      xgboost::bst_ulong matj = 0;
      for (xgboost::bst_ulong j = 0; j < ncol; ++j) {
        if (common::CheckNAN(data[ncol * i + j])) {
        } else if (nan_missing || data[ncol * i + j] != missing) {
          data_vec[offset_vec[i] + matj] =
              Entry(j, data[ncol * i + j]);
          ++matj;
        }
      }
    }
  }
  // restore omp state
  omp_set_num_threads(nthread_orig);

  mat.info.num_nonzero_ = mat.page_.data.Size();
  *out  = new std::shared_ptr<DMatrix>(DMatrix::Create(std::move(source)));
  API_END();
}

enum class DTType : uint8_t {
  kFloat32 = 0,
  kFloat64 = 1,
  kBool8 = 2,
  kInt32 = 3,
  kInt8 = 4,
  kInt16 = 5,
  kInt64 = 6,
  kUnknown = 7
};

DTType DTGetType(std::string type_string) {
  if (type_string == "float32") {
    return DTType::kFloat32;
  } else if (type_string == "float64") {
    return DTType::kFloat64;
  } else if (type_string == "bool8") {
    return DTType::kBool8;
  } else if (type_string == "int32") {
    return DTType::kInt32;
  } else if (type_string == "int8") {
    return DTType::kInt8;
  } else if (type_string == "int16") {
    return DTType::kInt16;
  } else if (type_string == "int64") {
    return DTType::kInt64;
  } else {
    LOG(FATAL) << "Unknown data table type.";
    return DTType::kUnknown;
  }
}

float DTGetValue(void* column, DTType dt_type, size_t ridx) {
  float missing = std::numeric_limits<float>::quiet_NaN();
  switch (dt_type) {
    case DTType::kFloat32: {
      float val = reinterpret_cast<float*>(column)[ridx];
      return std::isfinite(val) ? val : missing;
    }
    case DTType::kFloat64: {
      double val = reinterpret_cast<double*>(column)[ridx];
      return std::isfinite(val) ? static_cast<float>(val) : missing;
    }
    case DTType::kBool8: {
      bool val = reinterpret_cast<bool*>(column)[ridx];
      return static_cast<float>(val);
    }
    case DTType::kInt32: {
      int32_t val = reinterpret_cast<int32_t*>(column)[ridx];
      return val != (-2147483647 - 1) ? static_cast<float>(val) : missing;
    }
    case DTType::kInt8: {
      int8_t val = reinterpret_cast<int8_t*>(column)[ridx];
      return val != -128 ? static_cast<float>(val) : missing;
    }
    case DTType::kInt16: {
      int16_t val = reinterpret_cast<int16_t*>(column)[ridx];
      return val != -32768 ? static_cast<float>(val) : missing;
    }
    case DTType::kInt64: {
      int64_t val = reinterpret_cast<int64_t*>(column)[ridx];
      return val != -9223372036854775807 - 1 ? static_cast<float>(val)
                                             : missing;
    }
    default: {
      LOG(FATAL) << "Unknown data table type.";
      return 0.0f;
    }
  }
}

XGB_DLL int XGDMatrixCreateFromDT(void** data, const char** feature_stypes,
                                  xgboost::bst_ulong nrow,
                                  xgboost::bst_ulong ncol, DMatrixHandle* out,
                                  int nthread) {
  // avoid openmp unless enough data to be worth it to avoid overhead costs
  if (nrow * ncol <= 10000 * 50) {
    nthread = 1;
  }

  API_BEGIN();
  const int nthreadmax = std::max(omp_get_num_procs() / 2 - 1, 1);
  if (nthread <= 0) nthread = nthreadmax;
  int nthread_orig = omp_get_max_threads();
  omp_set_num_threads(nthread);

  std::unique_ptr<data::SimpleCSRSource> source(new data::SimpleCSRSource());
  data::SimpleCSRSource& mat = *source;
  mat.page_.offset.Resize(1 + nrow);
  mat.info.num_row_ = nrow;
  mat.info.num_col_ = ncol;

  auto& page_offset = mat.page_.offset.HostVector();
#pragma omp parallel num_threads(nthread)
  {
    // Count elements per row, column by column
    for (auto j = 0u; j < ncol; ++j) {
      DTType dtype = DTGetType(feature_stypes[j]);
#pragma omp for schedule(static)
      for (omp_ulong i = 0; i < nrow; ++i) {
        float val = DTGetValue(data[j], dtype, i);
        if (!std::isnan(val)) {
          page_offset[i + 1]++;
        }
      }
    }
  }
  // do cumulative sum (to avoid otherwise need to copy)
  PrefixSum(&page_offset[0], page_offset.size());

  mat.page_.data.Resize(mat.page_.data.Size() + page_offset.back());

  auto& page_data = mat.page_.data.HostVector();

  // Fill data matrix (now that know size, no need for slow push_back())
  std::vector<size_t> position(nrow);
#pragma omp parallel num_threads(nthread)
  {
    for (xgboost::bst_ulong j = 0; j < ncol; ++j) {
      DTType dtype = DTGetType(feature_stypes[j]);
#pragma omp for schedule(static)
      for (omp_ulong i = 0; i < nrow; ++i) {
        float val = DTGetValue(data[j], dtype, i);
        if (!std::isnan(val)) {
          page_data[page_offset[i] + position[i]] = Entry(j, val);
          position[i]++;
        }
      }
    }
  }

  // restore omp state
  omp_set_num_threads(nthread_orig);

  mat.info.num_nonzero_ = mat.page_.data.Size();
  *out = new std::shared_ptr<DMatrix>(DMatrix::Create(std::move(source)));
  API_END();
}

XGB_DLL int XGDMatrixSliceDMatrix(DMatrixHandle handle,
                                  const int* idxset,
                                  xgboost::bst_ulong len,
                                  DMatrixHandle* out) {
  std::unique_ptr<data::SimpleCSRSource> source(new data::SimpleCSRSource());

  API_BEGIN();
  CHECK_HANDLE();
  data::SimpleCSRSource src;
  src.CopyFrom(static_cast<std::shared_ptr<DMatrix>*>(handle)->get());
  data::SimpleCSRSource& ret = *source;

  CHECK_EQ(src.info.group_ptr_.size(), 0U)
      << "slice does not support group structure";

  ret.Clear();
  ret.info.num_row_ = len;
  ret.info.num_col_ = src.info.num_col_;

  auto iter = &src;
  iter->BeforeFirst();
  CHECK(iter->Next());

  const auto& batch = iter->Value();
  const auto& src_labels = src.info.labels_.ConstHostVector();
  const auto& src_weights = src.info.weights_.ConstHostVector();
  const auto& src_base_margin = src.info.base_margin_.ConstHostVector();
  auto& ret_labels = ret.info.labels_.HostVector();
  auto& ret_weights = ret.info.weights_.HostVector();
  auto& ret_base_margin = ret.info.base_margin_.HostVector();
  auto& offset_vec = ret.page_.offset.HostVector();
  auto& data_vec = ret.page_.data.HostVector();

  for (xgboost::bst_ulong i = 0; i < len; ++i) {
    const int ridx = idxset[i];
    auto inst = batch[ridx];
    CHECK_LT(static_cast<xgboost::bst_ulong>(ridx), batch.Size());
    data_vec.insert(data_vec.end(), inst.data(),
                    inst.data() + inst.size());
    offset_vec.push_back(offset_vec.back() + inst.size());
    ret.info.num_nonzero_ += inst.size();

    if (src_labels.size() != 0) {
      ret_labels.push_back(src_labels[ridx]);
    }
    if (src_weights.size() != 0) {
      ret_weights.push_back(src_weights[ridx]);
    }
    if (src_base_margin.size() != 0) {
      ret_base_margin.push_back(src_base_margin[ridx]);
    }
    if (src.info.root_index_.size() != 0) {
      ret.info.root_index_.push_back(src.info.root_index_[ridx]);
    }
  }
  *out = new std::shared_ptr<DMatrix>(DMatrix::Create(std::move(source)));
  API_END();
}

XGB_DLL int XGDMatrixFree(DMatrixHandle handle) {
#ifdef __SGX__
    enclave_XGDMatrixFree(enclave, &enclave_ret, handle);
#else
    API_BEGIN();
    CHECK_HANDLE();
    delete static_cast<std::shared_ptr<DMatrix>*>(handle);
    API_END();
#endif
}

XGB_DLL int XGDMatrixSaveBinary(DMatrixHandle handle,
                                const char* fname,
                                int silent) {
  API_BEGIN();
  CHECK_HANDLE();
  static_cast<std::shared_ptr<DMatrix>*>(handle)->get()->SaveToLocalFile(fname);
  API_END();
}

XGB_DLL int XGDMatrixSetFloatInfo(DMatrixHandle handle,
                          const char* field,
                          const bst_float* info,
                          xgboost::bst_ulong len) {
  API_BEGIN();
  CHECK_HANDLE();
  static_cast<std::shared_ptr<DMatrix>*>(handle)
      ->get()->Info().SetInfo(field, info, kFloat32, len);
  API_END();
}

XGB_DLL int XGDMatrixSetUIntInfo(DMatrixHandle handle,
                         const char* field,
                         const unsigned* info,
                         xgboost::bst_ulong len) {
  API_BEGIN();
  CHECK_HANDLE();
  static_cast<std::shared_ptr<DMatrix>*>(handle)
      ->get()->Info().SetInfo(field, info, kUInt32, len);
  API_END();
}

XGB_DLL int XGDMatrixSetGroup(DMatrixHandle handle,
                              const unsigned* group,
                              xgboost::bst_ulong len) {
  API_BEGIN();
  CHECK_HANDLE();
  auto *pmat = static_cast<std::shared_ptr<DMatrix>*>(handle);
  MetaInfo& info = pmat->get()->Info();
  info.group_ptr_.resize(len + 1);
  info.group_ptr_[0] = 0;
  for (uint64_t i = 0; i < len; ++i) {
    info.group_ptr_[i + 1] = info.group_ptr_[i] + group[i];
  }
  API_END();
}

XGB_DLL int XGDMatrixGetFloatInfo(const DMatrixHandle handle,
                                  const char* field,
                                  xgboost::bst_ulong* out_len,
                                  const bst_float** out_dptr) {
#ifdef __SGX__
    enclave_XGDMatrixGetFloatInfo(enclave, &enclave_ret, handle, field, out_len, (bst_float**) out_dptr);
#else
    API_BEGIN();
    CHECK_HANDLE();
    const MetaInfo& info = static_cast<std::shared_ptr<DMatrix>*>(handle)->get()->Info();
    const std::vector<bst_float>* vec = nullptr;
    if (!std::strcmp(field, "label")) {
      vec = &info.labels_.HostVector();
    } else if (!std::strcmp(field, "weight")) {
      vec = &info.weights_.HostVector();
    } else if (!std::strcmp(field, "base_margin")) {
      vec = &info.base_margin_.HostVector();
    } else {
      LOG(FATAL) << "Unknown float field name " << field;
    }
    *out_len = static_cast<xgboost::bst_ulong>(vec->size());  // NOLINT
    *out_dptr = dmlc::BeginPtr(*vec);
    API_END();
#endif
}

XGB_DLL int XGDMatrixGetUIntInfo(const DMatrixHandle handle,
                                 const char *field,
                                 xgboost::bst_ulong *out_len,
                                 const unsigned **out_dptr) {
#ifdef __SGX__
  enclave_XGDMatrixGetUintInfo(enclave, &enclave_ret, handle, field, out_len, (unsigned**) out_dptr);
#else
  API_BEGIN();
  CHECK_HANDLE();
  const MetaInfo& info = static_cast<std::shared_ptr<DMatrix>*>(handle)->get()->Info();
  const std::vector<unsigned>* vec = nullptr;
  if (!std::strcmp(field, "root_index")) {
    vec = &info.root_index_;
    *out_len = static_cast<xgboost::bst_ulong>(vec->size());
    *out_dptr = dmlc::BeginPtr(*vec);
  } else {
    LOG(FATAL) << "Unknown uint field name " << field;
  }
  API_END();
#endif
}

XGB_DLL int XGDMatrixNumRow(const DMatrixHandle handle,
                            xgboost::bst_ulong *out) {
  API_BEGIN();
  CHECK_HANDLE();
  *out = static_cast<xgboost::bst_ulong>(
      static_cast<std::shared_ptr<DMatrix>*>(handle)->get()->Info().num_row_);
  API_END();
}

XGB_DLL int XGDMatrixNumCol(const DMatrixHandle handle,
                            xgboost::bst_ulong *out) {
  API_BEGIN();
  CHECK_HANDLE();
  *out = static_cast<size_t>(
      static_cast<std::shared_ptr<DMatrix>*>(handle)->get()->Info().num_col_);
  API_END();
}

// xgboost implementation


#if defined(__SGX__) && defined (__HOST__)
XGB_DLL int XGBCreateEnclave(const char *enclave_image, uint32_t flags, int log_verbosity) {
  if (!enclave) {
    oe_result_t result;

    // Create the enclave
    result = oe_create_xgboost_enclave(
        enclave_image, OE_ENCLAVE_TYPE_AUTO, flags, NULL, 0, &enclave);
    if (result != OE_OK) {
      fprintf(
          stderr,
          "oe_create_enclave(): result=%u (%s)\n",
          result,
          oe_result_str(result));
      oe_terminate_enclave(enclave);
      return enclave_ret;
    }
    enclave_init(enclave, log_verbosity);
  }
  return 0;
}
#endif

XGB_DLL int XGBoosterCreate(const DMatrixHandle dmats[],
                    xgboost::bst_ulong len,
                    BoosterHandle *out) {
#ifdef __SGX__
    enclave_XGBoosterCreate(enclave, &enclave_ret, dmats, len, out);
#else
    API_BEGIN();
    std::vector<std::shared_ptr<DMatrix> > mats;
    for (xgboost::bst_ulong i = 0; i < len; ++i) {
      mats.push_back(*static_cast<std::shared_ptr<DMatrix>*>(dmats[i]));
    }
    *out = new Booster(mats);
    API_END();
#endif
}

XGB_DLL int XGBoosterFree(BoosterHandle handle) {
#ifdef __SGX__
    enclave_XGBoosterFree(enclave, &enclave_ret, handle);
#else 
    API_BEGIN();
    CHECK_HANDLE();
    delete static_cast<Booster*>(handle);
    API_END();
#endif 
}

XGB_DLL int XGBoosterSetParam(BoosterHandle handle,
                              const char *name,
                              const char *value) {
#ifdef __SGX__
    enclave_XGBoosterSetParam(enclave, &enclave_ret, handle, name, value);
#else
    API_BEGIN();
    CHECK_HANDLE();
    static_cast<Booster*>(handle)->SetParam(name, value);
    API_END();
#endif 
}

XGB_DLL int XGBoosterUpdateOneIter(BoosterHandle handle,
                                   int iter,
                                   DMatrixHandle dtrain) {
#ifdef __SGX__
    enclave_XGBoosterUpdateOneIter(enclave, &enclave_ret, handle, iter, dtrain);
#else
    API_BEGIN();
    CHECK_HANDLE();
    auto* bst = static_cast<Booster*>(handle);
    auto *dtr =
      static_cast<std::shared_ptr<DMatrix>*>(dtrain);

    bst->LazyInit();
    bst->learner()->UpdateOneIter(iter, dtr->get());
    API_END();
#endif 
}

XGB_DLL int XGBoosterBoostOneIter(BoosterHandle handle,
                                  DMatrixHandle dtrain,
                                  bst_float *grad,
                                  bst_float *hess,
                                  xgboost::bst_ulong len) {
#ifdef __SGX__
    //TODO: test this API
  enclave_XGBoosterBoostOneIter(enclave, &enclave_ret, handle, dtrain, grad, hess, len);
#else
  HostDeviceVector<GradientPair> tmp_gpair;
  API_BEGIN();
  CHECK_HANDLE();
  auto* bst = static_cast<Booster*>(handle);
  auto* dtr =
      static_cast<std::shared_ptr<DMatrix>*>(dtrain);
  tmp_gpair.Resize(len);
  std::vector<GradientPair>& tmp_gpair_h = tmp_gpair.HostVector();
  for (xgboost::bst_ulong i = 0; i < len; ++i) {
    tmp_gpair_h[i] = GradientPair(grad[i], hess[i]);
  }

  bst->LazyInit();
  bst->learner()->BoostOneIter(0, dtr->get(), &tmp_gpair);
  API_END();
#endif
}

XGB_DLL int XGBoosterEvalOneIter(BoosterHandle handle,
                                 int iter,
                                 DMatrixHandle dmats[],
                                 const char* evnames[],
                                 xgboost::bst_ulong len,
                                 const char** out_str) {
#ifdef __SGX__ 
    enclave_XGBoosterEvalOneIter(enclave, &enclave_ret, handle, iter, dmats, evnames, len, (char**) out_str);
#else
    std::string& eval_str = XGBAPIThreadLocalStore::Get()->ret_str;
    API_BEGIN();
    CHECK_HANDLE();
    auto* bst = static_cast<Booster*>(handle);
    std::vector<DMatrix*> data_sets;
    std::vector<std::string> data_names;

    for (xgboost::bst_ulong i = 0; i < len; ++i) {
      data_sets.push_back(static_cast<std::shared_ptr<DMatrix>*>(dmats[i])->get());
      data_names.emplace_back(evnames[i]);
    }

    bst->LazyInit();
    eval_str = bst->learner()->EvalOneIter(iter, data_sets, data_names);
    *out_str = eval_str.c_str();
    API_END();
#endif 
}

XGB_DLL int XGBoosterPredict(BoosterHandle handle,
                             DMatrixHandle dmat,
                             int option_mask,
                             unsigned ntree_limit,
                             xgboost::bst_ulong *len,
                             const bst_float **out_result) {
#ifdef __SGX__
    enclave_XGBoosterPredict(enclave, &enclave_ret, handle, dmat, option_mask, ntree_limit, len, (bst_float**) out_result);
#else 
    std::vector<bst_float>&preds =
      XGBAPIThreadLocalStore::Get()->ret_vec_float;
    API_BEGIN();
    CHECK_HANDLE();
    auto *bst = static_cast<Booster*>(handle);
    bst->LazyInit();
    HostDeviceVector<bst_float> tmp_preds;
    bst->learner()->Predict(
        static_cast<std::shared_ptr<DMatrix>*>(dmat)->get(),
        (option_mask & 1) != 0,
        &tmp_preds, ntree_limit,
        (option_mask & 2) != 0,
        (option_mask & 4) != 0,
        (option_mask & 8) != 0,
        (option_mask & 16) != 0);
    preds = tmp_preds.HostVector();
    *out_result = dmlc::BeginPtr(preds);
    *len = static_cast<xgboost::bst_ulong>(preds.size());
    API_END();
#endif 
}

XGB_DLL int XGBoosterLoadModel(BoosterHandle handle, const char* fname) {
#ifdef __SGX__ 
    enclave_XGBoosterLoadModel(enclave, &enclave_ret, handle, fname);
#else
    API_BEGIN();
    CHECK_HANDLE();
    std::unique_ptr<dmlc::Stream> fi(dmlc::Stream::Create(fname, "r"));
    static_cast<Booster*>(handle)->LoadModel(fi.get());
    API_END();
#endif 
}

XGB_DLL int XGBoosterSaveModel(BoosterHandle handle, const char* fname) {
#ifdef __SGX__
    enclave_XGBoosterSaveModel(enclave, &enclave_ret, handle, fname);
#else
    API_BEGIN();
    CHECK_HANDLE();
    std::unique_ptr<dmlc::Stream> fo(dmlc::Stream::Create(fname, "w"));
    auto *bst = static_cast<Booster*>(handle);
    bst->LazyInit();
    bst->learner()->Save(fo.get());
    API_END();
#endif 
}


XGB_DLL int XGBoosterLoadModelFromBuffer(BoosterHandle handle,
                                 const void* buf,
                                 xgboost::bst_ulong len) {
#ifdef __SGX__
  enclave_XGBoosterLoadModelFromBuffer(enclave, &enclave_ret, handle, buf, len);
#else
  API_BEGIN();
  CHECK_HANDLE();
  common::MemoryFixSizeBuffer fs((void*)buf, len);  // NOLINT(*)
  static_cast<Booster*>(handle)->LoadModel(&fs);
  API_END();
#endif
}

XGB_DLL int XGBoosterGetModelRaw(BoosterHandle handle,
                         xgboost::bst_ulong* out_len,
                         const char** out_dptr) {
#ifdef __SGX__
  enclave_XGBoosterGetModelRaw(enclave, &enclave_ret, handle, out_len, (char**)out_dptr);
#else
  std::string& raw_str = XGBAPIThreadLocalStore::Get()->ret_str;
  raw_str.resize(0);

  API_BEGIN();
  CHECK_HANDLE();
  common::MemoryBufferStream fo(&raw_str);
  auto *bst = static_cast<Booster*>(handle);
  bst->LazyInit();
  bst->learner()->Save(&fo);
  *out_dptr = dmlc::BeginPtr(raw_str);
  *out_len = static_cast<xgboost::bst_ulong>(raw_str.length());
  API_END();
#endif
}

inline void XGBoostDumpModelImpl(
    BoosterHandle handle,
    const FeatureMap& fmap,
    int with_stats,
    const char *format,
    xgboost::bst_ulong* len,
    const char*** out_models) {
  std::vector<std::string>& str_vecs = XGBAPIThreadLocalStore::Get()->ret_vec_str;
  std::vector<const char*>& charp_vecs = XGBAPIThreadLocalStore::Get()->ret_vec_charp;
  auto *bst = static_cast<Booster*>(handle);
  bst->LazyInit();
  str_vecs = bst->learner()->DumpModel(fmap, with_stats != 0, format);
  charp_vecs.resize(str_vecs.size());
  for (size_t i = 0; i < str_vecs.size(); ++i) {
    charp_vecs[i] = str_vecs[i].c_str();
  }
  *out_models = dmlc::BeginPtr(charp_vecs);
  *len = static_cast<xgboost::bst_ulong>(charp_vecs.size());
}
XGB_DLL int XGBoosterDumpModel(BoosterHandle handle,
                       const char* fmap,
                       int with_stats,
                       xgboost::bst_ulong* len,
                       const char*** out_models) {
  return XGBoosterDumpModelEx(handle, fmap, with_stats, "text", len, out_models);
}
XGB_DLL int XGBoosterDumpModelEx(BoosterHandle handle,
                       const char* fmap,
                       int with_stats,
                       const char *format,
                       xgboost::bst_ulong* len,
                       const char*** out_models) {
  API_BEGIN();
  CHECK_HANDLE();
  FeatureMap featmap;
  if (strlen(fmap) != 0) {
    std::unique_ptr<dmlc::Stream> fs(
        dmlc::Stream::Create(fmap, "r"));
    dmlc::istream is(fs.get());
    featmap.LoadText(is);
  }
  XGBoostDumpModelImpl(handle, featmap, with_stats, format, len, out_models);
  API_END();
}

XGB_DLL int XGBoosterDumpModelWithFeatures(BoosterHandle handle,
                                   int fnum,
                                   const char** fname,
                                   const char** ftype,
                                   int with_stats,
                                   xgboost::bst_ulong* len,
                                   const char*** out_models) {
  return XGBoosterDumpModelExWithFeatures(handle, fnum, fname, ftype, with_stats,
                                   "text", len, out_models);
}
XGB_DLL int XGBoosterDumpModelExWithFeatures(BoosterHandle handle,
                                   int fnum,
                                   const char** fname,
                                   const char** ftype,
                                   int with_stats,
                                   const char *format,
                                   xgboost::bst_ulong* len,
                                   const char*** out_models) {
  API_BEGIN();
  CHECK_HANDLE();
  FeatureMap featmap;
  for (int i = 0; i < fnum; ++i) {
    featmap.PushBack(i, fname[i], ftype[i]);
  }
  XGBoostDumpModelImpl(handle, featmap, with_stats, format, len, out_models);
  API_END();
}

XGB_DLL int XGBoosterGetAttr(BoosterHandle handle,
                     const char* key,
                     const char** out,
                     int* success) {
  auto* bst = static_cast<Booster*>(handle);
  std::string& ret_str = XGBAPIThreadLocalStore::Get()->ret_str;
  API_BEGIN();
  CHECK_HANDLE();
  if (bst->learner()->GetAttr(key, &ret_str)) {
    *out = ret_str.c_str();
    *success = 1;
  } else {
    *out = nullptr;
    *success = 0;
  }
  API_END();
}

XGB_DLL int XGBoosterSetAttr(BoosterHandle handle,
                     const char* key,
                     const char* value) {
  auto* bst = static_cast<Booster*>(handle);
  API_BEGIN();
  CHECK_HANDLE();
  if (value == nullptr) {
    bst->learner()->DelAttr(key);
  } else {
    bst->learner()->SetAttr(key, value);
  }
  API_END();
}

XGB_DLL int XGBoosterGetAttrNames(BoosterHandle handle,
                     xgboost::bst_ulong* out_len,
                     const char*** out) {
  std::vector<std::string>& str_vecs = XGBAPIThreadLocalStore::Get()->ret_vec_str;
  std::vector<const char*>& charp_vecs = XGBAPIThreadLocalStore::Get()->ret_vec_charp;
  auto *bst = static_cast<Booster*>(handle);
  API_BEGIN();
  CHECK_HANDLE();
  str_vecs = bst->learner()->GetAttrNames();
  charp_vecs.resize(str_vecs.size());
  for (size_t i = 0; i < str_vecs.size(); ++i) {
    charp_vecs[i] = str_vecs[i].c_str();
  }
  *out = dmlc::BeginPtr(charp_vecs);
  *out_len = static_cast<xgboost::bst_ulong>(charp_vecs.size());
  API_END();
}

XGB_DLL int XGBoosterLoadRabitCheckpoint(BoosterHandle handle,
                                 int* version) {
  API_BEGIN();
  CHECK_HANDLE();
  auto* bst = static_cast<Booster*>(handle);
  *version = rabit::LoadCheckPoint(bst->learner());
  if (*version != 0) {
    bst->Intialize();
  }
  API_END();
}

XGB_DLL int XGBoosterSaveRabitCheckpoint(BoosterHandle handle) {
  API_BEGIN();
  CHECK_HANDLE();
  auto* bst = static_cast<Booster*>(handle);
  if (bst->learner()->AllowLazyCheckPoint()) {
    rabit::LazyCheckPoint(bst->learner());
  } else {
    rabit::CheckPoint(bst->learner());
  }
  API_END();
}

/* hidden method; only known to C++ test suite */
const std::map<std::string, std::string>&
QueryBoosterConfigurationArguments(BoosterHandle handle) {
  CHECK_HANDLE();
  auto* bst = static_cast<Booster*>(handle);
  bst->LazyInit();
  return bst->learner()->GetConfigurationArguments();
}

// force link rabit
static DMLC_ATTRIBUTE_UNUSED int XGBOOST_LINK_RABIT_C_API_ = RabitLinkTag();


#if defined(__SGX__) && defined(__HOST__)
int ocall_rabit__GetRank() {
    return rabit::GetRank();
}

int ocall_rabit__GetWorldSize() {
    return rabit::GetWorldSize();
}

int ocall_rabit__IsDistributed() {
    return rabit::IsDistributed();
}
#endif // __SGX__

#ifdef __SGX__
XGB_DLL int get_remote_report_with_pubkey(
    uint8_t** pem_key,
    size_t* key_size,
    uint8_t** remote_report,
    size_t* remote_report_size) {
  enclave_get_remote_report_with_pubkey(enclave, &enclave_ret, pem_key, key_size, remote_report, remote_report_size);
  return enclave_ret;
}
/**
 * Attest the given remote report and accompanying data. It consists of the
 * following three steps:
 *
 * 1) The remote report is first attested using the oe_verify_report API. This
 * ensures the authenticity of the enclave that generated the remote report.
 * 2) Next, to establish trust of the enclave that  generated the remote report,
 * the mrsigner, product_id, isvsvn values are checked to  see if they are
 * predefined trusted values.
 * 3) Once the enclave's trust has been established, the validity of
 * accompanying data is ensured by comparing its SHA256 digest against the
 * report_data field.
 */
bool attest_remote_report(
    const uint8_t* remote_report,
    size_t remote_report_size,
    const uint8_t* data,
    size_t data_size) {

  bool ret = false;
  uint8_t sha256[32];
  oe_report_t parsed_report = {0};
  oe_result_t result = OE_OK;

  // 1)  Validate the report's trustworthiness
  // Verify the remote report to ensure its authenticity.
  result = oe_verify_report(NULL, remote_report, remote_report_size, &parsed_report);
  if (result != OE_OK) {
    std::cout << "oe_verify_report failed " << oe_result_str(result);
    exit(1);
  }

  // 2) validate the enclave identity's signed_id is the hash of the public
  // signing key that was used to sign an enclave. Check that the enclave was
  // signed by an trusted entity.
  // FIXME Enable this check
  /*if (memcmp(parsed_report.identity.signer_id, m_enclave_mrsigner, 32) != 0) {
    LOG(FATAL) << "identity.signer_id checking failed."

    std::cout<< "identity.signer_id " << parsed_report.identity.signer_id;

    for (int i = 0; i < 32; i++) {
    TRACE_ENCLAVE(
    "m_enclave_mrsigner[%d]=0x%0x\n",
    i,
    (uint8_t)m_enclave_mrsigner[i]);
    }

    TRACE_ENCLAVE("\n\n\n");

    for (int i = 0; i < 32; i++)
    {
    TRACE_ENCLAVE(
    "parsedReport.identity.signer_id)[%d]=0x%0x\n",
    i,
    (uint8_t)parsed_report.identity.signer_id[i]);
    }
    TRACE_ENCLAVE("m_enclave_mrsigner %s", m_enclave_mrsigner);
    goto exit;
    }*/

  // FIXME add verification for MRENCLAVE

  // Check the enclave's product id and security version
  // See enc.conf for values specified when signing the enclave.
  if (parsed_report.identity.product_id[0] != 1) {
    std::cout << "identity.product_id checking failed." << std::endl;
  }

  if (parsed_report.identity.security_version < 1) {
    std::cout << "identity.security_version checking failed." << std::endl;
  }

  // 3) Validate the report data
  //    The report_data has the hash value of the report data
  if (compute_sha256(data, data_size, sha256) != 0) {
    std::cout << "hash validation failed." << std::endl;
  }

  if (memcmp(parsed_report.report_data, sha256, sizeof(sha256)) != 0) {
    std::cout << "SHA256 mismatch." << std::endl;
  }
  ret = true;
  std::cout << "remote attestation succeeded." << std::endl;
  return ret;
}

// FIXME eventually we only need an enclave implementation of this API, callable from the host
// currently implemented for local testing
XGB_DLL int verify_remote_report_and_set_pubkey(
    uint8_t* pem_key,
    size_t key_size,
    uint8_t* remote_report,
    size_t remote_report_size) {

  // Attest the remote report and accompanying key.
  if (!attest_remote_report(remote_report, remote_report_size, pem_key, key_size)) {
    std::cout << "verify_report_and_set_pubkey failed." << std::endl;
    return -1;
  }
  std::cout << "verify_report_and_set_pubkey succeeded." << std::endl;
  return 0;
}

XGB_DLL int add_client_key(char* fname, uint8_t* data, size_t data_len, uint8_t* signature, size_t sig_len) {
    // FIXME return value / error handling
  enclave_add_client_key(enclave, &enclave_ret, fname, data, data_len, signature, sig_len);
  return enclave_ret;
}

XGB_DLL int encrypt_data_with_pk(char* data, size_t len, uint8_t* pem_key, size_t key_size, uint8_t* encrypted_data, size_t* encrypted_data_size) {
  bool result = false;
  mbedtls_pk_context key;
  int res = -1;

  mbedtls_ctr_drbg_context m_ctr_drbg_context;
  mbedtls_entropy_context m_entropy_context;
  mbedtls_pk_context m_pk_context;
  mbedtls_ctr_drbg_init(&m_ctr_drbg_context);
  mbedtls_entropy_init(&m_entropy_context);
  mbedtls_pk_init(&m_pk_context);
  res = mbedtls_ctr_drbg_seed(
      &m_ctr_drbg_context, mbedtls_entropy_func, &m_entropy_context, NULL, 0);
  res = mbedtls_pk_setup(
      &m_pk_context, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));

  mbedtls_rsa_context* rsa_context;

  mbedtls_pk_init(&key);

  // Read the given public key.
  key_size = strlen((const char*)pem_key) + 1; // Include ending '\0'.
  res = mbedtls_pk_parse_public_key(&key, pem_key, key_size);

  if (res != 0) {
    std::cout << "mbedtls_pk_parse_public_key failed.\n";
    mbedtls_pk_free(&key);
    return res;
  }

  rsa_context = mbedtls_pk_rsa(key);
  rsa_context->padding = MBEDTLS_RSA_PKCS_V21;
  rsa_context->hash_id = MBEDTLS_MD_SHA256;

  // Encrypt the data.
  res = mbedtls_rsa_pkcs1_encrypt(
      rsa_context,
      //mbedtls_pk_rsa(key),
      mbedtls_ctr_drbg_random,
      &m_ctr_drbg_context,
      MBEDTLS_RSA_PUBLIC,
      len,
      (const unsigned char*) data,
      (unsigned char*) encrypted_data);
  if (res != 0) {
    std::cout << "mbedtls_rsa_pkcs1_encrypt failed\n";
    mbedtls_pk_free(&key);
    return res;
  }

  *encrypted_data_size = mbedtls_pk_rsa(key)->len;

  mbedtls_pk_free( &m_pk_context );
  mbedtls_ctr_drbg_free( &m_ctr_drbg_context );
  mbedtls_entropy_free( &m_entropy_context );
  return 0;
}

XGB_DLL int sign_data(char *keyfile, uint8_t* data, size_t data_size, uint8_t* signature, size_t* sig_len) {
  mbedtls_pk_context pk;
  mbedtls_entropy_context m_entropy_context;
  mbedtls_ctr_drbg_context m_ctr_drbg_context;

  mbedtls_entropy_init( &m_entropy_context );
  mbedtls_pk_init( &pk );
  mbedtls_ctr_drbg_init( &m_ctr_drbg_context ); 

  unsigned char hash[32];
  int ret = 1;

  ret = mbedtls_ctr_drbg_seed(&m_ctr_drbg_context, mbedtls_entropy_func, &m_entropy_context, NULL, 0);

  if((ret = mbedtls_pk_parse_keyfile( &pk, keyfile, "")) != 0) {
    printf( " failed\n  ! Could not read key from '%s'\n", keyfile);
    printf( "  ! mbedtls_pk_parse_public_keyfile returned %d\n\n", ret );
    return ret;
  }
  if(!mbedtls_pk_can_do(&pk, MBEDTLS_PK_RSA)) {
    printf( " failed\n  ! Key is not an RSA key\n" );
    return ret;
  }

  mbedtls_rsa_set_padding(mbedtls_pk_rsa(pk), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256 );

  if((ret = compute_sha256(data, data_size, hash)) != 0) {
    printf( " failed\n  ! Could not hash\n\n");
    return ret;
  }

  if((ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 0, signature, sig_len, mbedtls_ctr_drbg_random, &m_ctr_drbg_context)) != 0) {
    printf( " failed\n  ! mbedtls_pk_sign returned %d\n\n", ret );
    return ret;
  }
  return 0;
}
#endif

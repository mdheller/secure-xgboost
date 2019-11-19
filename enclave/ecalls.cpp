#include <xgboost/c_api.h>
#include <xgboost/base.h>
#include <xgboost/logging.h>

#include "xgboost_t.h"
#include <stdio.h>

int enclave_XGDMatrixCreateFromFile(const char *fname, int silent, DMatrixHandle *out) {
  LOG(DEBUG) << "Ecall: XGDMatrixCreateFromFile";
  return XGDMatrixCreateFromFile(fname, silent, out);
}

int enclave_XGBoosterCreate(const DMatrixHandle dmats[], bst_ulong len, BoosterHandle* out) {
  LOG(DEBUG) << "Ecall: XGBoosterCreate";
  return XGBoosterCreate(dmats, len, out);
}

int enclave_XGBoosterSetParam(BoosterHandle handle, const char *name, const char *value) {
  LOG(DEBUG) << "Ecall: XGBoosterSetParam";
  return XGBoosterSetParam(handle, name, value);
}

int enclave_XGBoosterUpdateOneIter(BoosterHandle handle, int iter, DMatrixHandle dtrain) {
  LOG(DEBUG) << "Ecall: XGBoosterUpdateOneIter";
  return XGBoosterUpdateOneIter(handle, iter, dtrain);
}

int enclave_XGBoosterEvalOneIter(BoosterHandle handle, int iter, DMatrixHandle dmats[], const char* evnames[], bst_ulong len, const char** out_str) {
  LOG(DEBUG) << "Ecall: XGBoosterEvalOneIter";
  return XGBoosterEvalOneIter(handle, iter, dmats, evnames, len, out_str);
}

int enclave_XGBoosterLoadModel(BoosterHandle handle, const char *fname) {
  LOG(DEBUG) << "Ecall: XGBoosterLoadModel";
  return XGBoosterLoadModel(handle, fname);
}

int enclave_XGBoosterSaveModel(BoosterHandle handle, const char *fname) {
  LOG(DEBUG) << "Ecall: XGBoosterSaveModel";
  return XGBoosterSaveModel(handle, fname);
}

int enclave_XGBoosterPredict(BoosterHandle handle, DMatrixHandle dmat, int option_mask, unsigned ntree_limit, bst_ulong *len, const bst_float **out_result) {
  LOG(DEBUG) << "Ecall: XGBoosterPredict";
  return XGBoosterPredict(handle, dmat, option_mask, ntree_limit, len, out_result);
}

int enclave_XGDMatrixGetFloatInfo(const DMatrixHandle handle, const char* field, bst_ulong *out_len, const bst_float **out_dptr) {
  LOG(DEBUG) << "Ecall: XGDMatrixGetFloatInfo";
  return XGDMatrixGetFloatInfo(handle, field, out_len, out_dptr);
}

int enclave_XGDMatrixFree(DMatrixHandle handle) {
  LOG(DEBUG) << "Ecall: XGDMatrixFree";
  return XGDMatrixFree(handle);
}

int enclave_XGBoosterFree(BoosterHandle handle) {
  LOG(DEBUG) << "Ecall: XGBoosterFree";
  return XGBoosterFree(handle);
}

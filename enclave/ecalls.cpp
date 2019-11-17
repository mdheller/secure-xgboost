#include <xgboost/c_api.h>
#include <xgboost/base.h>

#include "xgboost_t.h"
#include <stdio.h>

void enclave_helloworld()
{
  // Print a message from the enclave. Note that this
  // does not directly call fprintf, but calls into the
  // host and calls fprintf from there. This is because
  // the fprintf function is not part of the enclave
  // as it requires support from the kernel.
  fprintf(stdout, "Hello world from the enclave\n");

  // Call back into the host
  oe_result_t result = host_helloworld();
  if (result != OE_OK)
  {
    fprintf(
        stderr,
        "Call to host_helloworld failed: result=%u (%s)\n",
        result,
        oe_result_str(result));
  }
}

int enclave_XGDMatrixCreateFromFile(const char *fname, int silent, DMatrixHandle *out) {
  return XGDMatrixCreateFromFile(fname, silent, out);
}

int enclave_XGBoosterCreate(const DMatrixHandle dmats[], bst_ulong len, BoosterHandle* out) {
  fprintf (stdout, "Ecall XGBoosterCreate\n");
  return XGBoosterCreate(dmats, len, out);
}

int enclave_XGBoosterSetParam(BoosterHandle handle, const char *name, const char *value) {
  fprintf (stdout, "Ecall XGBoosterSetParam\n");
  return XGBoosterSetParam(handle, name, value);
}

int enclave_XGBoosterUpdateOneIter(BoosterHandle handle, int iter, DMatrixHandle dtrain) {
  fprintf (stdout, "Ecall XGBoosterUpdateOneIter\n");
  return XGBoosterUpdateOneIter(handle, iter, dtrain);
}

int enclave_XGBoosterEvalOneIter(BoosterHandle handle, int iter, DMatrixHandle dmats[], const char* evnames[], bst_ulong len, const char** out_str) {
  fprintf (stdout, "Ecall XGBoosterEvalOneIter\n");
  return XGBoosterEvalOneIter(handle, iter, dmats, evnames, len, out_str);
}

int enclave_XGBoosterLoadModel(BoosterHandle handle, const char *fname) {
  fprintf (stdout, "Ecall XGBoosterLoadModel\n");
  return XGBoosterLoadModel(handle, fname);
}

int enclave_XGBoosterSaveModel(BoosterHandle handle, const char *fname) {
  fprintf (stdout, "Ecall XGBoosterSaveModel\n");
  return XGBoosterSaveModel(handle, fname);
}

int enclave_XGBoosterPredict(BoosterHandle handle, DMatrixHandle dmat, int option_mask, unsigned ntree_limit, bst_ulong *len, const bst_float **out_result) {
  fprintf (stdout, "Ecall enclave_XGBoosterPredict\n");
  return XGBoosterPredict(handle, dmat, option_mask, ntree_limit, len, out_result);
}

int enclave_XGDMatrixGetFloatInfo(const DMatrixHandle handle, const char* field, bst_ulong *out_len, const bst_float **out_dptr) {
  fprintf(stdout, "Ecall enclave_XGDMatrixGetFloatInfo\n");
  return XGDMatrixGetFloatInfo(handle, field, out_len, out_dptr);
}

int enclave_XGDMatrixFree(DMatrixHandle handle) {
  fprintf (stdout, "Ecall enclave_XGDMatrixFree\n");
  return XGDMatrixFree(handle);
}

int enclave_XGBoosterFree(BoosterHandle handle) {
  fprintf (stdout, "Ecall enclave_XGBoosterFree\n");
  return XGBoosterFree(handle);
}

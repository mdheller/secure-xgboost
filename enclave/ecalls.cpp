#include <xgboost/c_api.h>

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

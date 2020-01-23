#include <xgboost/c_api.h>
#include <xgboost/base.h>
#include <xgboost/logging.h>
#include <sys/mount.h>

#include "xgboost_t.h"
#include "src/common/common.h"

void enclave_init() {
  LOG(DEBUG) << "Ecall: init\n";
  oe_result_t result;
  if ((result = oe_load_module_host_resolver()) != OE_OK) {
      fprintf(stdout, "oe_load_module_host_resolver failed with %s\n", oe_result_str(result));
  }
  if ((result = oe_load_module_host_socket_interface()) != OE_OK) {
      fprintf(stdout, "oe_load_module_host_socket_interface failed with %s\n", oe_result_str(result));
  }
  if ((result = oe_load_module_host_file_system()) != OE_OK) {
      fprintf(stdout, "oe_load_module_host_file_system failed with %s\n", oe_result_str(result));
  }
  /* Mount the host file system on the root directory. */
  if (mount("/", "/", OE_HOST_FILE_SYSTEM, 0, NULL) != 0) {
      fprintf(stdout, "Unable to mount host file system on the root directory\n");
  }
  fprintf(stdout, "Loaded all modules\n");
}

int enclave_XGDMatrixCreateFromFile(const char *fname, int silent, DMatrixHandle *out) {
  LOG(DEBUG) << "Ecall: XGDMatrixCreateFromFile";
  return XGDMatrixCreateFromFile(fname, silent, out);
}

int enclave_XGDMatrixCreateFromEncryptedFile(const char *fname, int silent, DMatrixHandle *out) {
  LOG(DEBUG) << "Ecall: XGDMatrixCreateFromEncryptedFile";
  return XGDMatrixCreateFromEncryptedFile(fname, silent, out);
}

int enclave_XGBoosterCreate(const DMatrixHandle dmats[], bst_ulong len, BoosterHandle* out) {
  LOG(DEBUG) << "Ecall: XGBoosterCreate";
  return XGBoosterCreate(dmats, len, out);
}

int enclave_XGBoosterSetParam(BoosterHandle handle, const char *name, const char *value) {
  LOG(DEBUG) << "Ecall: XGBoosterSetParam";
  check_enclave_ptr(handle);
  return XGBoosterSetParam(handle, name, value);
}

int enclave_XGBoosterUpdateOneIter(BoosterHandle handle, int iter, DMatrixHandle dtrain) {
  LOG(DEBUG) << "Ecall: XGBoosterUpdateOneIter";
  check_enclave_ptr(handle);
  check_enclave_ptr(dtrain);
  return XGBoosterUpdateOneIter(handle, iter, dtrain);
}

int enclave_XGBoosterBoostOneIter(BoosterHandle handle, DMatrixHandle dtrain, bst_float *grad, bst_float *hess, xgboost::bst_ulong len) {
  LOG(DEBUG) << "Ecall: XGBoosterBoostOneIter";
  check_enclave_ptr(handle);
  check_enclave_ptr(dtrain);
  return XGBoosterBoostOneIter(handle, dtrain, grad, hess, len);
}

int enclave_XGBoosterEvalOneIter(BoosterHandle handle, int iter, DMatrixHandle dmats[], const char* evnames[], bst_ulong len, const char** out_str) {
  LOG(DEBUG) << "Ecall: XGBoosterEvalOneIter";
  check_enclave_ptr(handle);
  check_enclave_ptr(dmats[0]);
  check_enclave_ptr(dmats[1]);
  return XGBoosterEvalOneIter(handle, iter, dmats, evnames, len, out_str);
}

int enclave_XGBoosterLoadModel(BoosterHandle handle, const char *fname) {
  LOG(DEBUG) << "Ecall: XGBoosterLoadModel";
  check_enclave_ptr(handle);
  return XGBoosterLoadModel(handle, fname);
}

int enclave_XGBoosterSaveModel(BoosterHandle handle, const char *fname) {
  LOG(DEBUG) << "Ecall: XGBoosterSaveModel";
  check_enclave_ptr(handle);
  return XGBoosterSaveModel(handle, fname);
}

int enclave_XGBoosterPredict(BoosterHandle handle, DMatrixHandle dmat, int option_mask, unsigned ntree_limit, bst_ulong *len, const bst_float **out_result) {
  LOG(DEBUG) << "Ecall: XGBoosterPredict";
  check_enclave_ptr(handle);
  check_enclave_ptr(dmat);
  return XGBoosterPredict(handle, dmat, option_mask, ntree_limit, len, out_result);
}

int enclave_XGDMatrixGetFloatInfo(const DMatrixHandle handle, const char* field, bst_ulong *out_len, const bst_float **out_dptr) {
  LOG(DEBUG) << "Ecall: XGDMatrixGetFloatInfo";
  check_enclave_ptr(handle);
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

int enclave_get_remote_report_with_pubkey(
        uint8_t** pem_key,
        size_t* key_size,
        uint8_t** remote_report,
        size_t* remote_report_size) {
  fprintf(stdout, "Ecall: enclave_get_remote_report_with_pubkey\n");
  return get_remote_report_with_pubkey(pem_key, key_size, remote_report, remote_report_size);
}

int enclave_verify_remote_report_and_set_pubkey(
        uint8_t* pem_key,
        size_t key_size,
        uint8_t* remote_report,
        size_t remote_report_size) {
  fprintf(stdout, "Ecall: verify_remote_report_and_set_pubkey\n");
  return verify_remote_report_and_set_pubkey(pem_key, key_size, remote_report, remote_report_size);
}

int enclave_add_client_key(
    char* fname,
    uint8_t* data,
    size_t data_len,
    uint8_t* signature,
    size_t sig_len) {
  fprintf(stdout, "Ecall: add_client_key\n");
  return add_client_key(fname, data, data_len, signature, sig_len);
}

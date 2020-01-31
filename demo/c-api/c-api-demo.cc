/*!
 * Copyright 2019 XGBoost contributors
 *
 * \file c-api-demo.c
 * \brief A simple example of using xgboost C API.
 */

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <xgboost/c_api.h>

#ifdef __SGX__
#include "encrypt.h"
#include <openenclave/host.h>

bool check_simulate_opt(int* argc, char* argv[]) {
  for (int i = 0; i < *argc; i++) {
    if (strcmp(argv[i], "--simulate") == 0) {
      std::cout << "Running in simulation mode" << std::endl;
      memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char*));
      (*argc)--;
      return true;
    }
  }
  return false;
}
#endif

#define safe_xgboost(call) {                                            \
int err = (call);                                                       \
if (err != 0) {                                                         \
  fprintf(stderr, "%s:%d: error in %s: %s\n", __FILE__, __LINE__, #call, XGBGetLastError()); \
  exit(1);                                                              \
}                                                                       \
}

int main(int argc, char** argv) {

#ifdef __SGX__
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--encrypt") == 0) {
      encryptFile("../data/agaricus.txt.train", "train.encrypted");
      encryptFile("../data/agaricus.txt.test", "test.encrypted");
      return 0;
    }
  }

  uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
  if (check_simulate_opt(&argc, argv)) {
    flags |= OE_ENCLAVE_FLAG_SIMULATE;
  }

  int log_verbosity = 3;
  XGBCreateEnclave(argv[1], flags, log_verbosity);
  oe_result_t result;
  int ret = 1;
  oe_enclave_t* enclave = NULL;

  uint8_t* pem_key = NULL;
  size_t key_size = 0;
  uint8_t* remote_report = NULL;
  size_t remote_report_size = 0;

  //safe_xgboost(get_remote_report_with_pubkey(&pem_key, &key_size, &remote_report, &remote_report_size));
  //safe_xgboost(verify_remote_report_and_set_pubkey(pem_key, key_size, remote_report, remote_report_size));

  uint8_t* encrypted_data = (uint8_t*) malloc(1024*sizeof(uint8_t));
  size_t encrypted_data_size = 1024;
  uint8_t* signature = (uint8_t*) malloc(1024*sizeof(uint8_t));
  size_t sig_len;

  //safe_xgboost(encrypt_data_with_pk(test_key, KEY_BYTES, pem_key, key_size, encrypted_data, &encrypted_data_size));
  //safe_xgboost(sign_data("keypair.pem", encrypted_data, encrypted_data_size, signature, &sig_len));
  //verifySignature("publickey.crt", encrypted_data, encrypted_data_size, signature, sig_len);

  //std::string fname1("train.encrypted");
  //safe_xgboost(add_client_key((char*)fname1.c_str(), encrypted_data, encrypted_data_size, signature, sig_len));
  //std::string fname2("test.encrypted");
  //safe_xgboost(add_client_key((char*)fname2.c_str(), encrypted_data, encrypted_data_size, signature, sig_len));
#endif

  int silent = 0;
  int use_gpu = 0; // set to 1 to use the GPU for training
  
  // load the data
  DMatrixHandle dtrain, dtest;
#ifdef __SGX__
  safe_xgboost(XGDMatrixCreateFromEncryptedFile("/root/mc2/code/secure-xgboost/demo/c-api/train.encrypted", silent, &dtrain));
  safe_xgboost(XGDMatrixCreateFromEncryptedFile("/root/mc2/code/secure-xgboost/demo/c-api/test.encrypted", silent, &dtest));
  //safe_xgboost(XGDMatrixCreateFromFile("train.encrypted", silent, &dtrain));
  //safe_xgboost(XGDMatrixCreateFromFile("test.encrypted", silent, &dtest));
  //safe_xgboost(XGDMatrixCreateFromFile("/root/mc2/code/secure-xgboost/demo/data/agaricus.txt.train", silent, &dtrain));
  //safe_xgboost(XGDMatrixCreateFromFile("/root/mc2/code/secure-xgboost/demo/data/agaricus.txt.test", silent, &dtest));
#else
  safe_xgboost(XGDMatrixCreateFromFile("../data/agaricus.txt.train", silent, &dtrain));
  safe_xgboost(XGDMatrixCreateFromFile("../data/agaricus.txt.test", silent, &dtest));
#endif
  std::cout << "Data loaded" << std::endl;
  
  // create the booster
  BoosterHandle booster;
  DMatrixHandle eval_dmats[2] = {dtrain, dtest};
  safe_xgboost(XGBoosterCreate(eval_dmats, 2, &booster));
  std::cout << "Booster created" << std::endl;

  // configure the training
  // available parameters are described here:
  // https://xgboost.readthedocs.io/en/latest/parameter.html
  safe_xgboost(XGBoosterSetParam(booster, "tree_method", use_gpu ? "gpu_hist" : "hist"));
  std::cout << "First parameter set" << std::endl;
  if (use_gpu) {
    // set the number of GPUs and the first GPU to use;
    // this is not necessary, but provided here as an illustration
    safe_xgboost(XGBoosterSetParam(booster, "n_gpus", "1"));
    safe_xgboost(XGBoosterSetParam(booster, "gpu_id", "0"));
  } else {
    safe_xgboost(XGBoosterSetParam(booster, "n_gpus", "0"));
  }

  safe_xgboost(XGBoosterSetParam(booster, "objective", "binary:logistic"));
  safe_xgboost(XGBoosterSetParam(booster, "min_child_weight", "1"));
  safe_xgboost(XGBoosterSetParam(booster, "gamma", "0.1"));
  safe_xgboost(XGBoosterSetParam(booster, "max_depth", "3"));
  safe_xgboost(XGBoosterSetParam(booster, "verbosity", silent ? "0" : "3"));
  std::cout << "All parameters set" << std::endl;
  
  // train and evaluate for 10 iterations
  int n_trees = 10;
  const char* eval_names[2] = {"train", "test"};
  const char* eval_result = NULL;
  for (int i = 0; i < n_trees; ++i) {
    safe_xgboost(XGBoosterUpdateOneIter(booster, i, dtrain));
    safe_xgboost(XGBoosterEvalOneIter(booster, i, eval_dmats, eval_names, 2, &eval_result));
    printf("%s\n", eval_result);
  }

  // save model
  const char* fname = "/root/mc2/code/secure-xgboost/demo/c-api/demo_model.model";
  safe_xgboost(XGBoosterSaveModel(booster, fname));
  std::cout << "Saved model to demo_model.model" << std::endl;
  // load model
  booster = NULL;
  safe_xgboost(XGBoosterCreate(eval_dmats, 2, &booster));
  safe_xgboost(XGBoosterLoadModel(booster, fname));
  std::cout << "Loaded model from demo_model.model" << std::endl;


  //// save model
  //bst_ulong len;
  //const char* buf;
  //safe_xgboost(XGBoosterGetModelRaw(booster, &len, &buf));
  //std::cout << "Saved model to buffer" << std::endl;
  //// load model
  //booster = NULL;
  //safe_xgboost(XGBoosterCreate(eval_dmats, 2, &booster));
  ////FIXME doesn't work with `safe_xgboost`
  //XGBoosterLoadModelFromBuffer(booster, buf, len);
  ////safe_xgboost(XGBoosterLoadModel(booster, fname));
  //std::cout << "Loaded model from buffer" << std::endl;

  // predict
  bst_ulong out_len = 0;
  const float* out_result = NULL;
  int n_print = 10;
  
  safe_xgboost(XGBoosterPredict(booster, dtrain, 0, 0, &out_len, &out_result));
  printf("DONE PREDICTING\n");
  printf("y_pred: ");
  for (int i = 0; i < n_print; ++i) {
    printf("%1.4f ", out_result[i]);
  }
  printf("\n");
  
  safe_xgboost(XGDMatrixGetFloatInfo(dtrain, "label", &out_len, &out_result));
  printf("y_test: ");
  for (int i = 0; i < n_print; ++i) {
    printf("%1.4f ", out_result[i]);
  }
  printf("\n");
  
  safe_xgboost(XGBoosterFree(booster));
  safe_xgboost(XGDMatrixFree(dtrain));
  safe_xgboost(XGDMatrixFree(dtest));
  return 0;
}

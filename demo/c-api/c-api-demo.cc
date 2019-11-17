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
#include <openenclave/host.h>
#include "xgboost_u.h"
#endif

#define safe_xgboost(call) {                                            \
int err = (call);                                                       \
if (err != 0) {                                                         \
  fprintf(stderr, "%s:%d: error in %s: %s\n", __FILE__, __LINE__, #call, XGBGetLastError()); \
  exit(1);                                                              \
}                                                                       \
}

#ifdef __SGX__
bool check_simulate_opt(int* argc, char* argv[])
{
  for (int i = 0; i < *argc; i++)
  {
    if (strcmp(argv[i], "--simulate") == 0)
    {
      std::cout << "Running in simulation mode" << std::endl;
      memmove(&argv[i], &argv[i + 1], (*argc - i) * sizeof(char*));
      (*argc)--;
      return true;
    }
  }
  return false;
}
#endif

int main(int argc, char** argv) {

#ifdef __SGX__
  oe_result_t result;
  int ret = 1;
  oe_enclave_t* enclave = NULL;

  uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
  if (check_simulate_opt(&argc, argv)) {
    flags |= OE_ENCLAVE_FLAG_SIMULATE;
  }

  flags |= OE_ENCLAVE_FLAG_DEBUG;

  if (argc != 2) {
    fprintf(
        stderr, "Usage: %s enclave_image_path [ --simulate  ]\n", argv[0]);
    oe_terminate_enclave(enclave);
    return ret;
  }

  // Create the enclave
  result = oe_create_xgboost_enclave(
      argv[1], OE_ENCLAVE_TYPE_AUTO, flags, NULL, 0, &enclave);
  if (result != OE_OK) {
    fprintf(
        stderr,
        "oe_create_helloworld_enclave(): result=%u (%s)\n",
        result,
        oe_result_str(result));
    oe_terminate_enclave(enclave);
    return ret;
  }
#endif

  // TODO ecall error handling

  int silent = 0;
  int use_gpu = 0;  // set to 1 to use the GPU for training
  
  // load the data
  DMatrixHandle dtrain, dtest;
#ifdef __SGX__
  enclave_helloworld(enclave);
  safe_xgboost(enclave_XGDMatrixCreateFromFile(enclave, &ret, "../data/agaricus.txt.train", silent, &dtrain));
  safe_xgboost(enclave_XGDMatrixCreateFromFile(enclave, &ret, "../data/agaricus.txt.test", silent, &dtest));
#else
  safe_xgboost(XGDMatrixCreateFromFile("../data/agaricus.txt.train", silent, &dtrain));
  safe_xgboost(XGDMatrixCreateFromFile("../data/agaricus.txt.test", silent, &dtest));
#endif
  std::cout << "Data loaded" << std::endl;
  
  // create the booster
  BoosterHandle booster;
  DMatrixHandle eval_dmats[2] = {dtrain, dtest};
#ifdef __SGX__
  safe_xgboost(enclave_XGBoosterCreate(enclave, &ret, eval_dmats, 2, &booster));
#else
  safe_xgboost(XGBoosterCreate(eval_dmats, 2, &booster));
#endif
  std::cout << "Booster created" << std::endl;

  // configure the training
  // available parameters are described here:
  //   https://xgboost.readthedocs.io/en/latest/parameter.html
#ifdef __SGX__
  safe_xgboost(enclave_XGBoosterSetParam(enclave, &ret, booster, "tree_method", use_gpu ? "gpu_hist" : "hist"));
#else
  safe_xgboost(XGBoosterSetParam(booster, "tree_method", use_gpu ? "gpu_hist" : "hist"));
#endif
  std::cout << "First parameter set" << std::endl;
  if (use_gpu) {
    // set the number of GPUs and the first GPU to use;
    // this is not necessary, but provided here as an illustration
    safe_xgboost(XGBoosterSetParam(booster, "n_gpus", "1"));
    safe_xgboost(XGBoosterSetParam(booster, "gpu_id", "0"));
  } else {
    // avoid evaluating objective and metric on a GPU
#ifdef __SGX__
    safe_xgboost(enclave_XGBoosterSetParam(enclave, &ret, booster, "n_gpus", "0"));
#else
    safe_xgboost(XGBoosterSetParam(booster, "n_gpus", "0"));
#endif
  }

#ifdef __SGX__
  safe_xgboost(enclave_XGBoosterSetParam(enclave, &ret, booster, "objective", "binary:logistic"));
  safe_xgboost(enclave_XGBoosterSetParam(enclave, &ret, booster, "min_child_weight", "1"));
  safe_xgboost(enclave_XGBoosterSetParam(enclave, &ret, booster, "gamma", "0.1"));
  safe_xgboost(enclave_XGBoosterSetParam(enclave, &ret, booster, "max_depth", "3"));
  safe_xgboost(enclave_XGBoosterSetParam(enclave, &ret, booster, "verbosity", silent ? "0" : "2"));
#else
  safe_xgboost(XGBoosterSetParam(booster, "objective", "binary:logistic"));
  safe_xgboost(XGBoosterSetParam(booster, "min_child_weight", "1"));
  safe_xgboost(XGBoosterSetParam(booster, "gamma", "0.1"));
  safe_xgboost(XGBoosterSetParam(booster, "max_depth", "3"));
  safe_xgboost(XGBoosterSetParam(booster, "verbosity", silent ? "0" : "2"));
#endif
  std::cout << "All parameters set" << std::endl;
  
  // train and evaluate for 10 iterations
  int n_trees = 10;
  const char* eval_names[2] = {"train", "test"};
  const char* eval_result = NULL;
  for (int i = 0; i < n_trees; ++i) {
#ifdef __SGX__
    safe_xgboost(enclave_XGBoosterUpdateOneIter(enclave, &ret, booster, i, dtrain));
    safe_xgboost(enclave_XGBoosterEvalOneIter(enclave, &ret, booster, i, eval_dmats, eval_names, 2, &eval_result));
#else
    safe_xgboost(XGBoosterUpdateOneIter(booster, i, dtrain));
    safe_xgboost(XGBoosterEvalOneIter(booster, i, eval_dmats, eval_names, 2, &eval_result));
#endif
    printf("%s\n", eval_result);
  }

  // save model
  const char* fname = "demo_model.model";
#ifdef __SGX__
  safe_xgboost(enclave_XGBoosterSaveModel(enclave, &ret, booster, fname));
#else
  safe_xgboost(XGBoosterSaveModel(booster, fname));
#endif
  std::cout << "Saved model to demo_model.model" << std::endl;

  // load model
  booster = NULL;
#ifdef __SGX__
  safe_xgboost(enclave_XGBoosterCreate(enclave, &ret, eval_dmats, 2, &booster));
  safe_xgboost(enclave_XGBoosterLoadModel(enclave, &ret, booster, fname));
#else
  safe_xgboost(XGBoosterCreate(eval_dmats, 2, &booster));
  safe_xgboost(XGBoosterLoadModel(booster, fname));
#endif
  std::cout << "Loaded model" << std::endl;

  // predict
  bst_ulong out_len = 0;
  const float* out_result = NULL;
  int n_print = 10;
  
#ifdef __SGX__
  safe_xgboost(enclave_XGBoosterPredict(enclave, &ret, booster, dtest, 0, 0, &out_len, &out_result));
#else
  safe_xgboost(XGBoosterPredict(booster, dtest, 0, 0, &out_len, &out_result));
#endif
  printf("y_pred: ");
  for (int i = 0; i < n_print; ++i) {
    printf("%1.4f ", out_result[i]);
  }
  printf("\n");
  
  // print true labels
#ifdef __SGX__
  safe_xgboost(enclave_XGDMatrixGetFloatInfo(enclave, &ret, dtest, "label", &out_len, &out_result));
#else 
  safe_xgboost(XGDMatrixGetFloatInfo(dtest, "label", &out_len, &out_result));
#endif 
  printf("y_test: ");
  for (int i = 0; i < n_print; ++i) {
    printf("%1.4f ", out_result[i]);
  }
  printf("\n");

#ifdef __SGX__
  safe_xgboost(enclave_XGBoosterFree(enclave, &ret, booster));
  safe_xgboost(enclave_XGDMatrixFree(enclave, &ret, dtrain));
  safe_xgboost(enclave_XGDMatrixFree(enclave, &ret, dtest));
#else
  safe_xgboost(XGBoosterFree(booster));
  safe_xgboost(XGDMatrixFree(dtrain));
  safe_xgboost(XGDMatrixFree(dtest));
#endif
  return 0;
}

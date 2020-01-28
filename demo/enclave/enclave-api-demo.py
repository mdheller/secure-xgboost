import xgboost as xgb

OE_ENCLAVE_FLAG_DEBUG = 1
OE_ENCLAVE_FLAG_SIMULATE = 2

print("Creating enclave")

HOME_DIR="/root/mc2/code/secure-xgboost/"

# Uncomment below for enclave simulation mode
enclave = xgb.Enclave(HOME_DIR + "enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE), log_verbosity=3)
# enclave = xgb.Enclave("/root/mc2/code/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG))

# Remote Attestation
# print("Remote attestation")
# enclave.get_remote_report_with_pubkey()
# enclave.verify_remote_report_and_set_pubkey()

print("Creating training matrix")
dtrain = xgb.DMatrix(HOME_DIR + "demo/c-api/train.encrypted", encrypted=True)

print("Creating test matrix")
dtest = xgb.DMatrix(HOME_DIR + "demo/c-api/test.encrypted", encrypted=True) 

print("Creating Booster")
booster = xgb.Booster(cache=(dtrain, dtest))

print("Beginning Training")

# Set training parameters
params = {
        "tree_method": "hist",
        "n_gpus": "0",
        "objective": "binary:logistic",
        "min_child_weight": "1",
        "gamma": "0.1",
        "max_depth": "3",
        "verbosity": "3" 
}
booster.set_param(params)
print("All parameters set")

# Train and evaluate
n_trees = 10
for i in range(n_trees):
  booster.update(dtrain, i)
  print("Tree finished")
  print(booster.eval_set([(dtrain, "train"), (dtest, "test")], i))

# Predict
print("\n\nModel Predictions: ")
print(booster.predict(dtest)[:20])
print("\n\nTrue Labels: ")
print(dtest.get_label()[:20])

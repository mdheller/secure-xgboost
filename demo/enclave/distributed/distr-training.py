import xgboost as xgb
import os

OE_ENCLAVE_FLAG_DEBUG = 1
OE_ENCLAVE_FLAG_SIMULATE = 2

print("Creating enclave")

#  HOME_DIR="/root/mc2/code/secure-xgboost/"
HOME_DIR = "/home/xgb/secure-xgboost/"

# Uncomment below for enclave simulation mode
#  enclave = xgb.Enclave(HOME_DIR + "enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE))
enclave = xgb.Enclave("/home/xgb/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG))

# Remote Attestation
# print("Remote attestation")
# enclave.get_remote_report_with_pubkey()
# enclave.verify_remote_report_and_set_pubkey()

rabit_args = {
        "DMLC_NUM_WORKER": os.environ.get("DMLC_NUM_WORKER"),
        "DMLC_NUM_SERVER": os.environ.get("DMLC_NUM_SERVER"),
        "DMLC_TRACKER_URI": os.environ.get("DMLC_TRACKER_URI"),
        "DMLC_TRACKER_PORT": os.environ.get("DMLC_TRACKER_PORT"),
        "DMLC_ROLE": os.environ.get("DMLC_ROLE"),
        "DMLC_NODE_HOST": os.environ.get("DMLC_NODE_HOST")
}

rargs = [str.encode(str(k) + "=" + str(v)) for k, v in rabit_args.items()]

xgb.rabit.init(rargs)
xgb.rabit.tracker_print("We should see this message")

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

xgb.rabit.finalize()

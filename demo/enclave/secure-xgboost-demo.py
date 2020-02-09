import securexgboost as xgb
import os

OE_ENCLAVE_FLAG_DEBUG = 1
OE_ENCLAVE_FLAG_SIMULATE = 2

print("Creating enclave")
HOME_DIR = os.getcwd() + "/../../"

# Uncomment below for enclave simulation mode
enclave = xgb.Enclave(HOME_DIR + "enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE))
# enclave = xgb.Enclave(HOME_DIR + "enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG))

# Remote Attestation
# print("Remote attestation")
# enclave.get_remote_report_with_pubkey()
# enclave.verify_remote_report_and_set_pubkey()

print("Creating training matrix")
dtrain = xgb.DMatrix(HOME_DIR + "demo/c-api/train.encrypted", encrypted=True)

print("Creating test matrix")
dtest = xgb.DMatrix(HOME_DIR + "demo/c-api/test.encrypted", encrypted=True) 

print("Beginning Training")

# Set training parameters
params = {
        "tree_method": "hist",
        "n_gpus": "0",
        "objective": "binary:logistic",
        "min_child_weight": "1",
        "gamma": "0.1",
        "max_depth": "3",
        "verbosity": "1" 
}

# Train and evaluate
num_rounds = 10
booster = xgb.train(params, dtrain, num_rounds, evals=[(dtrain, "train"), (dtest, "test")])

# Predict
print("\n\nModel Predictions: ")
print(booster.predict(dtest)[:20])

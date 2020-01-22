import xgboost as xgb

OE_ENCLAVE_FLAG_DEBUG = 1
OE_ENCLAVE_FLAG_SIMULATE = 2

print("Creating enclave")

# Uncomment below for enclave simulation mode
# enclave = xgb.Enclave("/home/xgb/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE))
enclave = xgb.Enclave("/home/xgb/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG))

# Remote Attestation
print("Remote attestation")
enclave.get_remote_report_with_pubkey()
enclave.verify_remote_report_and_set_pubkey()

print("Creating training matrix")
dtrain = xgb.DMatrix("/home/xgb/secure-xgboost/demo/c-api/train.encrypted")

print("Creating test matrix")
dtest = xgb.DMatrix("/home/xgb/secure-xgboost/demo/c-api/test.encrypted") 

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

# Save model
# fname = "demo_model.model"
# booster.save_model(fname)

# Load model from scratch
# booster = None
# booster = xgb.Booster(cache=(dtrain, dtest))
# booster.load_model(fname)

# Predict
print("\n\nModel Predictions: ")
print(booster.predict(dtest)[:10])
print("\n\nTrue Labels: ")
print(dtest.get_label()[:10])

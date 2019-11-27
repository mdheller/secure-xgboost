# Based off demo/c-api/c-api.cc
import xgboost as xgb

OE_ENCLAVE_FLAG_DEBUG = 1
OE_ENCLAVE_FLAG_SIMULATE = 2

#  print("Creating enclave")
#  xgb.Enclave(enclave_image="/root/mc2/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE)

print("Creating training matrix")
dtrain = xgb.DMatrix("/root/mc2/secure-xgboost/demo/c-api/train.encrypted", open_enclave=True, enclave_image="/root/mc2/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE)
print("Creating test matrix")
dtest = xgb.DMatrix("/root/mc2/secure-xgboost/demo/c-api/test.encrypted") 
print("Data loaded")
booster = xgb.Booster(cache=(dtrain, dtest))
print("Booster created")

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
  print(booster.eval_set([(dtrain, "train"), (dtest, "test")], i))

# Save model
fname = "demo_model.model"
booster.save_model(fname)

# Load model from scratch
booster = None
booster = xgb.Booster(cache=(dtrain, dtest))
booster.load_model(fname)

# Predict
print("------ y_pred --------")
print(booster.predict(dtest)[:10])
print("------ y_test --------")
print(dtest.get_label()[:10])

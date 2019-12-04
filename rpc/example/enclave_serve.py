# Based off demo/c-api/c-api.cc
import xgboost as xgb
from rpc.remote_attestation_server import serve

OE_ENCLAVE_FLAG_DEBUG = 1
OE_ENCLAVE_FLAG_SIMULATE = 2

print("Creating enclave")

# Uncomment for simulation mode
# enclave = xgb.Enclave("/home/rishabh/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE))

# Uncomment for hardware mode
enclave = xgb.Enclave("/home/rishabh/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG))
print("Waiting for remote attestation...")
serve()

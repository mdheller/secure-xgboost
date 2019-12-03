# Based off demo/c-api/c-api.cc
import xgboost as xgb
from rpc.remote_attestation_server import serve

OE_ENCLAVE_FLAG_DEBUG = 1
OE_ENCLAVE_FLAG_SIMULATE = 2

print("Creating enclave")
# enclave = xgb.Enclave("/home/rishabh/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE))
enclave = xgb.Enclave("/home/rishabh/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG))
print("Remote attestation")
serve()

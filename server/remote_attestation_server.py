# Copyright 2015 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""The Python implementation of the GRPC RemoteAttestation server."""

from concurrent import futures
import logging

import grpc

import remote_attestation_pb2
import remote_attestation_pb2_grpc
import xgboost as xgb
from numproto import ndarray_to_proto, proto_to_ndarray
import numpy as np
import ctypes

def ctypes2numpy(cptr, length, dtype):
    """Convert a ctypes pointer array to a numpy array.
    """
    NUMPY_TO_CTYPES_MAPPING = {
            np.float32: ctypes.c_float,
            np.uint32: ctypes.c_uint,
            np.uint8: ctypes.c_uint8
            }
    if dtype not in NUMPY_TO_CTYPES_MAPPING:
        raise RuntimeError('Supported types: {}'.format(NUMPY_TO_CTYPES_MAPPING.keys()))
    ctype = NUMPY_TO_CTYPES_MAPPING[dtype]
    if not isinstance(cptr, ctypes.POINTER(ctype)):
        raise RuntimeError('expected {} pointer'.format(ctype))
    res = np.zeros(length, dtype=dtype)
    if not ctypes.memmove(res.ctypes.data, cptr, length * res.strides[0]):
        raise RuntimeError('memmove failed')
    return res

def pointer_to_proto(pointer, pointer_len, nptype=np.uint8):
    """
    Convert C u_int or float pointer to proto for RPC serialization

    Parameters
    ----------
    pointer : ctypes.POINTER
    pointer_len : length of pointer
    nptype : np type to cast to
        if pointer is of type ctypes.c_uint, nptype should be np.uint32
        if pointer is of type ctypes.c_float, nptype should be np.float32

    Returns:
        proto : proto.NDArray
    """
    ndarray = ctypes2numpy(pointer, pointer_len, nptype)
    proto = ndarray_to_proto(ndarray)
    return proto

def proto_to_pointer(proto):
    """
    Convert a serialized NDArray to a C pointer

    Parameters
    ----------
    proto : proto.NDArray

    Returns:
        pointer :  ctypes.POINTER(ctypes.u_int)
    """
    ndarray = proto_to_ndarray(proto)
    # FIXME make the ctype POINTER type configurable
    pointer = ndarray.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))
    return pointer

def xgb_load_train_predict():
    """
    This code will have been agreed upon by all parties before being run.
    """
    print("Creating training matrix")
    dtrain = xgb.DMatrix("/home/xgb/secure-xgboost/client/train.enc", encrypted=True)

    print("Creating test matrix")
    dtest = xgb.DMatrix("/home/xgb/secure-xgboost/client/test.enc", encrypted=True) 

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
            "verbosity": "1" 
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
    #  crypto = xgb.CryptoUtils()
    #  print("\n\nModel Predictions: ")
    # enc_preds is a c_char_p
    enc_preds, num_preds = booster.predict(dtest)

    #  print("\n\nDecrypt Predictions: ")
    #  # Decrypt Predictions
    #  preds = crypto.decrypt_predictions(sym_key, enc_preds, num_preds)
    #  print(preds)
    return enc_preds, num_preds

class RemoteAttestationServicer(remote_attestation_pb2_grpc.RemoteAttestationServicer):

    def GetAttestation(self, request, context):
        """
        Calls get_remote_report_with_public_key()
        """
        # Get a reference to the existing enclave
        enclave_reference = xgb.Enclave(create_enclave=False)

        # Get report from enclave
        enclave_reference.get_remote_report_with_pubkey()
        pem_key, key_size, remote_report, remote_report_size = enclave_reference.get_report_attrs()

        return remote_attestation_pb2.Report(pem_key=pem_key, key_size=key_size, remote_report=remote_report, remote_report_size=remote_report_size)

    def SendKey(self, request, context):
        """
        Sends encrypted symmetric key, signature over key, and filename of data that was encrypted using the symmetric key
        """
        # Get encrypted symmetric key, signature, and filename from request
        #  data_fname = request.data_fname
        enc_sym_key = request.enc_sym_key
        key_size = request.key_size
        signature = request.signature
        sig_len = request.sig_len

        crypto_utils = xgb.CryptoUtils()
        result = crypto_utils.add_client_key(enc_sym_key, key_size, signature, sig_len)

        return remote_attestation_pb2.Status(status=result)

    def SignalStart(self, request, context):
        """
        Signal to RPC server that client is ready to start
        """
        signal = request.status
        if signal == 1:
            try:
                # enc_preds is a c_char_p
                enc_preds, num_preds = xgb_load_train_predict()
                #  print(enc_preds.value)
                #  cptr = ctypes.POINTER(ctypes.c_float)(enc_preds.value)
                print("finish predicting")

                key_file = open("/home/xgb/secure-xgboost/client/key.txt", 'rb')
                sym_key = key_file.read() # The key will be type bytes
                key_file.close()
                
                crypto = xgb.CryptoUtils()
                enc_preds_proto = pointer_to_proto(enc_preds, num_preds * 8)
                unproto = proto_to_pointer(enc_preds_proto)
                preds = crypto.decrypt_predictions(sym_key, enc_preds, num_preds)
                print(preds)

                # Serialize encrypted predictions
                return remote_attestation_pb2.Predictions(predictions=enc_preds_proto, num_preds=num_preds, status=1)
            except Exception as e:
                print("Threw an exception\n")
                print(e)
                return remote_attestation_pb2.Predictions(predictions=None, num_preds=None, status=0)
        else:
            print("Signal != 1\n")
            return remote_attestation_pb2.Predictions(predictions=None, num_preds=None, status=0)


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    remote_attestation_pb2_grpc.add_RemoteAttestationServicer_to_server(RemoteAttestationServicer(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()

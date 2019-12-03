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

def xgb_load_train_predict():
    print("Creating test matrix")
    dtest = xgb.DMatrix("/home/rishabh/sample_data/test.encrypted") 

    print("Creating training matrix")
    dtrain = xgb.DMatrix("/home/rishabh/sample_data/train.encrypted")

    print("Creating booster")
    booster = xgb.Booster(cache=(dtrain, dtest))

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

    # Train and evaluate
    n_trees = 10
    for i in range(n_trees):
          booster.update(dtrain, i)
          print(booster.eval_set([(dtrain, "train"), (dtest, "test")], i))

    # Predict
    print("------ y_pred --------")
    print(booster.predict(dtest)[:10])
    print("------ y_test --------")
    print(dtest.get_label()[:10])



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
        Sends symmetric key and filenames of data
        """
        # Get encrypted symmetric key and filenames from request
        enc_sym_key = request.enc_sym_key
        training_data_fname = request.training_data_fname
        test_data_fname = request.test_data_fname

        # TODO: decrypt the symmetric key with the public key, save the filenames

        return remote_attestation_pb2.Status(status=1)

    def SignalStart(self, request, context):
        """
        Signal to RPC server that client is ready to start
        """
        signal = request.status
        if signal == 1:
            try:
                # enclave = xgb.Enclave(create_enclave=False)
                xgb_load_train_predict()
                # print("Creating training matrix")
                # dtrain = xgb.DMatrix("/home/rishabh/train.encrypted")
# 
                # print("Creating test matrix")
                # dtest = xgb.DMatrix("/home/rishabh/test.encrypted") 
# 
                # booster = xgb.Booster(cache=(dtrain, dtest))
                # print("Booster created")
# 
                # # Set training parameters
                # params = {
                        # "tree_method": "hist",
                        # "n_gpus": "0",
                        # "objective": "binary:logistic",
                        # "min_child_weight": "1",
                        # "gamma": "0.1",
                        # "max_depth": "3",
                        # "verbosity": "3" 
                        # }
                # booster.set_param(params)
# 
                # # Train and evaluate
                # n_trees = 10
                # for i in range(n_trees):
                    # booster.update(dtrain, i)
                    # print(booster.eval_set([(dtrain, "train"), (dtest, "test")], i))
# 
                # # Predict
                # print("------ y_pred --------")
                # print(booster.predict(dtest)[:10])
                # print("------ y_test --------")
                # print(dtest.get_label()[:10])
                return remote_attestation_pb2.Status(status=1)
            except:
                return remote_attestation_pb2.Status(status=-1)
        else:
            return remote_attestation_pb2.Status(status=-1)


def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    remote_attestation_pb2_grpc.add_RemoteAttestationServicer_to_server(RemoteAttestationServicer(), server)
    server.add_insecure_port('[::]:50051')
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()

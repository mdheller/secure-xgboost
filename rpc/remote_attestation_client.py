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
"""The Python implementation of the GRPC Remote Attestation client."""

from __future__ import print_function
import logging

import grpc
import base64

import remote_attestation_pb2
import remote_attestation_pb2_grpc

import xgboost as xgb

channel_addr = "40.68.150.201:50051"

def run():
    # Get remote report from enclave
    with grpc.insecure_channel(channel_addr) as channel:
        stub = remote_attestation_pb2_grpc.RemoteAttestationStub(channel)
        response = stub.GetAttestation(remote_attestation_pb2.Status(status=1))
    pem_key = response.pem_key
    key_size = response.key_size
    remote_report = response.remote_report
    remote_report_size = response.remote_report_size

    print("Report received from remote enclave")

    # Verify report
    enclave_reference = xgb.Enclave(create_enclave=False)
    enclave_reference.set_report_attrs(pem_key, key_size, remote_report, remote_report_size)
    enclave_reference.verify_remote_report_and_set_pubkey()

    print("Report successfully verified")

    keypair = "keypair.pem"
    crypto_utils = xgb.CryptoUtils()

    # Encrypt and sign symmetric key used to encrypt training data
    # fname must be path to file on server
    training_fname = "/home/rishabh/sample_data/agaricus.train.enc" 
    training_sym_key_path = "/root/mc2-client/train.key" 

    training_key_file = open(training_sym_key_path, 'rb')
    training_sym_key = training_key_file.read() # The key will be type bytes
    training_key_file.close()
    training_sym_key = base64.b64decode(training_sym_key)

    enc_sym_key_train, enc_sym_key_size_train = crypto_utils.encrypt_data_with_pk(training_sym_key, len(training_sym_key), pem_key, key_size)
    sig_train, sig_len_train = crypto_utils.sign_data(keypair, enc_sym_key_train, enc_sym_key_size_train) 

    # Encrypt and sign symmetric key used to encrypt test data
    test_fname = "/home/rishabh/sample_data/agaricus.test.enc" 
    test_sym_key_path= "/root/mc2-client/test.key" 

    test_key_file = open(test_sym_key_path, 'rb')
    test_sym_key = test_key_file.read() # The key will be type bytes
    test_key_file.close()
    test_sym_key = base64.b64decode(test_sym_key)

    enc_sym_key_test, enc_sym_key_size_test = crypto_utils.encrypt_data_with_pk(test_sym_key, len(test_sym_key), pem_key, key_size)
    sig_test, sig_len_test = crypto_utils.sign_data(keypair, enc_sym_key_test, enc_sym_key_size_test) 

    with grpc.insecure_channel(channel_addr) as channel:
        stub = remote_attestation_pb2_grpc.RemoteAttestationStub(channel)
        response = stub.SendKey(remote_attestation_pb2.DataMetadata(data_fname=training_fname, enc_sym_key=enc_sym_key_train, key_size=enc_sym_key_size_train, signature=sig_train, sig_len=sig_len_train))
        print("Symmetric key for training data sent to server")

        response = stub.SendKey(remote_attestation_pb2.DataMetadata(data_fname=test_fname, enc_sym_key=enc_sym_key_test, key_size=enc_sym_key_size_test, signature=sig_test, sig_len=sig_len_test))
        print("Symmetric key for test data sent to server")
        
    # Signal start
    with grpc.insecure_channel(channel_addr) as channel:
        stub = remote_attestation_pb2_grpc.RemoteAttestationStub(channel)
        response = stub.SignalStart(remote_attestation_pb2.Status(status=1))

    if response.status == 1:
        print("Training succeeded!")
    else:
        print("Training failed")
    


if __name__ == '__main__':
    logging.basicConfig()
    run()

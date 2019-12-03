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

import remote_attestation_pb2
import remote_attestation_pb2_grpc

import xgboost as xgb


def run():
    # Get remote report from enclave
    with grpc.insecure_channel('localhost:50051') as channel:
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

    # TODO: Encrypt symmetric key with public key pem_key
    # TODO: Get filenames of training data + test data
    enc_sym_key = 0
    training_data_fname = None
    test_data_fname = None

    with grpc.insecure_channel('localhost:50051') as channel:
        stub = remote_attestation_pb2_grpc.RemoteAttestationStub(channel)
        response = stub.SendKey(remote_attestation_pb2.DataMetadata(enc_sym_key=enc_sym_key, training_data_fname=training_data_fname, test_data_fname=test_data_fname))

    print("Symmetric key and filenames sent to server")

    with grpc.insecure_channel('localhost:50051') as channel:
        stub = remote_attestation_pb2_grpc.RemoteAttestationStub(channel)
        response = stub.SignalStart(remote_attestation_pb2.Status(status=1))

    if response.status == 1:
        print("Training succeeded!")
    else:
        print("Training failed")
    


if __name__ == '__main__':
    logging.basicConfig()
    run()

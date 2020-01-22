"""The Python implementation of the GRPC Remote Attestation client."""

from __future__ import print_function
import logging

import grpc
import base64

import remote_attestation_pb2
import remote_attestation_pb2_grpc

import xgboost as xgb
import argparse

def run(channel_addr, train_fname, train_sym_key_path, test_fname, test_sym_key_path, keypair):
    """
    The client will make 4 calls to the server that will run computation
    1. A call to retrieve the attestation report from the server. The client will use this report
    to verify that the it can trust the server.
    2. A call to send the symmetric key used to encrypt the training data to the server.
    3. A call to send the symmetric key used to encrypt the test data to the server.
    4. A call to commence computation.
    """
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

    crypto_utils = xgb.CryptoUtils()

    # Encrypt and sign symmetric key used to encrypt training data
    training_key_file = open(training_sym_key_path, 'rb')
    training_sym_key = training_key_file.read() # The key will be type bytes
    training_key_file.close()
    training_sym_key = base64.b64decode(training_sym_key)

    # Encrypt training data symmetric key
    enc_sym_key_train, enc_sym_key_size_train = crypto_utils.encrypt_data_with_pk(training_sym_key, len(training_sym_key), pem_key, key_size)
    print("Encrypted symmetric key")

    # Sign encrypted symmetric key
    sig_train, sig_len_train = crypto_utils.sign_data(keypair, enc_sym_key_train, enc_sym_key_size_train) 
    print("Signed ciphertext")

    # Encrypt and sign symmetric key used to encrypt test data
    test_key_file = open(test_sym_key_path, 'rb')
    test_sym_key = test_key_file.read() # The key will be type bytes
    test_key_file.close()
    test_sym_key = base64.b64decode(test_sym_key)

    enc_sym_key_test, enc_sym_key_size_test = crypto_utils.encrypt_data_with_pk(test_sym_key, len(test_sym_key), pem_key, key_size)
    sig_test, sig_len_test = crypto_utils.sign_data(keypair, enc_sym_key_test, enc_sym_key_size_test) 

    # Send training and test data keys to the server
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
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip-addr", help="server IP address", required=True)
    parser.add_argument("--train-fname", help="path to training data file on server", required=True)
    parser.add_argument("--train-key", help="path to key used to encrypt training data on client", required=True)
    parser.add_argument("--test-fname", help="path to test data file on server", required=True)
    parser.add_argument("--test-key", help="path to key used to encrypt test data on client", required=True)
    parser.add_argument("--keypair", help="path to keypair for signing data", required=True)

    args = parser.parse_args()

    channel_addr = str(args.ip_addr) + ":50051" 

    logging.basicConfig()
    run(channel_addr, str(args.train_fname), str(args.train_key), str(args.test_fname), str(args.test_key), str(args.keypair))

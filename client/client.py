from subprocess import check_call
import os
import binascii
import base64
import secrets

KEY_BYTES = 32

def generate_client_key(path_to_key):
    print("Generating client key...")
    key = os.urandom(KEY_BYTES)
    #  key = secrets.token_bytes(32)
    #  key = base64.b64encode(key)
    with open(path_to_key, "wb") as keyfile:
        keyfile.write(key)

def encrypt_file(input_path, output_path, client_key):
    print("Encrypting file {}".format(input_path))
    ret = check_call('./encrypt-file ' + input_path + ' ' + output_path + " " + client_key, shell=True)
    if ret != 0:
        print(ret)

def decrypt_file(input_path, output_path, client_key):
    print("Decrypting file {}".format(input_path))
    ret = check_call('./decrypt-file ' + input_path + ' ' + output_path + " " + client_key, shell=True)
    if ret != 0:
        print(ret)

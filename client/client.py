from subprocess import call
import os
import binascii

KEY_BYTES = 32

def generate_client_key(path_to_key):
    print("Generating client key...")
    key = binascii.hexlify(os.urandom(KEY_BYTES))
    with open(path_to_key, "wb") as keyfile:
        keyfile.write(key)

def encrypt_file(input_path, output_path, client_key):
    print("Encrypting file {}".format(input_path))
    call('./encrypt-file ' + input_path + ' ' + output_path + " " + client_key, shell=True)

def decrypt_file(input_path, output_path, client_key):
    print("Decrypting file {}".format(input_path))
    call('./decrypt-file ' + input_path + ' ' + output_path + " " + client_key, shell=True)

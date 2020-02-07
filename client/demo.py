from client import *

KEY_FILE = "key.txt"

# First generate a key that you will use to encrypt your data
generate_client_key(KEY_FILE)

# Next use the generated key to encrypt data
encrypt_file("../demo/data/agaricus.txt.train", "train.enc", KEY_FILE)

# Sanity check: decrypt the encrypted file and see if it matches the original
decrypt_file("train.enc", "train.ptxt", KEY_FILE)

# Client Setup Instructions
We'll be setting up the client so that it's ready to remotely initiate a job on its own data. **Before doing this setup, ensure that you've already setup the [server](../server)**
This setup will involve encrypting data on the client, transferring the data to the server, then initiating the XGBoost code. The code run by the server once the client makes the final call is in the `xgb_load_train_predict()` function in `server/remote_attestation_server.py`. In this example, data is decrypted and loaded into a `DMatrix`, a model is trained, and predictions are made. 

In this example, predictions are just printed to the console on the server. The user can, however, write code that runs on the server to encrypt the predictions and send them back to the client.  

### 1. Encrypt data locally.

Use the `ohe_encrypt.py` script to one hot encode and encrypt the data. The script assumes that the key used to encrypt the data is in the same directory under `key.txt`. The script takes in three arguments.

```
python3 ohe_encrypt.py --input_path <plaintext_data.csv> --output_path <desired_output_file_name.csv> --column_names <names_of_columns_to_one_hot_encode>
```

### 2. Send encrypted data to the server

We assume that there will be a mechanism to transfer the encrypted data to the server. For the purposes of this demo, the user can try, for example, `scp` to simulate this transfer.

### 3. Make client calls

On the client, make the aforementioned four calls to the server. 
The `remote_attestation_client.py` script takes in 5 arguments: server IP address, training data path on server, path to key used to encrypt training data on client, test data path on server, path to key used to encrypt test data on client.

```
python3 remote_attestation_client.py
```

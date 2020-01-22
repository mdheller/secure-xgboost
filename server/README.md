## Server Setup Instructions

In this example we'll need to start an RPC process on the server to listen for client calls. The RPC server listens for client calls to perform remote attestation, to accept the keys used to encrypt the data that will be used to train a model, and to start the XGBoost job.
### 1. Start RPC server

On the server with the enclave, start the RPC server to begin listening for client requests.

```
python3 enclave_serve.py
```
The code run by the server once the client makes the final call is in the `xgb_load_train_predict()` function in `rpc/remote_attestation_server.py`. In this example, data is decrypted and loaded into a `DMatrix`, a model is trained, and predictions are made. 

Once the console outputs "Waiting for remote attestation...", proceed to the [client](../client) setup.


# Installation

## Install the Open Enclave SDK (version 0.7) on Ubuntu 18.04
Follow the instructions [here](https://github.com/openenclave/openenclave/blob/master/docs/GettingStartedDocs/install_oe_sdk-Ubuntu_18.04.md).
You may also acquire a VM with the required features from [Azure Confidential Compute](https://azure.microsoft.com/en-us/solutions/confidential-compute/); in this case, however, you may need to manually upgrade the SDK installed in the VM to version 0.7:
```
sudo apt -y install open-enclave
```

Configure environment variables for Open Enclave SDK for Linux:
```
source /opt/openenclave/share/openenclave/openenclaverc
```

## Install secure XGBoost dependencies
```
sudo apt-get install -y libmbedtls-dev python3-pip
pip3 install numpy pandas sklearn numproto grpcio grpcio-tools kubernetes
```
Install ```cmake >= v3.11```. E.g., the following commands install ```cmake v3.15.6```.
```
wget https://github.com/Kitware/CMake/releases/download/v3.15.6/cmake-3.15.6-Linux-x86_64.sh
sudo bash cmake-3.15.6-Linux-x86_64.sh --skip-license --prefix=/usr/local
```

## Download and build secure XGBoost
```
git clone -b sgx-dev --recursive https://github.com/mc2-project/secure-xgboost.git
cd secure-xgboost
mkdir -p build

pushd build
cmake ..
make -j4
popd

mkdir enclave/build
pushd enclave/build
cmake ..
make -j4
popd

pushd python-package
sudo python3 setup.py install
popd
```

# Quickstart
We provide two examples. The first example is for users who are working on the same machine as the enclave, i.e. they have access to the enclave host. All computation will happen on this one machine. The second example is for users who want to remotely start a XGBoost job, i.e. there's a distinction between the server (the machine running training) and the client (a machine to which the user has direct access, but on which no computation is actually happening).

## Example 1
There is an example script, `enclave-api-demo.py`, that loads data, trains a model, and serves predictions at `demo/enclave/`. The example uses encrypted versions of the `agaricus.txt.train` and `agaricus.txt.test` data files from `demo/data/`. The encrypted data was generated using `demo/c-api/encrypt.cc`, with a key of all zeros.

You can run the example script with the following:
```
python3 demo/enclave/enclave-api-demo.py
```
## Example 2
This is an example of a scenario in which a party outsources all computation to a server with an enclave. This scenario can be extended to one in which *multiple* parties outsource all computation to the same central enclave, meaning that they collaborate by sending their data to the same location, at which a XGBoost model is trained over all parties' data.

In this example, the enclave server will start an RPC server to listen for client requests. The client will make four requests to the server: a request for remote attestation, a request to transfer the key used to encrypt the training data, a request to transfer the key used to encrypt the test data, and finally a request to start the XGBoost job.

This assumes that the client will have told the server what code to run -- the code to run in this example can be found in the `xgb_load_train_predict()` function in `remote_attestation_server.py`. 

1. Encrypt data locally.

Use the `ohe_encrypt.py` script to one hot encode and encrypt the data. The script assumes that the key used to encrypt the data is in the same directory under `key.txt`. The script takes in three arguments.

```
python3 ohe_encrypt.py --input_path <plaintext_data.csv> --output_path <desired_output_file_name.csv> --column_names <names_of_columns_to_one_hot_encode>
```

2. Send encrypted data to the server

We assume that there will be a mechanism to transfer the encrypted data to the server. For the purposes of this demo, the user can try, for example, `scp` to simulate this transfer.

3. Start RPC server

On the server with the enclave, start the RPC server to begin listening for client requests.

```
python3 rpc/example/enclave_serve.py
```

Once the console outputs "Waiting for remote attestation...", proceed to step 4. 

4. Make client calls

On the client, make the aforementioned four calls to the server. Be sure to change the IP address of the server in `remote_attestation_client.py`. 

```
python3 rpc/remote_attestation_client.py
```

The code run by the server once the client makes the final call is in the `xgb_load_train_predict()` function in `rpc/remote_attestation_server.py`. In this example, data is decrypted and loaded into a `DMatrix`, a model is trained, and predictions are made. 

In this example, predictions are just printed to the console on the server. The user can, however, write code that runs on the server to encrypt the predictions and send them back to the client. 


# API
Our implementation currently supports some XGBoost core data structure ([DMatrix](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.DMatrix) and [Booster](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.Booster)) methods.

We also support some additional enclave and cryptography specific functions. We've added the `Enclave` and `CryptoUtils` classes to the existing XGBoost library.

Below we list the functions we currently support. Please follow the respective links for each class for details about each function.

## Enclave
**Enclave(path_to_enclave, flags, create_enclave=True)**

> Constructor for enclave

> Params:
> * path_to_enclave: string
>> * path to built enclave
> * flags: int
>> * This is a bitwise OR of two OpenEnclaves-specific flags, `OE_ENCLAVE_FLAG_DEBUG = 1` and `OE_ENCLAVE_FLAG_SIMULATE = 2`. If you want to run in debug mode with actual hardware, pass `OE_ENCLAVE_FLAG_DEBUG` in. If you want to run non-debug simulation mode, pass `OE_ENCLAVE_FLAG_SIMULATE`. If you want to run in debug simulation mode, pass `OE_ENCLAVE_FLAG_DEBUG | OE_ENCLAVE_FLAG_SIMULATE`.
> * create_enclave: boolean
>> * whether you want to start an enclave. For example, if you just want to perform remote attestation as a client, there's no need to start an enclave on your client machine, but you will need to instantiate an enclave object to call the remote attestation methods.
  
 **Enclave.get_remote_report_with_pubkey()** 
 
> Retrieve the attestation report from the enclave
 
> *Note: This function should be called by the client (but run on the server) to perform remote attestation.*
 
 **Enclave.verify_remote_report_and_set_pubkey()** 
 
> Using the retrieved attestation report, verify that the enclave can be trusted.
 
> *Note: this function should be run on the client after calling `get_remote_report_with_pubkey()`.* 


## CryptoUtils
You can find example usage of the CryptoUtils class in `rpc/remote_attestation_client.py` and `rpc/remote_attestation_server.py`. For now, the CryptoUtils class is only necessary if you're adopting the client/server model of computation.

### Client Functions
The below functions would normally be run on the client.

**encrypt_data_with_pk(data, data_len, key, key_size)**

> Encrypt data to be transferred to server

**sign_data(keypair, data, data_size)**

> Sign data to be transferred to server
  
### Server Functions
The below functions would normally be run on the server.

**add_client_key(data_filename, key, key_size, signature, signature_length)**

> Store the key used to encrypt a specific data file and check that the key was sent by the client

## DMatrix
[**DMatrix(data)**](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.DMatrix)
* Constructor for DMatrix class

[**DMatrix.get_float_info()**](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.DMatrix.get_float_info)

[**DMatrix.get_label()**](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.DMatrix.get_label)

## Booster
[**Booster()**](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.Booster)
* Constructor for Booster class

[**Booster.set_param()**](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.Booster.set_param)

[**Booster.update()**](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.Booster.update)

[**Booster.eval_set()**](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.Booster.eval_set)

[**Booster.predict()**](https://xgboost.readthedocs.io/en/latest/python/python_api.html#xgboost.Booster.predict)

We are continuing to add support for more functions. If you'd like any specific functions, please file an issue. 

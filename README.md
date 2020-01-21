
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

First, perform [server setup](server/).
Next, perform [client setup](client/).

After server and client setup, the client should have initiated training and inference on the server. 
You can find the predictions outputted by the trained model on the console of the server.


# API
For XGBoost functionality (and additional functionality) we currently support, please look at our API [here](API.md)

cd /root/mc2/secure-xgboost/enclave/build
cmake ..
make -j4
cd /root/mc2/secure-xgboost/build
cmake ..
make -j4
cd /root/mc2/secure-xgboost/src/c_api
make build
cd /root/mc2/secure-xgboost/python-package
python3 setup.py install
python3 enclave-api-demo.py

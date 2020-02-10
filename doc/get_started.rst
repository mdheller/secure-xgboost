########################
Get Started with Secure XGBoost
########################

This is a quick start tutorial showing snippets for you to quickly try out Secure XGBoost
on the demo dataset on a binary classification task. This sanity check ensures that setup was done properly. This quickstart uses encrypted versions of the :code:`agaricus.txt.train` and :code:`agaricus.txt.test` data files from :code:`demo/data/`. The encrypted data was generated using :code:`demo/c-api/encrypt.cc`, with a key of all zeros.

********************************
Links to Other Helpful Resources
********************************
- See :doc:`Installation Guide </build>` on how to install Secure XGBoost.
- See :doc:`Text Input Format </tutorials/input_format>` on using text format for specifying training/testing data.

******
Python
******

Below is a snippet of the full Python demo located at :code:`secure-xgboost/demo/enclave/secure-xgboost-demo.py`. 

.. code-block:: python

   import securexgboost as xgb

   OE_ENCLAVE_FLAG_DEBUG = 1
   HOME_DIR = os.getcwd() + "/../../"

   enclave = xgb.Enclave(HOME_DIR + "enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG))

   dtrain = xgb.DMatrix(HOME_DIR + "demo/c-api/train.encrypted", encrypted=True)
   dtest = xgb.DMatrix(HOME_DIR + "demo/c-api/test.encrypted", encrypted=True) 

   params = {
           "objective": "binary:logistic",
           "gamma": "0.1",
           "max_depth": "3"
   }

   num_rounds = 10
   booster = xgb.train(params, dtrain, num_rounds)

   booster.predict(dtest)


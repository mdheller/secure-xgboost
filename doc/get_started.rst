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
- See :doc:`Tutorials </tutorials/index>` for tips and tutorials.
- See `Learning to use XGBoost by Examples <https://github.com/dmlc/xgboost/tree/master/demo>`_ for more code examples.

******
Python
******

Below is a snippet of the full Python demo located at :code:`secure-xgboost/demo/enclave/enclave-api-demo.py`.

.. code-block:: python

   import xgboost as xgb

   OE_ENCLAVE_FLAG_DEBUG = 1
   HOME_DIR = "/home/xgb/secure-xgboost/"

   enclave = xgb.Enclave("/home/xgb/secure-xgboost/enclave/build/xgboost_enclave.signed", flags=(OE_ENCLAVE_FLAG_DEBUG))

   dtrain = xgb.DMatrix(HOME_DIR + "demo/c-api/train.encrypted", encrypted=True)
   dtest = xgb.DMatrix(HOME_DIR + "demo/c-api/test.encrypted", encrypted=True) 

   booster = xgb.Booster(cache=(dtrain, dtest))
   params = {
           "tree_method": "hist",
           "objective": "binary:logistic",
           "min_child_weight": "1",
           "gamma": "0.1",
           "max_depth": "3",
           "verbosity": "3" 
   }
   booster.set_param(params)

   n_trees = 10
   for i in range(n_trees):
     booster.update(dtrain, i)
     booster.eval_set([(dtrain, "train"), (dtest, "test")], i))

   booster.predict(dtest)

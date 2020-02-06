###########################
Python Package Introduction
###########################
This document gives a basic walkthrough of xgboost python package.

**List of other Helpful Links**

* `Python walkthrough code collections <https://github.com/tqchen/xgboost/blob/master/demo/guide-python>`_
* :doc:`Python API Reference <python_api>`

Install XGBoost
---------------
To install XGBoost, follow instructions in :doc:`/build`.

To verify your installation, run the following in Python:

.. code-block:: python

  import securexgboost as xgb

Data Interface
--------------
The XGBoost python module is able to load data from:

- LibSVM text format file
- Comma-separated values (CSV) file

(See :doc:`/tutorials/input_format` for detailed description of text input format.)

The data is stored in a :py:class:`DMatrix <xgboost.DMatrix>` object.

* To load a libsvm text file or a XGBoost binary file into :py:class:`DMatrix <xgboost.DMatrix>`:

  .. code-block:: python

    dtrain = xgb.DMatrix('train.svm.txt')
    dtest = xgb.DMatrix('test.svm.buffer')

* To load a CSV file into :py:class:`DMatrix <xgboost.DMatrix>`:

  .. code-block:: python

    # label_column specifies the index of the column containing the true label
    dtrain = xgb.DMatrix('train.csv?format=csv&label_column=0')
    dtest = xgb.DMatrix('test.csv?format=csv&label_column=0')

  .. note:: Categorical features not supported

    Note that XGBoost does not support categorical features.

Setting Parameters
------------------
XGBoost can use either a list of pairs or a dictionary to set :doc:`parameters </parameter>`. For instance:

* Booster parameters

  .. code-block:: python

    param = {'max_depth': 2, 'eta': 1, 'silent': 1, 'objective': 'binary:logistic'}
    param['nthread'] = 4
    param['eval_metric'] = 'auc'

* You can also specify multiple eval metrics:

  .. code-block:: python

    param['eval_metric'] = ['auc', 'ams@0']

    # alternatively:
    # plst = param.items()
    # plst += [('eval_metric', 'ams@0')]

* Specify validations set to watch performance

  .. code-block:: python

    evallist = [(dtest, 'eval'), (dtrain, 'train')]

Training
--------

Training a model requires a parameter list and data set.

.. code-block:: python

  num_round = 10
  bst = xgb.train(param, dtrain, num_round, evallist)

Methods including `update` and `boost` from `xgboost.Booster` are designed for
internal usage only.  The wrapper function `xgboost.train` does some
pre-configuration including setting up caches and some other parameters.

Early Stopping
--------------
If you have a validation set, you can use early stopping to find the optimal number of boosting rounds.
Early stopping requires at least one set in ``evals``. If there's more than one, it will use the last.

.. code-block:: python

  train(..., evals=evals, early_stopping_rounds=10)

The model will train until the validation score stops improving. Validation error needs to decrease at least every ``early_stopping_rounds`` to continue training.

This works with both metrics to minimize (RMSE, log loss, etc.) and to maximize (MAP, NDCG, AUC). Note that if you specify more than one evaluation metric the last one in ``param['eval_metric']`` is used for early stopping.

Prediction
----------
A model that has been trained or loaded can perform predictions on data sets.

.. code-block:: python

  # 7 entities, each contains 10 features
  data = np.random.rand(7, 10)
  dtest = xgb.DMatrix(data)
  ypred = bst.predict(dtest)


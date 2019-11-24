SRC=c_api.cc
# TGT=c-api-demo
TGT_SGX=c_api_sgx

cc=g++
CFLAGS ?=-O3 
XGBOOST_ROOT ?=../..
INCLUDE_DIR=-I$(XGBOOST_ROOT)/include -I$(XGBOOST_ROOT)/dmlc-core/include -I$(XGBOOST_ROOT)/rabit/include
LIB_DIR=-L$(XGBOOST_ROOT)/lib
#LIB_DIR=-Wl,-rpath,$(XGBOOST_ROOT)/lib

SGX_SRC = $(SRC) ocalls.cc
SGX_CFLAGS = $(CFLAGS) -D__SGX__ -D__HOST__ $(shell pkg-config oehost-g++ --cflags)
SGX_LDFLAGS = $(shell pkg-config oehost-g++ --libs)

build: $(TGT_SGX)

# $(TGT): $(SRC) Makefile
	# $(cc) $(CFLAGS) $(INCLUDE_DIR) $(LIB_DIR) -o $(TGT) $(SRC) -lxgboost $(LDFLAGS)
# 
# run: $(TGT)
	# LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT)

$(TGT_SGX): $(SGX_SRC) Makefile
	oeedger8r ../../enclave/xgboost.edl --untrusted
	# $(cc) $(SGX_CFLAGS) $(INCLUDE_DIR) $(LIB_DIR) -o $(TGT_SGX) $(SGX_SRC) xgboost_u.c -lxgboost $(SGX_LDFLAGS)
	# $(cc) $(SGX_CFLAGS) $(INCLUDE_DIR) $(LIB_DIR) -o $(SGX_SRC) xgboost_u.c -lxgboost $(SGX_LDFLAGS)

sgx: $(TGT_SGX)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT_SGX) ../../enclave/build/xgboost_enclave.signed

debug: $(TGT_SGX)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib /opt/openenclave/bin/oegdb -arg ./$(TGT_SGX) ../../enclave/build/xgboost_enclave.signed --simulate

simulate : $(TGT_SGX)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT_SGX) ../../enclave/build/xgboost_enclave.signed --simulate

clean:
	# rm -f $(TGT) $(TGT_SGX)
	rm -f $(TGT_SGX)

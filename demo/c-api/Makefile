SRC=c-api-demo.cc
TGT=c-api-demo
TGT_SGX=c-api-demo-sgx
TGT_ENC=c-api-demo-enc

cc=g++
CFLAGS ?=-O3 -g
XGBOOST_ROOT ?=../..
INCLUDE_DIR=-I$(XGBOOST_ROOT)/include -I$(XGBOOST_ROOT)/dmlc-core/include -I$(XGBOOST_ROOT)/rabit/include
LIB_DIR=-L$(XGBOOST_ROOT)/lib -L/usr/lib/x86_64-linux-gnu/
#LIB_DIR=-Wl,-rpath,$(XGBOOST_ROOT)/lib

SGX_SRC = $(SRC)
SGX_CFLAGS = $(CFLAGS) -D__SGX__ -D__HOST__ $(shell pkg-config oehost-g++ --cflags)
SGX_LDFLAGS = $(shell pkg-config oehost-g++ --libs)

build: $(TGT)

$(TGT): $(SRC) Makefile
	$(cc) $(CFLAGS) $(INCLUDE_DIR) $(LIB_DIR) -o $(TGT) $(SRC) -lxgboost $(LDFLAGS)

$(TGT_ENC): encrypt-file.cc Makefile
	$(cc) $(CFLAGS) $(INCLUDE_DIR) $(LIB_DIR) -o $(TGT_ENC) encrypt-file.cc -lxgboost -lmbedtls -lmbedcrypto $(LDFLAGS)

run: $(TGT)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT)

encrypt: $(TGT_ENC)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT_ENC)

$(TGT_SGX): $(SGX_SRC) Makefile
	oeedger8r ../../enclave/xgboost.edl --untrusted
	$(cc) $(SGX_CFLAGS) $(INCLUDE_DIR) $(LIB_DIR) -o $(TGT_SGX) $(SGX_SRC) xgboost_u.c -lxgboost -lmbedtls -lmbedcrypto $(SGX_LDFLAGS)

sgx: $(TGT_SGX)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT_SGX) ../../enclave/build/xgboost_enclave.signed

debug: $(TGT_SGX)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib /opt/openenclave/bin/oegdb -arg ./$(TGT_SGX) ../../enclave/build/xgboost_enclave.signed --simulate

simulate: $(TGT_SGX)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib:/usr/lib/x86_64-linux-gnu/ ./$(TGT_SGX) ../../enclave/build/xgboost_enclave.signed --simulate

clean:
	rm -f $(TGT) $(TGT_SGX)

SRC=c-api-demo.cc ocalls.cc
TGT=c-api-demo

cc=g++
CFLAGS ?=-O3 -D__SGX__ -D__HOST__
XGBOOST_ROOT ?=../..
INCLUDE_DIR=-I$(XGBOOST_ROOT)/include -I$(XGBOOST_ROOT)/dmlc-core/include -I$(XGBOOST_ROOT)/rabit/include
LIB_DIR=-L$(XGBOOST_ROOT)/lib
#LIB_DIR=-Wl,-rpath,$(XGBOOST_ROOT)/lib

CFLAGS += $(shell pkg-config oehost-g++ --cflags)
LDFLAGS = $(shell pkg-config oehost-g++ --libs)

build: $(TGT)

$(TGT): $(SRC) Makefile
	oeedger8r ../../enclave/xgboost.edl --untrusted
	$(cc) $(CFLAGS) $(INCLUDE_DIR) $(LIB_DIR) -o $(TGT) $(SRC) xgboost_u.c -lxgboost $(LDFLAGS)

run: $(TGT)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT)

sgx: $(TGT)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT) ../../enclave/build/xgboost_enclave.signed

debug: $(TGT)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib /opt/openenclave/bin/oegdb -arg ./$(TGT) ../../enclave/build/xgboost_enclave.signed --simulate

simulate: $(TGT)
	LD_LIBRARY_PATH=$(XGBOOST_ROOT)/lib ./$(TGT) ../../enclave/build/xgboost_enclave.signed --simulate

clean:
	rm -f $(TGT)

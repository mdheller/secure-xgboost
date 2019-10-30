
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <xgboost/c_api.h>

#include "xgboost_u.h"

void host_helloworld() {
	  fprintf(stdout, "Enclave called into host to print: Hello World!\n");
}

int host_rabit__GetRank() {
    fprintf(stdout, "Ocall: rabit::GetRank\n");
	return ocall_rabit__GetRank();
}

int host_rabit__GetWorldSize() {
    fprintf(stdout, "Ocall: rabit::GetWorldSize\n");
	return ocall_rabit__GetWorldSize();
}

void* host_data__SimpleCSRSource() {
    fprintf(stdout, "Ocall: data::SimpleCSRSource\n");
    return ocall_data__SimpleCSRSource();
}

int host_rabit__IsDistributed() {
    fprintf(stdout, "Ocall: rabit::IsDistributed\n");
    return ocall_rabit__IsDistributed();
}

void* host_dmlc__Parser__Create(char* fname, int partid, int npart, char* file_format) {
    fprintf(stdout, "Ocall: dmlc::Parser::Create\n");
    return ocall_dmlc__Parser__Create(fname, partid, npart, file_format);
}

void* host_ObjFunction__Create(char* name) {
    fprintf(stdout, "Ocall: ObjFunction::Create\n");
    //fprintf(stdout, "Name: ");
    //fprintf(stdout, name);
    //fprintf(stdout, "\n");
    return ocall_ObjFunction__Create(name);
}


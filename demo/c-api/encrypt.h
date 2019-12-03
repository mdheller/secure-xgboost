
#ifndef DEMO_ENCRYPT_H
#define DEMO_ENCRYPT_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xgboost/c_api.h>
#include <dmlc/base64.h>

#include <mbedtls/entropy.h>    // mbedtls_entropy_context
#include <mbedtls/ctr_drbg.h>   // mbedtls_ctr_drbg_context
#include <mbedtls/cipher.h>     // MBEDTLS_CIPHER_ID_AES
#include <mbedtls/gcm.h>        // mbedtls_gcm_context
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>

#define KEY_BYTES 32
#define IV_BYTES 12
#define TAG_BYTES 16

static char test_key[KEY_BYTES] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

void encryptFile(char* fname, char* e_fname);
void decryptFile(char* fname, char* d_fname);

void encryptDataWithPublicKey(char* data, size_t len, uint8_t* pem_key, size_t key_size, uint8_t* encrypted_data, size_t* encrypted_data_size);

#endif

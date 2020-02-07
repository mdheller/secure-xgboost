
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"

#include <mbedtls/entropy.h>    // mbedtls_entropy_context
#include <mbedtls/ctr_drbg.h>   // mbedtls_ctr_drbg_context
#include <mbedtls/cipher.h>     // MBEDTLS_CIPHER_ID_AES
#include <mbedtls/gcm.h>        // mbedtls_gcm_context

#define KEY_BYTES 32
#define IV_BYTES 12
#define TAG_BYTES 16

// Create a key of KEY_BYTES bytes
void generate_client_key(char* k_fname) {

  unsigned char key[KEY_BYTES];
}

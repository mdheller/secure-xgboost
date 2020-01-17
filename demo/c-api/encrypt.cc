
#include "encrypt.h"

// Creates a CSV file with format "IV,tag,encrypt(line)" (base64 encoded) for each line in input file
void encryptFile(char* fname, char* e_fname) {
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  mbedtls_gcm_context gcm;

  unsigned char key[KEY_BYTES];
  unsigned char iv[IV_BYTES];
  unsigned char tag[TAG_BYTES];

  // Initialize the entropy pool and the random source
  mbedtls_entropy_init( &entropy );
  mbedtls_ctr_drbg_init( &ctr_drbg );
  // Initialize GCM context (just makes references valid) - makes the context ready for mbedtls_gcm_setkey()
  mbedtls_gcm_init(&gcm);
  // The personalization string should be unique to your application in order to add some
  // personalized starting randomness to your random sources.
  char *pers = "aes generate key for MC^2";
  // CTR_DRBG initial seeding Seed and setup entropy source for future reseeds
  int ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy, (unsigned char *)pers, strlen(pers) );
  if( ret != 0 )
  {
    printf( "mbedtls_ctr_drbg_seed() failed - returned -0x%04x\n", -ret );
    exit(1);
  }
  // Extract data for your key, in this case we generate 32 bytes (256 bits) of random data
  ret = mbedtls_ctr_drbg_random( &ctr_drbg, key, KEY_BYTES );
  if( ret != 0 ) {
    printf( "mbedtls_ctr_drbg_random failed to extract key - returned -0x%04x\n", -ret );
    exit(1);
  }
  // Set key to test_key for testing
  memcpy(key, test_key, KEY_BYTES);

  // Initialize the GCM context with our key and desired cipher
  ret = mbedtls_gcm_setkey(&gcm,                      // GCM context to be initialized
      MBEDTLS_CIPHER_ID_AES,     // cipher to use (a 128-bit block cipher)
      key,                       // encryption key
      KEY_BYTES * 8);            // key bits (must be 128, 192, or 256)
  if( ret != 0 ) {
    printf( "mbedtls_gcm_setkey failed to set the key for AES cipher - returned -0x%04x\n", -ret );
    exit(1);
  }

  std::ifstream infile(fname);
  std::ofstream myfile;
  myfile.open(e_fname);
  //myfile.open(e_fname, std::ios::out | std::ios::binary);

  std::string line;
  size_t index = 0;
  while (std::getline(infile, line)) {
    size_t length = strlen(line.c_str());

    // Extract data for your IV, in this case we generate 12 bytes (96 bits) of random data
    ret = mbedtls_ctr_drbg_random( &ctr_drbg, iv, IV_BYTES );
    if( ret != 0 ) {
        printf( "mbedtls_ctr_drbg_random failed to extract IV - returned -0x%04x\n", -ret );
        exit(1);
    }

    unsigned char* encrypted = (unsigned char*) malloc(length*sizeof(char));
    // We use line index as AEAD data to prevent tampering across lines
    const unsigned char* add_data = (const unsigned char*) &index;
    ret = mbedtls_gcm_crypt_and_tag( 
        &gcm,                                       // GCM context
        MBEDTLS_GCM_ENCRYPT,                        // mode
        length,                                     // length of input data
        iv,                                         // initialization vector
        IV_BYTES,                                   // length of IV
#if true // temporary macro for testing
        add_data,                                   // additional data
        // FIXME make this independent of platform
        sizeof(size_t),                             // length of additional data
#else
        NULL,                                       // additional data
        0,                                          // length of additional data
#endif 
        (const unsigned char*)line.c_str(),         // buffer holding the input data
        encrypted,                                  // buffer for holding the output data
        TAG_BYTES,                                  // length of the tag to generate
        tag);                                       // buffer for holding the tag
    index++;
    if( ret != 0 ) {
      printf( "mbedtls_gcm_crypt_and_tag failed to encrypt the data - returned -0x%04x\n", -ret );
      exit(1);
    }
    std::string encoded = dmlc::data::base64_encode(iv, IV_BYTES);
    myfile 
      << dmlc::data::base64_encode(iv, IV_BYTES) << ","
      << dmlc::data::base64_encode(tag, TAG_BYTES) << ","
      << dmlc::data::base64_encode(encrypted, length) << "\n";
    free(encrypted);
  }
  infile.close();
  myfile.close();
}

void decryptFile(char* fname, char* d_fname) {
  mbedtls_gcm_context gcm;
  unsigned char key[KEY_BYTES];

  // Initialize GCM context (just makes references valid) - makes the context ready for mbedtls_gcm_setkey()
  mbedtls_gcm_init(&gcm);
  // Set key to 0 for testing
  memset(key, 0, KEY_BYTES);
  int ret = mbedtls_gcm_setkey(&gcm,                // GCM context to be initialized
      MBEDTLS_CIPHER_ID_AES,                        // cipher to use (a 128-bit block cipher)
      key,                                          // encryption key
      KEY_BYTES * 8);                               // key bits (must be 128, 192, or 256)
  if( ret != 0 ) {
    printf( "mbedtls_gcm_setkey failed to set the key for AES cipher - returned -0x%04x\n", -ret );
    exit(1);
  }

  std::ifstream infile(fname);
  std::ofstream myfile;
  myfile.open(d_fname);

  std::string line;
  size_t index = 0;
  while (std::getline(infile, line)) {
    const char* data = line.c_str();
    //int iv_pos = line.find(",", 0);
    //int tag_pos = line.find(",", iv_pos + 1);
    char* p = const_cast<char*>(data);
    int iv_pos = 0;
    while(*p != '\0' && *p != ',') {
      p++;
      iv_pos++;
    }
    p++;
    int tag_pos = iv_pos + 1;
    while(*p != '\0' && *p != ',') {
      p++;
      tag_pos++;
    }

    size_t out_len;
    char tag[TAG_BYTES];
    char iv[IV_BYTES];

    char* ct = (char *) malloc(line.size() * sizeof(char));

    out_len = dmlc::data::base64_decode(data, iv_pos, iv);
    out_len = dmlc::data::base64_decode(data + iv_pos + 1, tag_pos - iv_pos, tag);
    out_len = dmlc::data::base64_decode(data + tag_pos + 1, line.size() - tag_pos, ct);

    unsigned char* decrypted = (unsigned char*) malloc((out_len + 1) * sizeof(char));
    const unsigned char* add_data = (const unsigned char*) &index;
    int ret = mbedtls_gcm_auth_decrypt(
        &gcm,                                     // GCM context
        out_len,                                  // length of the input ciphertext data (always same as plain)
        (const unsigned char*) iv,                // initialization vector
        IV_BYTES,                                 // length of IV
#if true // temporary macro for testing
        add_data,                                 // additional data
        // FIXME make this independent of platform
        sizeof(size_t),                           // length of additional data
#else
        NULL,                                     // additional data
        0,                                        // length of additional data
#endif 
        (const unsigned char*) tag,               // buffer holding the tag
        TAG_BYTES,                                // length of the tag
        (const unsigned char*) ct,                // buffer holding the input ciphertext data
        decrypted);                               // buffer for holding the output decrypted data
    index++;
    decrypted[out_len] = '\0';
    free(ct);
    if (ret != 0) {
      std::cout << "mbedtls_gcm_auth_decrypt failed with error " << -ret << std::endl;
      exit(1);
    }
    myfile << decrypted << "\n";
  }
  infile.close();
  myfile.close();
}

int compute_sha256(const uint8_t* data, size_t data_size, uint8_t sha256[32]) {
  int ret = 0;
  mbedtls_sha256_context ctx;

#define safe_sha(call) {                \
int ret = (call);                       \
if (ret) {                              \
  mbedtls_sha256_free(&ctx);            \
  return -1;                            \
}                                       \
}
  mbedtls_sha256_init(&ctx);
  safe_sha(mbedtls_sha256_starts_ret(&ctx, 0));
  safe_sha(mbedtls_sha256_update_ret(&ctx, data, data_size));
  safe_sha(mbedtls_sha256_finish_ret(&ctx, sha256));

  mbedtls_sha256_free(&ctx);
  return ret;
}

void verifySignature(char *pkfile, uint8_t* data, size_t data_size, uint8_t* signature, size_t sig_len) {
  mbedtls_pk_context pk;
  unsigned char hash[32];
  int ret = 1;

  mbedtls_pk_init( &pk );

  if((ret = mbedtls_pk_parse_public_keyfile(&pk, pkfile)) != 0) {
    printf(" failed\n  ! Could not read key\n");
    printf("  ! mbedtls_pk_parse_public_keyfile returned %d\n\n", ret);
    exit(1);
  }

  if(!mbedtls_pk_can_do(&pk, MBEDTLS_PK_RSA)) {
    printf( " failed\n  ! Key is not an RSA key\n" );
    exit(1);
  }

  mbedtls_rsa_set_padding( mbedtls_pk_rsa( pk ), MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256 );

  if((ret = compute_sha256(data, data_size, hash)) != 0) {
    printf( " failed\n  ! Could not hash\n\n");
    exit(1);
  }

  if((ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, 0, signature, sig_len)) != 0 ) {
    printf( " failed\n  ! mbedtls_pk_verify returned %d\n\n", ret );
    exit(1);
  }

  mbedtls_pk_free( &pk );
}

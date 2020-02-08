#include "ssl_socket.h"
#include "../include/dmlc/logging.h"
#include "ssl_context_manager.h"
#include "certs.h"

namespace rabit {
namespace utils {

namespace {}  // namespace
bool SSLTcpSocket::SSLConnect(const SockAddr &addr) {
  //mbedtls_net_init(&net);
  mbedtls_ssl_config_init(&conf);
  mbedtls_x509_crt_init(&cacert);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  // seeds and sets up entropy source
  mbedtls_entropy_init(&entropy);
  int ret;
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
    print_err(ret);
  }

  if (Connect(addr)) {
    mbedtls_ssl_init(&ssl_);

    // configure TLS layer
    if ((ret = mbedtls_ssl_config_defaults(&conf,
        MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
      print_err(ret);
    }
    // FIXME add certificate auth (currently not verifying) 
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    // configure RNG
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);


    // FIXME disable debug
    mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );
    mbedtls_debug_set_threshold(DEBUG_LEVEL);


    // set up SSL context
    if ((ret = mbedtls_ssl_setup(ssl(), &conf)) != 0) {
      print_err(ret);
    }
    // configure hostname
    // FIXME set hostname to check against cert
    //if ((ret = mbedtls_ssl_set_hostname(&ssl, addr.AddrStr().c_str())) != 0) {
      //print_err(ret);
    //  Error("Error: Could not set hostname");
    //}

    // configure input/output functions for sending data
    mbedtls_net_init(&net);
    net.fd = this->sockfd;
    mbedtls_ssl_set_bio(ssl(), &net, mbedtls_net_send, mbedtls_net_recv, NULL);

    // perform handshake
    while ((ret = mbedtls_ssl_handshake(ssl())) != 0) {
      if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
        print_err(ret);
        return false;
      }
    }
    return true;
  }
  return false;
}

void SSLTcpSocket::SSLAccept(SSLTcpSocket* client_sock) {
  SOCKET client_fd = Accept();

  client_sock->sockfd = client_fd;
  mbedtls_ssl_config_init(&client_sock->conf);
  int ret;

  mbedtls_x509_crt srvcert;
  mbedtls_x509_crt cachain;
  mbedtls_pk_context pkey;

  mbedtls_x509_crt_init(&srvcert);
  mbedtls_x509_crt_init(&cachain);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  // seeds and sets up entropy source
  mbedtls_entropy_init(&entropy);
  if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) {
    print_err(ret);
  }

  // FIXME currently using inbuilt certs / key
  ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_srv_crt_ec, mbedtls_test_srv_crt_ec_len );
  if( ret != 0 ) {
    print_err(ret);
  }
  ret = mbedtls_x509_crt_parse( &cachain, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len );
  if( ret != 0 ) {
    print_err(ret);
  }
  mbedtls_pk_init( &pkey );
  ret =  mbedtls_pk_parse_key( &pkey, (const unsigned char *) mbedtls_test_srv_key_ec, mbedtls_test_srv_key_ec_len, NULL, 0 );
  if( ret != 0 ) {
    print_err(ret);
  }
  mbedtls_ssl_conf_ca_chain( &client_sock->conf, &cachain, NULL );
  if( ( ret = mbedtls_ssl_conf_own_cert( &client_sock->conf, &srvcert, &pkey ) ) != 0 ) {
    print_err(ret);
  }

  if( ( ret = mbedtls_ssl_config_defaults( &client_sock->conf,
          MBEDTLS_SSL_IS_SERVER,
          MBEDTLS_SSL_TRANSPORT_STREAM,
          MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 ) {
    print_err(ret);
  }

  mbedtls_ssl_conf_rng( &client_sock->conf, mbedtls_ctr_drbg_random, &ctr_drbg );

  // FIXME disable debug
  mbedtls_ssl_conf_dbg( &conf, my_debug, stdout );
  mbedtls_debug_set_threshold(DEBUG_LEVEL);

  // Make sure memory refs are valid
  mbedtls_ssl_init( &client_sock->ssl_);

  if ((ret = mbedtls_ssl_setup(&client_sock->ssl_, &client_sock->conf)) != 0) {
    print_err(ret);
  }

  mbedtls_net_init(&client_sock->net);
  client_sock->net.fd = client_sock->sockfd;
  mbedtls_ssl_set_bio(&client_sock->ssl_, &client_sock->net,  mbedtls_net_send, mbedtls_net_recv, NULL);

  while( ( ret = mbedtls_ssl_handshake(&client_sock->ssl_ ) ) != 0 ) {
    if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
      print_err(ret);
    }
  }
}

size_t SSLTcpSocket::SSLSendAll(const void *buf_, size_t len) {
  const char *buf = reinterpret_cast<const char *>(buf_);
  size_t ndone = 0;
  while (ndone < len) {
    ssize_t ret = SSLSend(buf, static_cast<ssize_t>(len - ndone));
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret < 0) {
      if (LastErrorWouldBlock())
        return ndone;
      print_err(ret);
    }
    buf += ret;
    ndone += ret;
  }
  return ndone;
}

size_t SSLTcpSocket::SSLRecvAll(void *buf_, size_t len) {
  char *buf = reinterpret_cast<char *>(buf_);
  size_t ndone = 0;
  while (ndone < len) {
    ssize_t ret = SSLRecv(buf, static_cast<sock_size_t>(len - ndone));
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret < 0) {
      if (LastErrorWouldBlock())
        return ndone;
      print_err(ret);
    }
    if (ret == 0) return ndone;
    buf += ret;
    ndone += ret;
  }
  return ndone;
}

} /* namespace utils */
} /* namespace rabit */

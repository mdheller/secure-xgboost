#ifndef RABIT_TLS_SOCKET_H_
#define RABIT_TLS_SOCKET_H_

#include "socket.h"
#include "ssl_context_manager.h"
#include "mbedtls/debug.h"


namespace rabit {
namespace utils {

static void print_err(int error_code) {
  const size_t LEN = 2048;
  char err_buf[LEN];
  mbedtls_strerror(error_code, err_buf, LEN);
  mbedtls_printf(" ERROR %d: %s\n", getpid(), err_buf);
  exit(1);
}

#define DEBUG_LEVEL 0

static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
    fflush(  (FILE *) ctx  );
}

class SSLTcpSocket : public TCPSocket {
 public:
  //using SharedSSL = std::shared_ptr<mbedtls_ssl_context>;
  using SharedSSL = mbedtls_ssl_context;

  // unique_ptr deleter got called only get() != nullptr.
  SSLTcpSocket() : TCPSocket(), ssl_() {}

  SSLTcpSocket(SOCKET sockfd) : TCPSocket(sockfd), ssl_() {}

  SSLTcpSocket(SOCKET sockfd, SharedSSL ssl) : TCPSocket(sockfd), ssl_(ssl) {}


  // Indicate has ssl context.
  //bool HasSSL() const { return ssl_.get() != nullptr; }

  void SetSSL(SharedSSL ssl) {
    //if (HasSSL()) {
    //  Socket::Error("Already has ssl.");
    //}
    ssl_ = ssl;
  }

  SSLTcpSocket* getPtr() {
    return this;
  }

  // SSL Accept.
  void SSLAccept(SSLTcpSocket* client_sock);

  // SSL Connect.
  bool SSLConnect(const SockAddr &addr);

  void setBio() {
    mbedtls_net_init(&net);
    net.fd = this->sockfd;
    mbedtls_ssl_set_bio(ssl(), &net, mbedtls_net_send, mbedtls_net_recv, NULL);
  }

  // SSL Write, note this does not support |flag| argument.
  ssize_t SSLSend(const void *buf, size_t len) {
    int ret = -1;
    while( ( ret = mbedtls_ssl_write( ssl(), (const unsigned char*)buf, len ) ) <= 0 )
    {
        if( ret == MBEDTLS_ERR_NET_CONN_RESET )
        {
          printf("SEND Error %x %d\n", ssl(), len);
          print_err(ret);
          return -1;
        }

        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
        {
          printf("SEND Error %x %d\n", ssl(), len);
          print_err(ret);
          return -1;
        }
    }
    printf("%d SEND SUcess %x %d\n", getpid(), ssl(), len);
    return ret;
  }

  // SSL Read, note this does not support |flag| argument.
  ssize_t SSLRecv(void *buf, size_t len) { 
    int ret = -1;
    do {
        ret = mbedtls_ssl_read(ssl(), (unsigned char*)buf, len);

        if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
            continue;

        if ( ret > (int)len ) {
            printf("%d RECVD more bytes than expected %d %d\n", getpid(), ret, len);
        }

        if( ret <= 0 )
        {
            printf("%d RECV Error %x %x %d %d\n", getpid(), this, ssl(), len, net.fd);
            switch( ret )
            {
                case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
                  print_err(ret);
                  return -1;

                case MBEDTLS_ERR_NET_CONN_RESET:
                  print_err(ret);
                  return -1;

                default:
                  print_err(ret);
                  return -1;
            }
        }

        if( ret > 0 )
            break;
    } while( 1 );
    printf("%d RECV SUccess %x %x %d %d\n", getpid(), this, ssl(), len, net.fd);
    return ret;
  }

  // SSL SendAll
  size_t SSLSendAll(const void *buf_, size_t len);

  // SSL RecvAll
  size_t SSLRecvAll(void *buf_, size_t len);

  /*!
   * \brief send a string over network
   * \param str the string to be sent
   */
  void SSLSendStr(const std::string &str) {
    int len = static_cast<int>(str.length());
    utils::Assert(this->SSLSendAll(&len, sizeof(len)) == sizeof(len),
                  "error during send SendStr");
    if (len != 0) {
      utils::Assert(this->SSLSendAll(str.c_str(), str.length()) == str.length(),
                    "error during send SendStr");
    }
  }

  /*!
   * \brief recv a string from network
   * \param out_str the string to receive
   */
  void SSLRecvStr(std::string *out_str) {
    int len;
    utils::Assert(this->SSLRecvAll(&len, sizeof(len)) == sizeof(len),
                  "error during send RecvStr");
    out_str->resize(len);
    if (len != 0) {
      utils::Assert(this->SSLRecvAll(&(*out_str)[0], len) == out_str->length(),
                    "error during send SendStr");
    }
  }

  mbedtls_ssl_context *ssl() { return &ssl_; }
  mbedtls_ssl_config *conf_() { return &conf; }

  SharedSSL ssl_;

  mbedtls_net_context net;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_config conf;
  mbedtls_entropy_context entropy;
  mbedtls_x509_crt cacert;
    private:
};

}  // namespace utils
}  // namespace rabit

#endif  // RABIT_TLS_SOCKET_H_

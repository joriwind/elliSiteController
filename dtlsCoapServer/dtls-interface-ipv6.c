/* server-dtls.c 
 *
 * Copyright (C) 2006-2014 wolfSSL Inc.
 *
 * This file is part of CyaSSL.
 *
 * CyaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CyaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  
 * USA
 *=============================================================================
 *
 * Bare-bones example of a DTLS erver for instructional/learning purposes.
 * Utilizes DTLS 1.2.
 */

#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <cyassl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define SERV_PORT   5683           /* define our server port number */
#define MSGLEN      4096
#define SERV_ADDRESS ::1

static int cleanup;                 /* To handle shutdown */

/* Callback function*/
typedef int (*callbackFt)(char* message);

int initDTLS(WOLFSSL_CTX** ctx, char* verifyCert, char* ourCert, char* ourKey, int isServer);
int connectToServer(WOLFSSL** ssl, WOLFSSL_CTX** ctx, char* host, int port);    /* Separate out Handling Datagrams */
int awaitConnection(WOLFSSL** ssl, WOLFSSL_CTX** ctx, int port);
void readDTLS(WOLFSSL** ssl, callbackFt fct);
void writeDTLS(WOLFSSL** ssl, char* message);
void closeDTLS();

WOLFSSL_CTX** getTypeWOLFSSL_CTX(void);
WOLFSSL** getTypeWOLFSSL(void);

enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};

WOLFSSL_CTX** getTypeWOLFSSL_CTX(void){
   WOLFSSL_CTX* ctx;
   return &ctx;
}

WOLFSSL** getTypeWOLFSSL(void){
   WOLFSSL* ssl;
   return &ssl;
}

static INLINE int tcp_select(int socketfd, int to_sec)
{
   fd_set recvfds, errfds;
   int nfds = socketfd + 1;
   struct timeval timeout = { (to_sec > 0) ? to_sec : 0, 0};
   int result;

   FD_ZERO(&recvfds);
   FD_SET(socketfd, &recvfds);
   FD_ZERO(&errfds);
   FD_SET(socketfd, &errfds);

   result = select(nfds, &recvfds, NULL, &errfds, &timeout);

   if (result == 0)
     return TEST_TIMEOUT;
   else if (result > 0) {
     if (FD_ISSET(socketfd, &recvfds))
         return TEST_RECV_READY;
     else if(FD_ISSET(socketfd, &errfds))
         return TEST_ERROR_READY;
   }

   return TEST_SELECT_FAIL;
}

static INLINE int myVerify(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
    (void)preverify;
    char buffer[WOLFSSL_MAX_ERROR_SZ];

#ifdef OPENSSL_EXTRA
    WOLFSSL_X509* peer;
#endif

    printf("In verification callback, error = %d, %s\n", store->error,
                                 wolfSSL_ERR_error_string(store->error, buffer));
#ifdef OPENSSL_EXTRA
    peer = store->current_cert;
    if (peer) {
        char* issuer  = wolfSSL_X509_NAME_oneline(
                                       wolfSSL_X509_get_issuer_name(peer), 0, 0);
        char* subject = wolfSSL_X509_NAME_oneline(
                                      wolfSSL_X509_get_subject_name(peer), 0, 0);
        printf("peer's cert info:\n issuer : %s\n subject: %s\n", issuer,
                                                                  subject);
        XFREE(subject, 0, DYNAMIC_TYPE_OPENSSL);
        XFREE(issuer,  0, DYNAMIC_TYPE_OPENSSL);
    }
    else
        printf("peer has no cert!\n");
#endif
    printf("Subject's domain name is %s\n", store->domain);

    printf("Allowing to continue anyway (shouldn't do this, EVER!!!)\n");
    return 1;
}


/**
 * Use defined WOLFSSL_CTX to connect to host on port port
 * @return int succesful? WOLFSSL* object, connected with host on port port
**/
int connectToServer(WOLFSSL** ssl, WOLFSSL_CTX** ctx, char* host, int port)
{
   if(*ctx == NULL){
      printf("Need to init ctx first\n");
      return -1;
   }else{
      printf("Ctx check passed \n");
   }
   
   if(*ssl == NULL){
      printf("No ssl object available\n");
      WOLFSSL* sslnew;
      *ssl = sslnew;
   }else{
      printf("Ssl check passed \n");
   }
   
   int     on = 1;
   int     res = 1;
   int     sc_fd = 0;            /* Initialize our socket */
   int     len = sizeof(on);
   int     cont;
   //WOLFSSL* ssl = ;

   int     recvLen;                 /* length of string read */
   int     readWriteErr; 
   char    buff[4096]; 
   char    ack[] = "I hear you fashizzle\n";

   //WOLFSSL* ssl = NULL;              /* Initialize ssl object */
   struct sockaddr_in6 servAddr;     /* our server's address */
        
   /* Create the WOLFSSL Object */
   if (( *ssl = wolfSSL_new(*ctx)) == NULL) {
      printf("Unable to create ssl object\n");
      return -1;
   }
   
   memset((char *)&servAddr, 0, sizeof(servAddr));
   servAddr.sin6_family = AF_INET6;
   servAddr.sin6_port = htons(port);

   if (host == INADDR_ANY)
        servAddr.sin6_addr = in6addr_any;
   else {
      struct addrinfo  hints;
      struct addrinfo* answer = NULL;
      int    ret;
      char   strPort[80];

      memset(&hints, 0, sizeof(hints));

      hints.ai_family   = AF_INET6;
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_protocol = IPPROTO_UDP;

      sprintf(strPort, "%d", port);
      strPort[79] = '\0';

      ret = getaddrinfo(host, strPort, &hints, &answer);
      if (ret < 0 || answer == NULL){
         printf("Getaddrinfo failed\n");
      }

      memcpy(&servAddr, answer->ai_addr, answer->ai_addrlen);
      freeaddrinfo(answer);
   }
   
   wolfSSL_dtls_set_peer(*ssl, &servAddr, sizeof(servAddr));
   
   /* Create a UDP/IP client socket, IPv6 */
   if ((sc_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0 ) {
      printf("Cannot create socket.\n");
      return -1;
   }
   
   wolfSSL_set_fd(*ssl, sc_fd);
   printf("Socket allocated\n");

   wolfSSL_set_using_nonblock(*ssl, 1);
   
   //Set using non block
   int flags = fcntl(sc_fd, F_GETFL, 0);
   if (flags < 0){
      printf("fcntl get failed\n");
   }
   flags = fcntl(sc_fd, F_SETFL, flags | O_NONBLOCK);
   if (flags < 0){
      printf("fcntl set failed\n");
   }
   
   int ret = wolfSSL_connect(*ssl);
   
   int error = wolfSSL_get_error(*ssl, 0);
   int select_ret;
   
   while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ ||
                                  error == SSL_ERROR_WANT_WRITE)) {
      int currTimeout = 1;

      if (error == SSL_ERROR_WANT_READ){
         printf("... client would read block\n");
      }else{
         printf("... client would write block\n");
      }
      currTimeout = wolfSSL_dtls_get_current_timeout(*ssl);
      select_ret = tcp_select(sc_fd, currTimeout);
      
      if ((select_ret == TEST_RECV_READY) || (select_ret == TEST_ERROR_READY)) {
         ret = wolfSSL_connect(*ssl);
         error = wolfSSL_get_error(*ssl, 0);
      }
      else if (select_ret == TEST_TIMEOUT && !wolfSSL_dtls(*ssl)) {
         error = SSL_ERROR_WANT_READ;
      }else {
         error = SSL_FATAL_ERROR;
      }

   }
   if (ret != SSL_SUCCESS){
      printf("SSL_connect failed\n");
      return error;
   }
   
   printf("Connected to server, returning WOLFSSL* object\n");
      
   return 1;
}

int awaitConnection(WOLFSSL** ssl, WOLFSSL_CTX** ctx, int port){
   if(*ctx == NULL){
      printf("Need to init ctx first\n");
      return -1;
   }else{
      printf("Ctx check passed \n");
   }
   
   if(*ssl == NULL){
      printf("No ssl object available\n");
      WOLFSSL* sslnew;
      *ssl = sslnew;
   }else{
      printf("Ssl check passed \n");
   }
   
   int     on = 1;
   int     res = 1;
   int     sc_fd = 0;            /* Initialize our socket */
   int     len = sizeof(on);
   int     cont;

   //WOLFSSL* ssl = NULL;              /* Initialize ssl object */
   struct sockaddr_in6 clientAddr;     /* our server's address */
        
   /* Create the WOLFSSL Object */
   if (( *ssl = wolfSSL_new(*ctx)) == NULL) {
      printf("Unable to create ssl object\n");
      return -1;
   }
   
   memset((char *)&clientAddr, 0, sizeof(clientAddr));
   clientAddr.sin6_family = AF_INET6;
   clientAddr.sin6_port = htons(port);
   clientAddr.sin6_addr = in6addr_any;
   
   
   wolfSSL_dtls_set_peer(*ssl, &clientAddr, sizeof(clientAddr));
   
   /* Create a UDP/IP client socket, IPv6 */
   if ((sc_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0 ) {
      printf("Cannot create socket.\n");
      return -1;
   }
   
   //set non blocking socket
   int flags = fcntl(sc_fd, F_GETFL, 0);
   if (flags < 0){
      printf("fcntl get failed\n");
   }
   flags = fcntl(sc_fd, F_SETFL, flags | O_NONBLOCK);
   if (flags < 0){
      printf("fcntl set failed\n");
   }
   
   wolfSSL_set_fd(*ssl, sc_fd);
   printf("Socket allocated\n");

   wolfSSL_set_using_nonblock(*ssl, 1);
   
   int ret = wolfSSL_accept(*ssl);
   
   int error = wolfSSL_get_error(*ssl, 0);
    int select_ret;

    while (ret != SSL_SUCCESS && (error == SSL_ERROR_WANT_READ ||
                                  error == SSL_ERROR_WANT_WRITE)) {
        int currTimeout = 1;

        if (error == SSL_ERROR_WANT_READ)
            printf("... server would read block\n");
        else
            printf("... server would write block\n");

        currTimeout = wolfSSL_dtls_get_current_timeout(*ssl);
        select_ret = tcp_select(sc_fd, currTimeout);

        if ((select_ret == TEST_RECV_READY) ||
                                        (select_ret == TEST_ERROR_READY)) {
            ret = wolfSSL_accept(*ssl);
            
            error = wolfSSL_get_error(*ssl, 0);
        }
        else if (select_ret == TEST_TIMEOUT && !wolfSSL_dtls(*ssl)) {
            error = SSL_ERROR_WANT_READ;
        }
        else if (select_ret == TEST_TIMEOUT && wolfSSL_dtls(*ssl) &&
                                            wolfSSL_dtls_got_timeout(*ssl) >= 0) {
            error = SSL_ERROR_WANT_READ;
        }
        else {
            error = SSL_FATAL_ERROR;
        }
    }
    if (ret != SSL_SUCCESS){
      printf("SSL_accept failed\n");
      return error;
    }
    
   printf("Connection established with client, returning WOLFSSL* object\n");
   return 1;
}

int main(int argc, char** argv)
{
   /* cont short for "continue?", Loc short for "location" */    
   int         cont = 0;
   char        eccCert[] = "../certs/server-ecc.pem";
   char        ourCert[] = "../certs/client-ecc-cert.pem";
   char        ourKey[] = "../certs/ecc-client-key.pem";
   WOLFSSL_CTX* ctx = 0;
   WOLFSSL* ssl = 0;

   if(initDTLS(&ctx,"","","", 0) < 0){
      printf("Unable to initialize ctx\n");
      return -1;
   }
    
   if(connectToServer(&ssl,&ctx,"::1",5683) < 0){
      printf("Unable to initialize ssl\n");
      return -1;
   }
   
   writeDTLS(&ssl, "Hello this is a test\n");
   //readDTLS(ssl, ) inplement test callback!?
   
   closeDTLS();

   return 0;
}



/**
 * Create WOLFSSL_CTX for communicating with dtls servers
**/
int initDTLS(WOLFSSL_CTX** ctx, char* verifyCert, char* ourCert, char* ourKey, int isServer)
{
   int cont = 0;
   int err = 0;
   WOLFSSL_METHOD*  method  = 0;
   //WOLFSSL_CTX* ctx = 0;
   
   if(strlen(verifyCert) <= 1){
      if(isServer){
         verifyCert = "../certs/client-ecc-cert.pem";
      }else{
         verifyCert = "../certs/server-ecc.pem";
      }
      printf("Using standard verifyCert %s\n", verifyCert);
   }else{
      printf("Using specific verifyCert %s\n", verifyCert);
   }
   if(strlen(ourCert) <= 1){
      if(isServer){
         ourCert = "../certs/server-ecc.pem";
      }else{
         ourCert = "../certs/client-ecc-cert.pem";
      }
      printf("Using standard ourCert %s\n", ourCert);
   }else{
      printf("Using specific ourCert %s\n", ourCert);
   }
   if(strlen(ourKey) <= 1){
      if(isServer){
         ourKey = "../certs/ecc-key.pem";
      }else{
         ourKey = "../certs/ecc-client-key.pem";
      }
      printf("Using standard ourKey %s\n", ourKey);
   }else{
      printf("Using specific ourKey %s\n", ourKey);
   }

   /* cont short for "continue?", Loc short for "location" */    


   /* "./config --enable-debug" and uncomment next line for debugging */
   //wolfSSL_Debugging_ON(); 

   /* Initialize WOLFSSL */
   wolfSSL_Init();
   if(isServer){
      method = wolfDTLSv1_2_server_method();
   }else{
      method = wolfDTLSv1_2_client_method();
   }
   /* Set ctx to DTLS 1.2 */
   if ((*ctx = wolfSSL_CTX_new(method)) == NULL) {
     printf("wolfSSL_CTX_new error.\n");
     return -1;
   }

   /* Load CA certificates */
   wolfSSL_CTX_set_verify(*ctx, SSL_VERIFY_PEER |
                                SSL_VERIFY_FAIL_IF_NO_PEER_CERT, myVerify);
   if ((err = wolfSSL_CTX_load_verify_locations(*ctx,verifyCert,0)) != SSL_SUCCESS) {
     printf("Error loading %s, please check the file.\n", verifyCert);
     return err;
   }
    
    
   //if (wolfSSL_CTX_use_certificate_chain_file(*ctx, ourCert) != SSL_SUCCESS){
   if ((err = wolfSSL_CTX_use_certificate_file(*ctx, ourCert, SSL_FILETYPE_PEM)) != SSL_SUCCESS){
      printf("can't load own cert file, check file and run from wolfSSL home dir\n");
      return err;
   }

   if ((err = wolfSSL_CTX_use_PrivateKey_file(*ctx, ourKey, SSL_FILETYPE_PEM))
                                != SSL_SUCCESS){
      printf("can't load own private key file, check file and run from wolfSSL home dir\n");
      return err;
   }
   
   return 1;
}

/**
 * Do read while cleanup != 1, send received messages back to callback function
**/
void readDTLS(WOLFSSL** ssl, callbackFt fct){
   /* Begin do-while read */
   char buff[MSGLEN];
   int recvLen;
   int readWriteErr;
   printf("Starting to read\n");
   //TODO: fix error, ssl should have been given correctly to read
   /* Begin: Reply to the client */
   do {
      recvLen = wolfSSL_read(*ssl, buff, sizeof(buff)-1);
   
      //recvLen = 0;
     do {
         if (cleanup == 1) {
             memset(buff, 0, sizeof(buff));
             break;
         }
         if (recvLen < 0) {
             readWriteErr = wolfSSL_get_error(*ssl, 0);
             if (readWriteErr != SSL_ERROR_WANT_READ) {
                 printf("Read Error, error was: %d.\n", readWriteErr);
                 cleanup = 1;
             } else {
                 recvLen = wolfSSL_read(*ssl, buff, sizeof(buff)-1);
                 //printf("Read, read and read");
             }
         }
     } while (readWriteErr == SSL_ERROR_WANT_READ && 
                                         recvLen < 0 && 
                                         cleanup != 1);
  
      /* End do-while read */

      if (recvLen > 0) {
         buff[recvLen] = 0;
         printf("I heard this:\"%s\"\n", buff);
         (*fct)(buff);
      } 
      else {
         //printf("Connection Timed Out.\n");
      }

   }while(cleanup != 1);
}

/**
 * Send a message to peer specified in WOLFSSL* object
**/
void writeDTLS(WOLFSSL** ssl, char* message){
   /* Begin do-while write */
   printf("I am going to write: %s\n", message);
   int readWriteErr;
   char    ack[] = "I hear you fashizzle\n";
   do {
      //if (cleanup == 1) {
      //    memset(&buff, 0, sizeof(buff));
      //    break;
      //}
      readWriteErr = wolfSSL_get_error(*ssl, 0);
      if (wolfSSL_write(*ssl, message, strlen(message)) < 0) {
          printf("Write error.\n");
          cleanup = 1;
          return;
      }
      printf("Reply sent:\"%s\" with length: %i \n", message, sizeof(message));
   }while(readWriteErr == SSL_ERROR_WANT_WRITE && cleanup != 1);
   /* End do-while write */

   /* free allocated memory */
   //memset(buff, 0, sizeof(buff));
   //wolfSSL_free(ssl);

   /* End: Reply to the Client */
}

void closeDTLS(){
   cleanup = 1;
}




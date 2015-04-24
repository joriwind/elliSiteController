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

WOLFSSL_CTX* initDTLS(char* eccCert, char* ourCert, char* ourKey);
WOLFSSL* connectToServer(WOLFSSL_CTX* ctx, char* host, int port);    /* Separate out Handling Datagrams */
void readDTLS(WOLFSSL* ssl, callbackFt fct);
void writeDTLS(WOLFSSL* ssl, char* message);
void closeDTLS();


/**
 * Use defined WOLFSSL_CTX to connect to host on port port
 * @return WOLFSSL* object, connected with host on port port
**/
WOLFSSL* connectToServer(WOLFSSL_CTX* ctx, char* host, int port)
{
   int     on = 1;
   int     res = 1;
   int     sc_fd = 0;            /* Initialize our socket */
   int     len = sizeof(on);
   int     cont;
   WOLFSSL* ssl;

   int     recvLen;                 /* length of string read */
   int     readWriteErr; 
   char    buff[4096]; 
   char    ack[] = "I hear you fashizzle\n";

   //WOLFSSL* ssl = NULL;              /* Initialize ssl object */
   struct sockaddr_in6 servAddr;     /* our server's address */
    
   /* Create the WOLFSSL Object */
   if (( ssl = wolfSSL_new(ctx)) == NULL) {
      printf("Unable to create ssl object\n");
      return ssl;
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

      SNPRINTF(strPort, sizeof(strPort), "%d", port);
      strPort[79] = '\0';

      ret = getaddrinfo(host, strPort, &hints, &answer);
      if (ret < 0 || answer == NULL)
       err_sys("getaddrinfo failed");

      memcpy(&servAddr, answer->ai_addr, answer->ai_addrlen);
      freeaddrinfo(answer);
   }
   
   wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));
   
   /* Create a UDP/IP client socket, IPv6 */
   if ((sc_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0 ) {
      printf("Cannot create socket.\n");
      return ssl;
   }
   
   wolfSSL_set_fd(ssl, sc_fd);
   printf("Socket allocated\n");

   wolfSSL_set_using_nonblock(ssl, 1);
   
   //Set using non block
   int flags = fcntl(sc_fd, F_GETFL, 0);
   if (flags < 0)
      err_sys("fcntl get failed");
   flags = fcntl(sc_fd, F_SETFL, flags | O_NONBLOCK);
   if (flags < 0)
      err_sys("fcntl set failed");
   
   NonBlockingSSL_Connect(ssl);
   printf("Connected to server, returning WOLFSSL* object");
      
   return ssl;
}


int main(int argc, char** argv)
{
   /* cont short for "continue?", Loc short for "location" */    
   int         cont = 0;
   char        eccCert[] = "../certs/server-ecc.pem";
   char        ourCert[] = "../certs/client-cert.pem";
   char        ourKey[] = "../certs/client-key.pem";
   WOLFSSL_CTX* ctx;
   WOLFSSL* ssl = NULL;

   ctx = initDTLS("","","");
    
   ssl = connectToServer(ctx,"::1",5683);

   writeDTLS(ssl, "Hello this is a test");
   //readDTLS(ssl, ) inplement test callback!?
   
   closeDTLS();

   return 0;
}

/**
 * Create WOLFSSL_CTX for communicating with dtls servers
**/
WOLFSSL_CTX* initDTLS(char* eccCert, char* ourCert, char* ourKey)
{
   int cont = 0;
   WOLFSSL_METHOD*  method  = 0;
   WOLFSSL_CTX* ctx = 0;
   
   if(strlen(eccCert) <= 1){
      eccCert = "../certs/server-ecc.pem";
   }
   if(strlen(ourCert) <= 1){
      ourCert = "../certs/client-cert.pem";
   }
   if(strlen(ourKey) <= 1){
      ourKey = "../certs/client-key.pem";
   }

   /* cont short for "continue?", Loc short for "location" */    


   /* "./config --enable-debug" and uncomment next line for debugging */
   wolfSSL_Debugging_ON(); 

   /* Initialize WOLFSSL */
   wolfSSL_Init();
   method = wolfDTLSv1_2_client_method();
   /* Set ctx to DTLS 1.2 */
   if ((ctx = wolfSSL_CTX_new(method)) == NULL) {
     printf("wolfSSL_CTX_new error.\n");
     return ctx;
   }

   /* Load CA certificates */
   if (wolfSSL_CTX_load_verify_locations(ctx,eccCert,0) != SSL_SUCCESS) {
     printf("Error loading %s, please check the file.\n", eccCert);
     return ctx;
   }
    
    
   if (wolfSSL_CTX_use_certificate_chain_file(ctx, ourCert) != SSL_SUCCESS){
      err_sys("can't load client cert file, check file and run from wolfSSL home dir");
   }

   if (wolfSSL_CTX_use_PrivateKey_file(ctx, ourKey, SSL_FILETYPE_PEM)
                                != SSL_SUCCESS){
      err_sys("can't load client private key file, check file and run from wolfSSL home dir");
   }
   
   return ctx;
}

/**
 * Do read while cleanup != 1, send received messages back to callback function
**/
void readDTLS(WOLFSSL* ssl, callbackFt fct){
   /* Begin do-while read */
   char buff[MSGLEN];
   int recvLen;
   int readWriteErr;
   //TODO: fix error, ssl should have been given correctly to read
   /* Begin: Reply to the client */
   do {
      recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1);
   
      //recvLen = 0;
     do {
         if (cleanup == 1) {
             memset(buff, 0, sizeof(buff));
             break;
         }
         if (recvLen < 0) {
             readWriteErr = wolfSSL_get_error(ssl, 0);
             if (readWriteErr != SSL_ERROR_WANT_READ) {
                 printf("Read Error, error was: %d.\n", readWriteErr);
                 cleanup = 1;
             } else {
                 recvLen = wolfSSL_read(ssl, buff, sizeof(buff)-1);
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
void writeDTLS(WOLFSSL* ssl, char* message){
   /* Begin do-while write */
   int readWriteErr;
   char    ack[] = "I hear you fashizzle\n";
   do {
      //if (cleanup == 1) {
      //    memset(&buff, 0, sizeof(buff));
      //    break;
      //}
      readWriteErr = wolfSSL_get_error(ssl, 0);
      if (wolfSSL_write(ssl, message, strlen(message)) < 0) {
          printf("Write error.\n");
          cleanup = 1;
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




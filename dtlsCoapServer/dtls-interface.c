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
 * Bare-bones example of a DTLS server for instructional/learning purposes.
 * Utilizes DTLS 1.2.
 */

#include <stdio.h>                  /* standard in/out procedures */
#include <stdlib.h>                 /* defines system calls */
#include <string.h>                 /* necessary for memset */
#include <netdb.h>
#include <sys/socket.h>             /* used for all socket calls */
#include <netinet/in.h>             /* used for sockaddr_in */
#include <arpa/inet.h>
#include <cyassl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096

static int cleanup;                 /* To handle shutdown */
struct sockaddr_in servAddr;        /* our server's address */
struct sockaddr_in cliaddr;         /* the client's address */

void dtls_set_nonblocking(int*);    /* set the socket non-blocking */
int NonBlockingSSL_Accept(CYASSL*); /* non-blocking accept */
CYASSL* AwaitDGram(CYASSL_CTX* ctx);   /* Separate out Handling Datagrams */
int listenDTLS(CYASSL* ssl, char buffer[MSGLEN]);
int writeDTLS(CYASSL* ssl, char message[]);
CYASSL_CTX* init();
void closeDTLS(CYASSL* ssl);
int dtls_select();
void CleanUp();

/* costumes for select_ret to wear */
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};


CYASSL* AwaitDGram(CYASSL_CTX* ctx)
{
    int           on = 1;
    int           res = 1; 
    int           connfd = 0;  
    int           listenfd = 0;   /* Initialize our socket */
    CYASSL*       ssl;
    socklen_t     cliLen;
    socklen_t     len = sizeof(on);
    unsigned char b[MSGLEN];      /* watch for incoming messages */

    //while (cleanup != 1) {
        /* Create a UDP/IP socket */
        if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            printf("Cannot create socket.\n");
            cleanup = 1;
        }
        
        dtls_set_nonblocking(&listenfd);
        printf("Socket all/ocated\n");

        /* clear servAddr each loop */
        memset((char *)&servAddr, 0, sizeof(servAddr));

        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servAddr.sin_family      = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port        = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            cleanup = 1;
            return ssl;
        }

        /*Bind Socket*/
        if (bind(listenfd, 
                    (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
            printf("Bind failed.\n");
            cleanup = 1;
            return ssl;
        }

        printf("Awaiting client connection on port %d\n", SERV_PORT);
         
         //TODO: use non blocking wait?
        cliLen = sizeof(cliaddr);   
        connfd = (int)recvfrom(listenfd, (char *)&b, sizeof(b), MSG_PEEK,
                (struct sockaddr*)&cliaddr, &cliLen);
         
        if (connfd < 0) {
            printf("No clients in que, enter idle state\n");
            //continue;
            return ssl;
        }
        else if (connfd > 0) {
            if (connect(listenfd, (const struct sockaddr *)&cliaddr, 
                        sizeof(cliaddr)) != 0) {
                printf("Udp connect failed.\n");
                cleanup = 1;
                return ssl;
            }
        }
        else {
            printf("Recvfrom failed.\n");
            cleanup = 1;
            return ssl;
        }
        printf("Connected!\n");

        /* Create the CYASSL Object */
        if ((ssl = CyaSSL_new(ctx)) == NULL) {
            printf("CyaSSL_new error.\n");
            cleanup = 1;
            return ssl;
        }
        
        /* set the session ssl to client connection port */
        CyaSSL_set_fd(ssl, listenfd);
        CyaSSL_set_using_nonblock(ssl, 1);
        
        int cont = NonBlockingSSL_Accept(ssl);

        if (cont != 0) {
            printf("NonBlockingSSL_Accept failed.\n");
            return ssl;
        }
        
        
        return ssl;
    //}
}

int NonBlockingSSL_Accept(CYASSL* ssl)
{
    int select_ret;
    int currTimeout = 1;
    int ret = CyaSSL_accept(ssl);
    int error = CyaSSL_get_error(ssl, 0);
    int listenfd = (int)CyaSSL_get_fd(ssl);

    while (cleanup != 1 && (ret != SSL_SUCCESS && 
                (error == SSL_ERROR_WANT_READ ||
                 error == SSL_ERROR_WANT_WRITE))) {
        if (cleanup == 1) {
            CyaSSL_free(ssl);
            CyaSSL_shutdown(ssl);
            break;
        }

        if (error == SSL_ERROR_WANT_READ)
            printf("... server would read block\n");
        else
            printf("... server would write block\n");

        currTimeout = CyaSSL_dtls_get_current_timeout(ssl);
        select_ret = dtls_select(listenfd, currTimeout);

        if ((select_ret == TEST_RECV_READY) ||
                (select_ret == TEST_ERROR_READY)) {
            ret = CyaSSL_accept(ssl);
            error = CyaSSL_get_error(ssl, 0);
        }
        else if (select_ret == TEST_TIMEOUT && !CyaSSL_dtls(ssl)) {
            error = SSL_ERROR_WANT_READ;
        }
        else if (select_ret == TEST_TIMEOUT && CyaSSL_dtls(ssl) &&
                CyaSSL_dtls_got_timeout(ssl) >= 0) {
            error = SSL_ERROR_WANT_READ;
        }
        else {
            error = SSL_FATAL_ERROR;
        }
    }
    if (ret != SSL_SUCCESS) {
        printf("SSL_accept failed.\n");
        return 1;
    }

    return 0;
}

int udp_read_connect(int listenfd)
{
    int bytesRecvd;
    unsigned char  b[MSGLEN];
    struct sockaddr_in cliAddr;
    socklen_t clilen = sizeof(cliAddr);

    do {
        bytesRecvd = (int)recvfrom(listenfd, (char*)b, sizeof(b), MSG_PEEK,
            (struct sockaddr*)&cliAddr, &clilen);
    } while (bytesRecvd <= 0);

    if (bytesRecvd > 0) {
        if (connect(listenfd, (const struct sockaddr*)&cliAddr, 
                    sizeof(cliAddr)) != 0) {
            printf("udp connect failed.\n");
        }
    }
    else {
        printf("recvfrom failed.\n");
    }

    printf("Connected!\n");
    /* ensure b is empty upon each call */
    memset(&b, 0, sizeof(b));
    return listenfd;
}

void dtls_set_nonblocking(int* sockfd)
{
    int flags = fcntl(*sockfd, F_GETFL, 0);
    if (flags < 0) {
        printf("fcntl get failed");
        cleanup = 1;
    }            
    flags = fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK);            
    if (flags < 0) {
        printf("fcntl set failed.\n");
        cleanup = 1;
    }
}

int dtls_select(int socketfd, int toSec)
{
    int result;
    int nfds = socketfd + 1;
    fd_set  recvfds, errfds;
    struct timeval timeout = { (toSec > 0) ? toSec : 0, 0};

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

//, int(*giveData) (char buffer[MSGLEN])
int listenDTLS(CYASSL* ssl, char* buffer){
   /* Begin: Reply to the client */
   int recvLen = CyaSSL_read(ssl, buffer, sizeof(buffer)-1);
   int readWriteErr;
   /* Begin do-while read */
   do {
      if (cleanup == 1) {
          memset(buffer, 0, sizeof(buffer));
          break;
      }
      if (recvLen < 0) {
          readWriteErr = CyaSSL_get_error(ssl, 0);
          if (readWriteErr != SSL_ERROR_WANT_READ) {
              printf("Read Error, error was: %d.\n", readWriteErr);
              cleanup = 1;
          } else {
              recvLen = CyaSSL_read(ssl, buffer, sizeof(buffer)-1);
              if (recvLen > 0) {
                  buffer[recvLen] = 0;
                  printf("I heard this:\"%s\"\n", buffer);
               } 
               else {
                  printf("Connection Timed Out.\n");
               }
          }
      }
      
   } while (cleanup != 1);
   /* End do-while read */
   if (cleanup == 1) {
       memset(&buffer, 0, sizeof(buffer));
       
   }
   
}

int writeDTLS(CYASSL* ssl, char message[MSGLEN]){
   //char  ack[] = "I hear you fashizzle!\n";
   /* Begin do-while write */
   int readWriteErr;
   do {
      
      readWriteErr = CyaSSL_get_error(ssl, 0);
      if (CyaSSL_write(ssl, message, sizeof(message)) < 0) {
          printf("Write error.\n");
          cleanup = 1;
      }
      printf("Reply sent:\"%s\"\n", message);
   }while(cleanup != 1);
   /* End do-while write */
}

void closeDTLS(CYASSL* ssl){
   CyaSSL_set_fd(ssl, 0); 
   CyaSSL_shutdown(ssl);        
   CyaSSL_free(ssl);

   printf("Client left return to idle state\n");
}

CYASSL_CTX* init()
{
    /* cont short for "continue?", Loc short for "location" */    
    //int         cont = 0;
    char        caCertLoc[] = "../certs/ca-cert.pem";
    char        servCertLoc[] = "../certs/server-cert.pem";
    char        servKeyLoc[] = "../certs/server-key.pem";
    CYASSL_CTX* ctx;
    
    /* "./config --enable-debug" and uncomment next line for debugging */
    /*CyaSSL_Debugging_ON();*/

    /* Initialize CyaSSL */
    CyaSSL_Init();

    /* Set ctx to DTLS 1.2 */
    if ((ctx = CyaSSL_CTX_new(CyaDTLSv1_2_server_method())) == NULL) {
        printf("CyaSSL_CTX_new error.\n");
        return ctx;
    }
    /* Load CA certificates */
    if (CyaSSL_CTX_load_verify_locations(ctx,caCertLoc,0) != 
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return ctx;
    }
    /* Load server certificates */
    if (CyaSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != 
                                                                 SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return ctx;
    }
    /* Load server Keys */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return ctx;
    }

    return ctx;
}

int main(int argc, char** argv){
   
   
   char  ack[] = "I hear you fashizzle!\n";
   int cont = 1;
   CYASSL* ssl;
   CYASSL_CTX* ctx;
   char buff[MSGLEN];
   
   if((ctx = init(argc, argv))==NULL){
      printf("Error in Init");
      return 0;
   }
   
   if((ssl = AwaitDGram(ctx))==NULL){
         printf("Error in AwaitDGram");
         return 0;
      }
   while(cleanup != 1){
      
      
      
      listenDTLS(ssl, buff);
      if(writeDTLS(ssl,ack)==1){
         printf("Error in write");
         return 0;
      }
   }
   
   closeDTLS(ssl);
   
    if (cont == 1) {
        CyaSSL_CTX_free(ctx);
        CyaSSL_Cleanup();
    }
   return 0;
}


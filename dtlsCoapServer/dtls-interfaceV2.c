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
#include <arpa/inet.h>
#include <cyassl/ssl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096

static int cleanup;                 /* To handle shutdown */

/* Callback function*/
typedef int (*callbackFt)(char* message);

void dtls_set_nonblocking(int*);    /* set the socket non-blocking */
int NonBlockingSSL_Accept(CYASSL*); /* non-blocking accept */
CYASSL* AwaitDGram(CYASSL_CTX* ctx, CYASSL* ssl);    /* Separate out Handling Datagrams */
int udp_read_connect(int);          /* broken out to improve readability */
int dtls_select();
void writeDTLS(CYASSL* ssl, char* message);
CYASSL* initDTLS();
void readDTLS(CYASSL* ssl, callbackFt fct);

/* costumes for select_ret to wear */
enum {
    TEST_SELECT_FAIL,
    TEST_TIMEOUT,
    TEST_RECV_READY,
    TEST_ERROR_READY
};


CYASSL* AwaitDGram(CYASSL_CTX* ctx, CYASSL* ssl)
{
    int     on = 1;
    int     res = 1;
    int     listenfd = 0;            /* Initialize our socket */
    int     clientfd = 0;            /* client connection */
    int     len = sizeof(on);
    int     cont;
    ssl = NULL;
    
    
    int     recvLen;                 /* length of string read */
    int     readWriteErr; 
    char    buff[4096]; 
    char    ack[] = "I hear you fashizzle\n";
    
    //CYASSL* ssl = NULL;              /* Initialize ssl object */
    struct sockaddr_in servAddr;     /* our server's address */
    
    //while (cleanup != 1) {

        /* Create a UDP/IP socket */
        if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            printf("Cannot create socket.\n");
            return ssl;
        }
        
        printf("Socket allocated\n");
        
        dtls_set_nonblocking(&listenfd);

        memset((char *)&servAddr, 0, sizeof(servAddr));

        /* host-to-network-long conversion (htonl) */
        /* host-to-network-short conversion (htons) */
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port = htons(SERV_PORT);

        /* Eliminate socket already in use error */
        res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, len);
        if (res < 0) {
            printf("Setsockopt SO_REUSEADDR failed.\n");
            return ssl;
        }

        /*Bind Socket*/
        if (bind(listenfd, 
                    (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
            printf("Bind failed.\n");
            return ssl;
        }

        printf("Awaiting client connection on port %d\n", SERV_PORT);
        
        
        clientfd = udp_read_connect(listenfd);
     

//        dtls_set_nonblocking(&clientfd);

        /* Create the CYASSL Object */
        if (( ssl = CyaSSL_new(ctx)) == NULL) {
            printf("CyaSSL_new error.\n");
            return ssl;
        }

        /* set clilen to |cliAddr| */
        printf("Connected!\n");

        /* set the/ session ssl to client connection port */
        CyaSSL_set_fd(ssl, clientfd);
        
        //SOCKET is up and running
         //return ssl;
        CyaSSL_set_using_nonblock(ssl, 1);
        cont = NonBlockingSSL_Accept(ssl);

        if (cont != 0) {
            printf("NonBlockingSSL_Accept failed.\n");
            return ssl;
        }
         //return ssl;
        /* Begin: Reply to the client */
        //recvLen = CyaSSL_read(ssl, buff, sizeof(buff)-1);
        /* Begin: Reply to the client */
        recvLen = CyaSSL_read(ssl, buff, sizeof(buff)-1);
        
        int error = 0;
         socklen_t lengt = sizeof (error);
         int retval = getsockopt (clientfd, SOL_SOCKET, SO_ERROR, &error, &lengt );
         printf("SOCKET ERROR %i\n ", retval);
        printf("DTLS: %i & %i\n", CyaSSL_dtls(ssl), CyaSSL_get_using_nonblock(ssl));
        
        
        //return ssl;
        /* Begin do-while read *//**
        do {
           //printf("Loop 1\n");
            if (cleanup == 1) {
                memset(buff, 0, sizeof(buff));
                break;
            }
            if (recvLen < 0) {
                readWriteErr = CyaSSL_get_error(ssl, 0);
                if (readWriteErr != SSL_ERROR_WANT_READ) {
                    printf("Read Error, error was: %d.\n", readWriteErr);
                    cleanup = 1;
                } else {
                    recvLen = CyaSSL_read(ssl, buff, sizeof(buff)-1);
                }
            }
        } while (readWriteErr == SSL_ERROR_WANT_READ && 
                                         recvLen < 0 && 
                                         cleanup != 1);
        /* End do-while read */
         /**
        if (recvLen > 0) {
            buff[recvLen] = 0;
            printf("I heard this:\"%s\"\n", buff);
        } 
        else {
            printf("Connection Timed Out.\n");
        }
        
        /* Begin do-while write *//**
        do {
           //printf("Loop 2\n");
            if (cleanup == 1) {
                memset(&buff, 0, sizeof(buff));
                break;
            }
            readWriteErr = CyaSSL_get_error(ssl, 0);
            if (CyaSSL_write(ssl, ack, sizeof(ack)) < 0) {
                printf("Write error.\n");
                cleanup = 1;
            }
            printf("Reply sent:\"%s\"\n", ack);
        }while(readWriteErr == SSL_ERROR_WANT_WRITE && cleanup != 1);
        /* End do-while write */

        /* free allocated memory */
        //memset(buff, 0, sizeof(buff));
        //CyaSSL_free(ssl);**/
    //}
    return ssl;
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

int NonBlockingSSL_Accept(CYASSL* ssl)
{
    int select_ret;
    int currTimeout = 1;
    int ret = CyaSSL_accept(ssl);
    int error = CyaSSL_get_error(ssl, 0);
    int listenfd = (int)CyaSSL_get_fd(ssl);
    printf("ret & error: %i  &  %i cleanup: %i\n", ret, error, cleanup);
    while (cleanup != 1 && (ret != SSL_SUCCESS && 
                (error == SSL_ERROR_WANT_READ ||
                 error == SSL_ERROR_WANT_WRITE))) {
    printf("ret & error: %i  &  %i cleanup: %i\n", ret, error, cleanup);
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

int main(int argc, char** argv)
{
    /* cont short for "continue?", Loc short for "location" */    
    int         cont = 0;
    char        caCertLoc[] = "../certs/ca-cert.pem";
    char        servCertLoc[] = "../certs/server-cert.pem";
    char        servKeyLoc[] = "../certs/server-key.pem";
    CYASSL_CTX* ctx;
    CYASSL* ssl = NULL;

    /* "./config --enable-debug" and uncomment next line for debugging */
    /* CyaSSL_Debugging_ON(); */

    /* Initialize CyaSSL */
    CyaSSL_Init();

    /* Set ctx to DTLS 1.2 */
    if ((ctx = CyaSSL_CTX_new(CyaDTLSv1_2_server_method())) == NULL) {
        printf("CyaSSL_CTX_new error.\n");
        return 1;
    }
    /* Load CA certificates */
    if (CyaSSL_CTX_load_verify_locations(ctx,caCertLoc,0) != 
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return 1;
    }
    /* Load server certificates */
    if (CyaSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != 
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return 1;
    }
    /* Load server Keys */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return 1;
    }

    //cont = AwaitDGram(ctx, ssl);
    ssl = AwaitDGram(ctx, ssl);

    //if (cont == 1) {
    //    CyaSSL_CTX_free(ctx);
    //    CyaSSL_Cleanup();
    //}

    return 0;
}

CYASSL* initDTLS()
{
    /* cont short for "continue?", Loc short for "location" */    
    int         cont = 0;
    char        caCertLoc[] = "../certs/ca-cert.pem";
    char        servCertLoc[] = "../certs/server-cert.pem";
    char        servKeyLoc[] = "../certs/server-key.pem";
    CYASSL_CTX* ctx;
    CYASSL* ssl = NULL;

    /* "./config --enable-debug" and uncomment next line for debugging */
     /*CyaSSL_Debugging_ON();*/

    /* Initialize CyaSSL */
    CyaSSL_Init();

    /* Set ctx to DTLS 1.2 */
    if ((ctx = CyaSSL_CTX_new(CyaDTLSv1_2_server_method())) == NULL) {
        printf("CyaSSL_CTX_new error.\n");
        return ssl;
    }
    /* Load CA certificates */
    if (CyaSSL_CTX_load_verify_locations(ctx,caCertLoc,0) != 
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", caCertLoc);
        return ssl;
    }
    /* Load server certificates */
    if (CyaSSL_CTX_use_certificate_file(ctx, servCertLoc, SSL_FILETYPE_PEM) != 
            SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servCertLoc);
        return ssl;
    }
    /* Load server Keys */
    if (CyaSSL_CTX_use_PrivateKey_file(ctx, servKeyLoc, 
                SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        printf("Error loading %s, please check the file.\n", servKeyLoc);
        return ssl;
    }
    


    return AwaitDGram(ctx, ssl);
}

void readDTLS(CYASSL* ssl, callbackFt fct){
   /* Begin do-while read */
   char buff[MSGLEN];
   int recvLen;
   int readWriteErr;
   //TODO: fix error, ssl should have been given correctly to read
   /* Begin: Reply to the client */
   do {
      recvLen = CyaSSL_read(ssl, buff, sizeof(buff)-1);
   
      //recvLen = 0;
     do {
         if (cleanup == 1) {
             memset(buff, 0, sizeof(buff));
             break;
         }
         if (recvLen < 0) {
             readWriteErr = CyaSSL_get_error(ssl, 0);
             if (readWriteErr != SSL_ERROR_WANT_READ) {
                 printf("Read Error, error was: %d.\n", readWriteErr);
                 cleanup = 1;
             } else {
                 recvLen = CyaSSL_read(ssl, buff, sizeof(buff)-1);
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


void writeDTLS(CYASSL* ssl, char* message){
   /* Begin do-while write */
   int readWriteErr;
   char    ack[] = "I hear you fashizzle\n";
   do {
      //if (cleanup == 1) {
      //    memset(&buff, 0, sizeof(buff));
      //    break;
      //}
      readWriteErr = CyaSSL_get_error(ssl, 0);
      if (CyaSSL_write(ssl, message, strlen(message)) < 0) {
          printf("Write error.\n");
          cleanup = 1;
      }
      printf("Reply sent:\"%s\" with length: %i \n", message, sizeof(message));
   }while(readWriteErr == SSL_ERROR_WANT_WRITE && cleanup != 1);
   /* End do-while write */

   /* free allocated memory */
   //memset(buff, 0, sizeof(buff));
   //CyaSSL_free(ssl);

   /* End: Reply to the Client */
}




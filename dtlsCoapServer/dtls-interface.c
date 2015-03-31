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

#define SERV_PORT   11111           /* define our server port number */
#define MSGLEN      4096

static int cleanup;                 /* To handle shutdown */
struct sockaddr_in servAddr;        /* our server's address */
struct sockaddr_in cliaddr;         /* the client's address */

CYASSL* AwaitDGram(CYASSL_CTX* ctx);   /* Separate out Handling Datagrams */
int listenDTLS(CYASSL* ssl, char buffer[MSGLEN]);
int writeDTLS(CYASSL* ssl, char message[]);
CYASSL_CTX* init();
void closeDTLS(CYASSL* ssl);
void CleanUp();



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
        
        if (CyaSSL_accept(ssl) != SSL_SUCCESS) {

            int e = CyaSSL_get_error(ssl, 0);

            printf("error = %d, %s\n", e, CyaSSL_ERR_reason_error_string(e));
            printf("SSL_accept failed.\n");
            //continue;
            return ssl;
        }
        return ssl;
    //}
}

//, int(*giveData) (char buffer[MSGLEN])
int listenDTLS(CYASSL* ssl, char* buffer){
   int           recvLen = 0;    /* length of message */
   char          buff[MSGLEN];   /* the incoming message */
   if ((recvLen = CyaSSL_read(ssl, buff, sizeof(buff)-1)) > 0) {
      printf("heard %d bytes\n", recvLen);

      buff[recvLen] = 0;
      printf("I heard this: \"%s\"\n", buff);
      //giveData(buff);
      //buffer = &"";
      strcat(buffer, buff);
      //buffer = buff;
      return 0;
   }
   else if (recvLen < 0) {
      int readErr = CyaSSL_get_error(ssl, 0);
      if(readErr != SSL_ERROR_WANT_READ) {
          printf("SSL_read failed.\n");
          cleanup = 1;
          return 1;
      }
   }
   return 1;
        
}

int writeDTLS(CYASSL* ssl, char message[MSGLEN]){
   //char  ack[] = "I hear you fashizzle!\n";
   if (CyaSSL_write(ssl, message, MSGLEN) < 0) {
      printf("CyaSSL_write fail.\n");
      cleanup = 1;
      return 1;
   } 
   else {
      printf("Sending reply.\n");
   }

   printf("reply sent \"%s\"\n", message);
   return 0;
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
    CyaSSL_Debugging_ON();

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


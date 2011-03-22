/*
 *  tcpserver.c
 *  StreamingServer
 *
 *  Created by Bjorn Runaker on 2011-03-18.
 *  Copyright 2011 Runåker Produktkonsult AB. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>             /* close() */
#include <sys/types.h>
#include <sys/socket.h>         /* socket() */
#include <netinet/in.h>         /* IPPROTO_TCP */
#include <arpa/inet.h>          /* inet_addr() */
#include <errno.h>              /* errno */
#include <pthread.h>
#include <string.h>
#include "tcpserver.h"

#define MAXPENDING 5    /* Max connection requests */

void Die(char *mess) { perror(mess); exit(1); }

int create_tcpserver(int port, void* (*client_handler)(void* sock))
{
	int serversock, clientsock;
	struct sockaddr_in tcpserver, tcpclient;
	
	printf("Starting TCP server\n");
	
	/* Create the TCP socket */
	if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		Die("Failed to create socket");
	}
	/* Construct the server sockaddr_in structure */
	memset(&tcpserver, 0, sizeof(tcpserver));       /* Clear struct */
	tcpserver.sin_family = AF_INET;                  /* Internet/IP */
	tcpserver.sin_addr.s_addr = htonl(INADDR_ANY);   /* Incoming addr */
	tcpserver.sin_port = htons(port);       /* server port */
	
	/* Bind the server socket */
	if (bind(serversock, (struct sockaddr *) &tcpserver,
			 sizeof(tcpserver)) < 0) {
		Die("Failed to bind the server socket");
	}
	/* Listen on the server socket */
	if (listen(serversock, MAXPENDING) < 0) {
		Die("Failed to listen on server socket");
	}
	while (1) {
		unsigned int clientlen = sizeof(tcpclient);
		/* Wait for client connection */
		if ((clientsock =
			 accept(serversock, (struct sockaddr *) &tcpclient,
					&clientlen)) < 0) {
				 Die("Failed to accept client connection");
			 }
		fprintf(stdout, "Client connected: %s\n",
				inet_ntoa(tcpclient.sin_addr));
		pthread_t send_id;
		pthread_create(&send_id, NULL, client_handler, (void*) clientsock);

	}
	return 0;
}

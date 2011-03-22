/*
 *  tcpserver.h
 *  StreamingServer
 *
 *  Created by Bjorn Runaker on 2011-03-18.
 *  Copyright 2011 Runåker Produktkonsult AB. All rights reserved.
 *
 */


int create_tcpserver(int port, void* (*client_handler)(void* sock));


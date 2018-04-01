/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Sergio R. Caprile - "commonalization" from prior samples and/or documentation extension
 *******************************************************************************/



/**
This simple low-level implementation assumes a single connection for a single thread. Thus, a static
variable is used for that connection.
On other scenarios, the user must solve this by taking into account that the current implementation of
MQTTPacket_read() has a function pointer for a function call to get the data to a buffer, but no provisions
to know the caller or other indicator (the socket id): int (*getfn)(unsigned char*, int)
*/

#include "sim800c.h"
#include "delay.h"

int transport_sendPacketBuffer(int sock, unsigned char* buf, int buflen)
{
	return sim800c_write_tcp_data((char*)buf, buflen);
}


int transport_getdata(unsigned char* buf, int count)
{
	return sim800c_transport_getdata(buf, count);
}

int transport_getdatanb(void *sck, unsigned char* buf, int count)
{
return 0;
}

/**
return >=0 for a socket descriptor, <0 for an error code
@todo Basically moved from the sample without changes, should accomodate same usage for 'sock' for clarity,
removing indirections
*/
int transport_open(char* addr, int port)
{
	init_sim800c();
	if (!sim800c_connect_server(addr, port)) {
		return -1;
	}
	return 0;
}

int transport_close(int sock)
{
	sim800c_close_tcp();
	return 1;
}

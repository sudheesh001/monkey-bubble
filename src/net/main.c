/* main.c - 
 * Copyright (C) 2002 Christophe Segui
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#include "simple-server.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <net/mb-net-server.h>
void handler( int signo )
{

    printf(" signo = %d. \n", signo );

  //  signal( SIGPIPE, handler );
//    signal( SIGSEGV, handler );
}

  
int main(int argc, char **argv) {
	 MbNetServer * server;

    g_type_init();
    g_thread_init(NULL);
	 
	server = MB_NET_SERVER(g_object_new(MB_NET_TYPE_SERVER, NULL));
	
	GError * error = NULL;
	mb_net_server_accept_on(server, "mb://localhost:6666", &error);

	if( error == NULL ) {
		mb_net_server_join(server);
	}
    return(0);
}

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* monkey-server.c - 
 * Copyright (C) 2004 Laurent Belmonte <lolo3d@tuxfamily.org>
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include <gtk/gtk.h>

#include "monkey-message-handler.h"

#include <glade/glade.h>
#include "network-game-launcher.h"

struct NetworkGameLauncherPrivate {
        GladeXML * glade_xml;
        GtkWidget * window;
        gchar * server_name;
        MonkeyMessageHandler * handler;
        GtkLabel * connection_label;
};

#define PRIVATE( NetworkGameLauncher ) (NetworkGameLauncher->private)

static GObjectClass* parent_class = NULL;

void network_game_launcher_finalize(GObject *);

static void connect_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget);

static gboolean connect_server(NetworkGameLauncher * launcher);

NetworkGameLauncher *network_game_launcher_new() {
        NetworkGameLauncher * ngl;
        GtkWidget * item;

        ngl = 
                NETWORK_GAME_LAUNCHER(g_object_new(TYPE_NETWORK_GAME_LAUNCHER
                                                    , NULL));

        PRIVATE(ngl)->server_name = NULL;

        PRIVATE(ngl)->glade_xml = glade_xml_new("/home/lolo/cvs/monkey-bubble/data/netgame.glade","network_window",NULL);
        
        PRIVATE(ngl)->window = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "network_window");

       
        item = glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "go_button");
        g_signal_connect_swapped( item,"clicked",GTK_SIGNAL_FUNC(connect_server_signal),ngl);

        PRIVATE(ngl)->connection_label = GTK_LABEL(glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "connection_state_label"));

       gtk_widget_set_sensitive( glade_xml_get_widget( PRIVATE(ngl)->glade_xml
                                                       , "connected_game_hbox"),
                                 FALSE);

        return ngl;
        
}


static void connect_server_signal(gpointer    callback_data,
                           guint       callback_action,
                           GtkWidget  *widget) {
        NetworkGameLauncher  * ngl;
        GtkWidget * entry;

        ngl = NETWORK_GAME_LAUNCHER(callback_data);

        entry =  glade_xml_get_widget( PRIVATE(ngl)->glade_xml, "server_name_entry");
        if( PRIVATE(ngl)->server_name != NULL) {
                g_free(PRIVATE(ngl)->server_name);
        }

        PRIVATE(ngl)->server_name = 
                g_strdup( gtk_entry_get_text(GTK_ENTRY(entry)));

        gtk_label_set_label(PRIVATE(ngl)->connection_label,
                            g_strdup_printf("Connecting %s ...",
                                             PRIVATE(ngl)->server_name));
        
        
        gtk_widget_set_sensitive( glade_xml_get_widget( PRIVATE(ngl)->glade_xml
                                                        , "connect_hbox"),
                                  FALSE);


        g_idle_add((GSourceFunc ) connect_server,
                   ngl);
}


void recv_network_init_message(MonkeyMessageHandler * mmh,
		  guint32 client_id,
		  gchar * message,
		  gpointer * p) {

        NetworkGameLauncher * ngl;
        gchar * m;

        ngl = NETWORK_GAME_LAUNCHER(p);
        
        gtk_label_set_label(PRIVATE(ngl)->connection_label,
                            g_strdup_printf("Connected to %s",
                                            PRIVATE(ngl)->server_name));
        

        g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"message recieved client_id %d, message %s\n",client_id,message);

        m = g_strdup_printf("INIT_OK\nname=%s",g_get_user_name());
        monkey_message_handler_send_message(PRIVATE(ngl)->handler,
                                            client_id,
                                            m,
                                            strlen(m));
                                            
                                            
}

static gboolean connect_server(NetworkGameLauncher * launcher) {
        
        int sock;
        struct sockaddr_in sock_client;
        struct hostent *src_host;
        
        g_print("connect sever \n");
        sock =  socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
 
        if ( sock == -1) {
                perror("socket()");
                return FALSE;
        }
        
        
        bzero((char *) &sock_client, sizeof(sock_client));
        sock_client.sin_family = AF_INET;
        sock_client.sin_port = (unsigned short) htons(6666);
        src_host = (struct hostent *) gethostbyname(PRIVATE(launcher)->server_name);	
        if (!src_host) {
                fprintf(stderr, "Not a valid Server IP...\n");
                return FALSE;
        }
        
        bcopy( (char *) src_host->h_addr, (char *) &sock_client.sin_addr.s_addr, src_host->h_length);
        
        while (connect(sock, (struct sockaddr *) &sock_client, sizeof(sock_client)) == -1) {
                if (errno != EAGAIN)
                        {
                                perror("connect()");
                                return FALSE;
                        }
        }

        PRIVATE(launcher)->handler = monkey_message_handler_new(sock);
        
   
        g_signal_connect( G_OBJECT(PRIVATE(launcher)->handler),
                          "recv-message",
                          G_CALLBACK( recv_network_init_message ),
                          launcher);

        monkey_message_handler_start_listening(PRIVATE(launcher)->handler);
        
        return FALSE;
}


void network_game_launcher_finalize(GObject *object) {
        NetworkGameLauncher * ngl = (NetworkGameLauncher *) object;
	
        g_free(PRIVATE(ngl));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }		
}

static void network_game_launcher_instance_init(NetworkGameLauncher * ngl) {
        ngl->private =g_new0 (NetworkGameLauncherPrivate, 1);
}

static void network_game_launcher_class_init (NetworkGameLauncherClass *klass) {
        GObjectClass* object_class;
        
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = network_game_launcher_finalize;
}

GType network_game_launcher_get_type(void) {
        static GType network_game_launcher_type = 0;
        
        if (!network_game_launcher_type) {
                static const GTypeInfo network_game_launcher_info = {
                        sizeof(NetworkGameLauncherClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) network_game_launcher_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(NetworkGameLauncher),
                        1,              /* n_preallocs */
                        (GInstanceInitFunc) network_game_launcher_instance_init,
                };
                
                network_game_launcher_type = g_type_register_static(G_TYPE_OBJECT,
                                                                     "NetworkGameLauncher",
                                                                     &network_game_launcher_info, 0
                                                                     );

        }
        
        return network_game_launcher_type;
}

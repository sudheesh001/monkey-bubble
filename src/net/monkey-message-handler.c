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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <glib/gthread.h>
#include <sys/time.h>
#include <time.h>
 
#include "monkey-message-handler.h"
#include "monkey-net-marshal.h"

#define CHUNK_SIZE 32
#define SEND_MESSAGE 1

enum {
        CONNECTION_CLOSED,
        RECV_SHOOT,
        RECV_ADD_BUBBLE,
        RECV_WINLOST,
        RECV_START,
        RECV_MESSAGE,
        LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];

struct MonkeyMessageHandlerPrivate {
        int sock;
        GThread * main_thread;
        gboolean is_running;
};

#define PRIVATE( MonkeyMessageHandler ) (MonkeyMessageHandler->private)

static GObjectClass* parent_class = NULL;

void monkey_message_handler_finalize(GObject *);



gboolean read_chunk(MonkeyMessageHandler * mmh,
                guint8 * data);
	
MonkeyMessageHandler *monkey_message_handler_new(int sock) {
        MonkeyMessageHandler * mmh;
        
        mmh = 
                MONKEY_MESSAGE_HANDLER(g_object_new(TYPE_MONKEY_MESSAGE_HANDLER
                                                    , NULL));
        
        PRIVATE(mmh)->sock = sock;
        return(mmh);
        
}


void monkey_message_handler_finalize(GObject *object) {
        MonkeyMessageHandler * mmh = (MonkeyMessageHandler *) object;
	
        g_free(PRIVATE(mmh));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }		
}

static void monkey_message_handler_instance_init(MonkeyMessageHandler * mmh) {
        mmh->private =g_new0 (MonkeyMessageHandlerPrivate, 1);
}

static void monkey_message_handler_class_init (MonkeyMessageHandlerClass *klass) {
        GObjectClass* object_class;
        
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = monkey_message_handler_finalize;

        signals[RECV_SHOOT]= g_signal_new ("recv-shoot",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_FIRST |
                                           G_SIGNAL_NO_RECURSE,
                                           G_STRUCT_OFFSET (MonkeyMessageHandlerClass, recv_shoot),
                                           NULL, NULL,
                                           monkey_net_marshal_VOID__UINT_UINT_FLOAT,
                                           G_TYPE_NONE,
                                           3,G_TYPE_UINT,G_TYPE_UINT,G_TYPE_FLOAT);
        
        
        signals[RECV_ADD_BUBBLE]= g_signal_new ("recv-add-bubble",
                                           G_TYPE_FROM_CLASS (klass),
                                           G_SIGNAL_RUN_FIRST |
                                           G_SIGNAL_NO_RECURSE,
                                           G_STRUCT_OFFSET (MonkeyMessageHandlerClass,recv_add_bubble ),
                                           NULL, NULL,
                                           monkey_net_marshal_VOID__UINT_UINT,
                                           G_TYPE_NONE,
                                           2,G_TYPE_UINT,G_TYPE_UINT);
        
        
        signals[RECV_WINLOST] = g_signal_new ("recv-winlost",
                                              G_TYPE_FROM_CLASS (klass),
                                              G_SIGNAL_RUN_FIRST |
                                              G_SIGNAL_NO_RECURSE,
                                              G_STRUCT_OFFSET (MonkeyMessageHandlerClass, recv_winlost),
                                              NULL, NULL,
                                              monkey_net_marshal_VOID__UINT_BOOLEAN,
                                              G_TYPE_NONE,
                                              2,G_TYPE_UINT,G_TYPE_BOOLEAN);
        
 
        
        
        signals[RECV_START]= g_signal_new ("recv-start",
                                               G_TYPE_FROM_CLASS (klass),
                                               G_SIGNAL_RUN_FIRST |
                                               G_SIGNAL_NO_RECURSE,
                                               G_STRUCT_OFFSET (MonkeyMessageHandlerClass, recv_start),
                                               NULL, NULL,
                                               g_cclosure_marshal_VOID__VOID,
                                               G_TYPE_NONE,
                                               0,NULL);

        signals[RECV_MESSAGE]= g_signal_new ("recv-message",
                                             G_TYPE_FROM_CLASS (klass),
                                             G_SIGNAL_RUN_FIRST |
                                             G_SIGNAL_NO_RECURSE,
                                             G_STRUCT_OFFSET (MonkeyMessageHandlerClass, recv_message),
                                             NULL, NULL,
                                             monkey_net_marshal_VOID__UINT_POINTER,
                                             G_TYPE_NONE,
                                             2,G_TYPE_UINT,G_TYPE_POINTER);

        
        
        signals[CONNECTION_CLOSED]= g_signal_new ("connection-closed",
                                                  G_TYPE_FROM_CLASS (klass),
                                                  G_SIGNAL_RUN_FIRST |
                                                  G_SIGNAL_NO_RECURSE,
                                                  G_STRUCT_OFFSET (MonkeyMessageHandlerClass, connection_closed),
                                                  NULL, NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE,
                                                  0,NULL);

}

GType monkey_message_handler_get_type(void) {
        static GType monkey_message_handler_type = 0;
        
        if (!monkey_message_handler_type) {
                static const GTypeInfo monkey_message_handler_info = {
                        sizeof(MonkeyMessageHandlerClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) monkey_message_handler_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(MonkeyMessageHandler),
                        1,              /* n_preallocs */
                        (GInstanceInitFunc) monkey_message_handler_instance_init,
                };
                
                monkey_message_handler_type = g_type_register_static(G_TYPE_OBJECT,
                                                                     "MonkeyMessageHandler",
                                                                     &monkey_message_handler_info, 0
                                                                     );

        }
        
        return monkey_message_handler_type;
}

void parse_message(MonkeyMessageHandler * mnh,
                   guint8 * message) {

        struct Test {
                guint8 message_type;
                guint32 client_id;
                char message[32-5];
        };

        struct Test * t;

        t = (struct Test *) message;

        switch( t->message_type ) {
        case SEND_MESSAGE :
                g_signal_emit( G_OBJECT(mnh),signals[RECV_MESSAGE],0,
                               g_ntohl(t->client_id),
                               t->message);               
                break;
        default :
                break;
        }


}


void * handler_loop(MonkeyMessageHandler * mmh) {
        guint8  message[32];
        
        while( PRIVATE(mmh)->is_running) {
                g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"Waiting for incoming data\n");
                memset(message,0,32);
                if(read_chunk(mmh,message) ) {
                        parse_message(mmh,message);
                }
                // handle the message

        }
        g_signal_emit( G_OBJECT(mmh),signals[CONNECTION_CLOSED],0);
        return 0;
}


void monkey_message_handler_start_listening(MonkeyMessageHandler * mmh) {
        GError ** error;

        error = NULL;
        PRIVATE(mmh)->is_running = TRUE;
        PRIVATE(mmh)->main_thread =
                g_thread_create((GThreadFunc) handler_loop,mmh, TRUE, error);

}

void write_chunk(MonkeyMessageHandler * mmh,
                  guint8 * data,guint chunk_count) {

        guint size;
        guint wsize;

        size = chunk_count * CHUNK_SIZE;
        
        while( size > 0 ) {
                if( (wsize = write(PRIVATE(mmh)->sock, data, size)) < 1) {
                        g_error("write()");
                }

                size -= wsize;
        }
        
}

gboolean read_chunk(MonkeyMessageHandler * mmh,
                guint8 * data) {

        guint size;
        guint wsize;

        size = CHUNK_SIZE;
        wsize = 1;
        while( size > 0 && wsize > 0) {
                if( (wsize = read(PRIVATE(mmh)->sock, data, size)) < 1) {
                        g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"Connection close %d\n",PRIVATE(mmh)->sock);
                        PRIVATE(mmh)->is_running = FALSE;
                        return FALSE;
                } else {
                        size -= wsize;
                }
        }

        return TRUE;

}


void monkey_message_handler_send_message (MonkeyMessageHandler * mmh,
                                          guint32 client_id,
                                          const gchar * text,
                                          guint message_size) {


        guint8 message[CHUNK_SIZE];
        struct Test {
                guint8 message_type;
                guint32 client_id;
                gchar message[CHUNK_SIZE-5];
        };

        struct Test * t;

        g_assert( message_size < (CHUNK_SIZE-5) );

        t = (struct Test *)  message;
        

        memset(t,0,CHUNK_SIZE);
        t->message_type = SEND_MESSAGE;
        
        t->client_id = htonl( client_id);
        g_print("message :\n%s",text);
       g_strlcat( t->message,text,CHUNK_SIZE -5);
        
        write_chunk(mmh,message,1);
}


void monkey_message_handler_join(MonkeyMessageHandler * mnh) {
        g_thread_join( PRIVATE(mnh)->main_thread);
}
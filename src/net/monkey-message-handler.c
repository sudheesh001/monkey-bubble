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

#include <math.h>
#include <libxml/tree.h> 
#include "monkey-message-handler.h"
#include "monkey-net-marshal.h"

#define CHUNK_SIZE 32
#define SEND_MESSAGE 1
#define SEND_XML_MESSAGE 2
#define SEND_BUBBLE_ARRAY 3
#define SEND_ADD_BUBBLE 4
#define SEND_START 5
#define SEND_SHOOT 6
#define SEND_WAITING_ADDED 7
#define SEND_WINLOST 8
enum {
        CONNECTION_CLOSED,
        RECV_SHOOT,
        RECV_ADD_BUBBLE,
        RECV_WINLOST,
        RECV_WAITING_ADDED,
        RECV_START,
        RECV_BUBBLE_ARRAY,
        RECV_MESSAGE,
        RECV_XML_MESSAGE,
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


void parse_xml_message(MonkeyMessageHandler * mmh,
                       guint8 * message);

void parse_bubble_array(MonkeyMessageHandler * mmh,
                        guint8 * message);

void parse_add_bubble(MonkeyMessageHandler * mmh,
                      guint8 * message);

void parse_winlost(MonkeyMessageHandler * mmh,
                   guint8 * message);

void parse_waiting_added(MonkeyMessageHandler * mmh,
                         guint8 * message);

void parse_shoot(MonkeyMessageHandler * mmh,
                 guint8 * message);

gboolean read_chunk(MonkeyMessageHandler * mmh,
                guint8 * data);
	
MonkeyMessageHandler *
monkey_message_handler_new(int sock) {
        MonkeyMessageHandler * mmh;
        
        mmh = 
                MONKEY_MESSAGE_HANDLER(g_object_new(TYPE_MONKEY_MESSAGE_HANDLER
                                                    , NULL));
        
        PRIVATE(mmh)->sock = sock;
        return(mmh);
        
}


void 
monkey_message_handler_finalize(GObject *object) {
        MonkeyMessageHandler * mmh = (MonkeyMessageHandler *) object;
	
        g_free(PRIVATE(mmh));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }		
}

static void 
monkey_message_handler_instance_init(MonkeyMessageHandler * mmh) {
        mmh->private =g_new0 (MonkeyMessageHandlerPrivate, 1);
}

static void 
monkey_message_handler_class_init (MonkeyMessageHandlerClass *klass) {
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
        
        signals[RECV_WAITING_ADDED] = g_signal_new ("recv-waiting-added",
                                              G_TYPE_FROM_CLASS (klass),
                                              G_SIGNAL_RUN_FIRST |
                                              G_SIGNAL_NO_RECURSE,
                                              G_STRUCT_OFFSET (MonkeyMessageHandlerClass, recv_waiting_added),
                                              NULL, NULL,
                                              monkey_net_marshal_VOID__UINT_UINT_POINTER_POINTER,
                                              G_TYPE_NONE,
                                              4,G_TYPE_UINT,G_TYPE_UINT,G_TYPE_POINTER,G_TYPE_POINTER);
        
        
        
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


        signals[RECV_XML_MESSAGE]= g_signal_new ("recv-xml-message",
                                                 G_TYPE_FROM_CLASS (klass),
                                                 G_SIGNAL_RUN_FIRST |
                                                 G_SIGNAL_NO_RECURSE,
                                                 G_STRUCT_OFFSET (MonkeyMessageHandlerClass, recv_xml_message),
                                                 NULL, NULL,
                                                 monkey_net_marshal_VOID__UINT_POINTER,
                                                 G_TYPE_NONE,
                                                 2,G_TYPE_UINT,G_TYPE_POINTER);
        
        signals[RECV_BUBBLE_ARRAY]= g_signal_new ("recv-bubble-array",
                                             G_TYPE_FROM_CLASS (klass),
                                             G_SIGNAL_RUN_FIRST |
                                             G_SIGNAL_NO_RECURSE,
                                             G_STRUCT_OFFSET (MonkeyMessageHandlerClass, recv_bubble_array),
                                             NULL, NULL,
                                             monkey_net_marshal_VOID__UINT_UINT_POINTER,
                                             G_TYPE_NONE,
                                             3,G_TYPE_UINT,G_TYPE_UINT,G_TYPE_POINTER);
        
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

void parse_message(MonkeyMessageHandler * mmh,
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
                g_signal_emit( G_OBJECT(mmh),signals[RECV_MESSAGE],0,
                               g_ntohl(t->client_id),
                               t->message);               
                break;
        case SEND_XML_MESSAGE :
                parse_xml_message( mmh,
                                   message);
                break;
        case SEND_BUBBLE_ARRAY :
                parse_bubble_array(mmh,
                                   message);
        case SEND_WAITING_ADDED :
                parse_waiting_added(mmh,
                                    message);
                break;
        case SEND_ADD_BUBBLE :
                parse_add_bubble(mmh,message);
                break;
        case SEND_SHOOT :
                parse_shoot(mmh,message);
                break;
        case SEND_WINLOST :
                parse_winlost(mmh,message);
                break;
        case SEND_START :
                g_signal_emit( G_OBJECT(mmh),signals[RECV_START],0);
                break;

        default :
                break;
        }


}

void parse_xml_message(MonkeyMessageHandler * mmh,
                       guint8 * message) {

        guint8 message2[CHUNK_SIZE];
        gpointer xml_message;
        gint size;
        gint count;
        xmlChar * xmlMessage;
        xmlParserCtxt * ctxt; 
        xmlDocPtr doc;
        
        struct Test {
                guint8 message_type;
                guint32 client_id;
                guint32 chunk_count;
                gchar message[CHUNK_SIZE-(1+4+4)];
        };

        struct Test * t;
  
        t = (struct Test *)  message;
        t->client_id = g_ntohl( t->client_id);
        t->chunk_count = g_ntohl( t->chunk_count);

        size = (t->chunk_count-1) * CHUNK_SIZE;
        xml_message = g_malloc(size);

        memset(xml_message,0, size);
        count = 0;
        while( count < (t->chunk_count -1 )) {
                
                read_chunk(mmh,message2);
                memcpy((xml_message+(CHUNK_SIZE*count)),message2,CHUNK_SIZE);
                count++;
        }

        g_print("***********************************************\nxml readed\n %s\n*************************************************************\n",(char *)xml_message);

        xmlMessage = xmlStrdup((const xmlChar *) ( xml_message ));

        ctxt = xmlNewParserCtxt();
        if (ctxt == NULL) {
                fprintf(stderr, "Failed to allocate parser context\n");
                return;
        }
        doc = xmlCtxtReadDoc(ctxt, xmlMessage, 
                             NULL,NULL,0);
        
        
        g_signal_emit( G_OBJECT(mmh),signals[RECV_XML_MESSAGE],0,
                       g_ntohl(t->client_id),
                       doc);               
        
}

void * handler_loop(MonkeyMessageHandler * mmh) {
        guint8  message[CHUNK_SIZE];
        
        while( PRIVATE(mmh)->is_running) {
                memset(message,0,CHUNK_SIZE);
                if(read_chunk(mmh,message) ) {
                        parse_message(mmh,message);
                }
                // handle the message

        }
        g_signal_emit( G_OBJECT(mmh),signals[CONNECTION_CLOSED],0);
        return 0;
}


void monkey_message_handler_disconnect(MonkeyMessageHandler * mmh) {
        g_assert( IS_MONKEY_MESSAGE_HANDLER(mmh));

        close( PRIVATE(mmh)->sock );

        PRIVATE(mmh)->is_running = FALSE;

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
        guint p;
        guint writed;
        p = 0;
        writed = 0;
        size = chunk_count * CHUNK_SIZE;
        
        while( size > 0 ) {
                if( (writed = write(PRIVATE(mmh)->sock, data+p, size)) < 1) {
                        g_error("write()");
                }
                size -= writed;
                p += writed;
        }
        
}

gboolean read_chunk(MonkeyMessageHandler * mmh,
                    guint8 * data) {
        
        guint size,p;
        guint readed;
        size = CHUNK_SIZE;
        readed = 1;
        p = 0;
        while( size > 0 && readed > 0) {
                if( (readed = read(PRIVATE(mmh)->sock, data+p, size)) < 1) {
                        g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"Connection close %d\n",PRIVATE(mmh)->sock);
                        PRIVATE(mmh)->is_running = FALSE;
                        return FALSE;
                } else {
                        p += readed;
                        size -= readed;
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

        g_print("*********************************************************\nmessage :\n%s\n*****************************************************************\n",text);

        g_strlcat( t->message,text,CHUNK_SIZE -5);
        
        write_chunk(mmh,message,1);
}


void monkey_message_handler_send_xml_message(MonkeyMessageHandler * mmh,
                                             guint32 client_id,
                                             xmlDoc * doc) {

        xmlChar * mem;
        int size;
        guint32 computed_size;
        guint8 message[CHUNK_SIZE];
        gpointer xml_message;
 
        struct Test {
                guint8 message_type;
                guint32 client_id;
                guint32 chunk_count;
                gchar message[CHUNK_SIZE-(1+4+4)];
        };

        struct Test * t;


        t = (struct Test *)  message;
        memset(t,0,CHUNK_SIZE);
        t->message_type = SEND_XML_MESSAGE;
        
        t->client_id = htonl( client_id);

        mem = NULL;
        xmlDocDumpMemory(doc,&mem,&size);

        computed_size = size + CHUNK_SIZE;
        computed_size = ceil((( float)computed_size / CHUNK_SIZE)) * CHUNK_SIZE;// + computed_size % CHUNK_SIZE) ;

        t->chunk_count = htonl( computed_size / CHUNK_SIZE);

        xml_message = g_malloc( computed_size);
        g_print("****************************************************\nsend xml message:\n%s\n******************************************************************\n",mem);
        memset(xml_message,0,computed_size);
        memcpy(xml_message,t,CHUNK_SIZE);
        memcpy(xml_message+CHUNK_SIZE,mem,size);
        write_chunk(mmh,(guint8 *)xml_message,computed_size / CHUNK_SIZE );
        
}

void monkey_message_handler_join(MonkeyMessageHandler * mmh) {
        g_thread_join( PRIVATE(mmh)->main_thread);
}

void monkey_message_handler_send_waiting_added (MonkeyMessageHandler * mmh,
                                                guint32 monkey_id,
                                                guint8 bubbles_count,
                                                Color * colors,
                                                guint8 * columns) {

        guint8 message[CHUNK_SIZE];
        int i,j;

        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint8 bubbles_count;
        };

        struct Test * t;


        t = (struct Test *)  message;
        memset(t,0,CHUNK_SIZE);
        
        t->message_type = SEND_WAITING_ADDED;
        
        t->monkey_id = htonl( monkey_id);
        t->bubbles_count = bubbles_count;

        j = sizeof(struct Test);

        for(i = 0 ; i < bubbles_count; i++ ){
                if(j >= CHUNK_SIZE) {
                        write_chunk(mmh,message,1);
                        j = 0;
                        memset(message,0,CHUNK_SIZE);                        
                }

                g_print("colors[%d] = %d\n",i,colors[i]);
                message[j] = colors[i];
                j++;

        }

        
                        
        for(i = 0 ; i < bubbles_count; i++ ){
                if(j >= CHUNK_SIZE) {
                        write_chunk(mmh,message,1);
                        j = 0;
                        memset(message,0,CHUNK_SIZE);                        
                }

                
                g_print("columns[%d] = %d\n",i,columns[i]);
                message[j] = columns[i];
                j++;

        }

        write_chunk(mmh,message,1);

}


void parse_waiting_added(MonkeyMessageHandler * mmh,
                         guint8 * message) {

        int i,j;
        Color * bubbles;
        guint8 * columns;
        guint32 monkey_id;
        guint8 bubble_count;

        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint8 bubble_count;
        };

        struct Test * t;
  
        t = (struct Test *)  message;
        g_print("bubble count %d\n",t->bubble_count);
        bubbles = g_malloc( t->bubble_count);

        columns = g_malloc( t->bubble_count);

        monkey_id = g_ntohl( t->monkey_id);
        bubble_count = t->bubble_count;
        j = sizeof(struct Test);
        
        for(i = 0;  i < bubble_count ; i++) {
                
                g_print("colors[%d] = %d\n",i,message[j]);

                bubbles[i] = message[j];

                j++;
                if( j >= CHUNK_SIZE) {
                        g_print("read chunck\n");
                        read_chunk(mmh,message);
                        j = 0;
                        
                }

        }

        for(i = 0;  i < bubble_count ; i++) {

                g_print("columns[%d] = %d\n",i,message[j]);

                columns[i] = message[j];

                j++;
                if( j >= CHUNK_SIZE) {
                        g_print("read chunck\n");
                        read_chunk(mmh,message);
                        j = 0;
                        
                }

        }

        g_signal_emit( G_OBJECT(mmh),signals[RECV_WAITING_ADDED],0,
                       monkey_id,
                       bubble_count,
                       bubbles,
                       columns);   
        
}

void monkey_message_handler_send_bubble_array( MonkeyMessageHandler * mmh,
                                               guint32 monkey_id,
                                               guint8  bubbles_count,
                                               Color * bubbles) {

        guint8 message[CHUNK_SIZE];
        int i,j;

        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint8 bubbles_count;
        };

        struct Test * t;


        t = (struct Test *)  message;
        memset(t,0,CHUNK_SIZE);
        
        t->message_type = SEND_BUBBLE_ARRAY;
        
        t->monkey_id = htonl( monkey_id);
        t->bubbles_count = bubbles_count;
        j = sizeof(struct Test);

        for(i = 0 ; i < bubbles_count; i++ ){
                if(j >= CHUNK_SIZE) {
                        g_print("chunk wirted\n");
                        write_chunk(mmh,message,1);
                        j = 0;
                        memset(message,0,CHUNK_SIZE);                        
                }

                g_print("color[%d]=%d\n",i,bubbles[i]);
                message[j] = bubbles[i];
                j++;

        }

        write_chunk(mmh,message,1);

}

void parse_bubble_array(MonkeyMessageHandler * mmh,
                        guint8 * message) {

        int i,j;
        Color * bubbles;
        guint32 monkey_id;
        guint8 bubble_count;

        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint8 bubble_count;
        };

        struct Test * t;
  
        t = (struct Test *)  message;
        g_print("bubble count %d\n",t->bubble_count);
        bubbles = g_malloc( t->bubble_count);

        monkey_id = g_ntohl( t->monkey_id);
        bubble_count = t->bubble_count;
        j = sizeof(struct Test);

        for(i = 0;  i < bubble_count ; i++) {

                bubbles[i] = message[j];

                g_print("color[%d] %d\n",i,bubbles[i]);
                j++;
                if( j >= CHUNK_SIZE) {
                        g_print("read chunck\n");
                        read_chunk(mmh,message);
                        j = 0;
                        
                        g_print("readed chunck\n");
                }

        }

        g_signal_emit( G_OBJECT(mmh),signals[RECV_BUBBLE_ARRAY],0,
                       monkey_id,
                       bubble_count,
                       bubbles);       
        
}
                     
void monkey_message_handler_send_add_bubble  (MonkeyMessageHandler * mmh,
					      guint32 monkey_id,
					      Color color) {

        guint8 message[CHUNK_SIZE];

        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                Color bubble;
        };

        struct Test * t;


        t = (struct Test *)  message;
        memset(t,0,CHUNK_SIZE);
        
        t->message_type = SEND_ADD_BUBBLE;
        
        t->monkey_id = htonl( monkey_id);
        t->bubble = color;
        write_chunk(mmh,message,1);

}

void parse_add_bubble(MonkeyMessageHandler * mmh,
                      guint8 * message) {
        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint8 bubble;
        };

        struct Test * t;
  
        t = (struct Test *)  message;

        t->monkey_id = g_ntohl( t->monkey_id);
        
        g_signal_emit( G_OBJECT(mmh),signals[RECV_ADD_BUBBLE],0,
                       t->monkey_id,
                       t->bubble);     
 
}

void monkey_message_handler_send_shoot       (MonkeyMessageHandler * mmh,
					      guint32 monkey_id,
					      guint32 time,
                                              gfloat angle) {

        guint8 message[CHUNK_SIZE];

        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint32 time;
                gint32 angle;
        };

        struct Test * t;


        t = (struct Test *)  message;
        memset(t,0,CHUNK_SIZE);
        
        t->message_type = SEND_SHOOT;
        
        t->monkey_id = htonl( monkey_id);
        t->time = htonl(time);
        t->angle = htonl( (gint32)rint(  angle*100000 ) );
        write_chunk(mmh,message,1);
}



void parse_shoot(MonkeyMessageHandler * mmh,
                      guint8 * message) {


        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint32 time;
                gint32 angle;
        };
        struct Test * t;
  
        t = (struct Test *)  message;

        t->monkey_id = g_ntohl( t->monkey_id);
        t->time = g_ntohl( t->time);
        t->angle = g_ntohl( t->angle);
        g_signal_emit( G_OBJECT(mmh),signals[RECV_SHOOT],0,
                       t->monkey_id,
                       t->time,
                       ((gfloat)t->angle)/100000);     
 
}

void monkey_message_handler_send_winlost     (MonkeyMessageHandler * mmh,
					      guint32 monkey_id,
					      guint8 win_lost) {
        guint8 message[CHUNK_SIZE];

        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint8  win_lost;
        };
        struct Test * t;
        
        t = (struct Test *)  message;
        
        memset(t,0,CHUNK_SIZE);
        
        t->message_type = SEND_WINLOST;
        t->monkey_id =  htonl( monkey_id);
        t->win_lost = win_lost;

        write_chunk(mmh,message,1);

}



void parse_winlost(MonkeyMessageHandler * mmh,
                      guint8 * message) {


        struct Test {
                guint8 message_type;
                guint32 monkey_id;
                guint8 winlost;
        };
        struct Test * t;
  
        t = (struct Test *)  message;

        t->monkey_id = g_ntohl( t->monkey_id);

        g_signal_emit( G_OBJECT(mmh),signals[RECV_WINLOST],0,
                       t->monkey_id,
                       t->winlost);     
 
}

void monkey_message_handler_send_start       (MonkeyMessageHandler * mmh) {

        guint8 message[CHUNK_SIZE];



        memset(message,0,CHUNK_SIZE);
        message[0] = SEND_START;
        write_chunk(mmh,message,1);

}



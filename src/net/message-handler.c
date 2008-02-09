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

#include <netdb.h>

#include <math.h>
#include <libxml/tree.h>
#include "message-handler.h"
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
#define SEND_NEXT_RANGE 9
#define SEND_ADD_ROW 10
#define SEND_SCORE 11

enum
{
	CONNECTION_CLOSED,
	RECV_SHOOT,
	RECV_ADD_BUBBLE,
	RECV_WINLOST,
	RECV_WAITING_ADDED,
	RECV_START,
	RECV_BUBBLE_ARRAY,
	RECV_MESSAGE,
	RECV_XML_MESSAGE,
	RECV_NEXT_RANGE,
        RECV_ADD_ROW,
        RECV_SCORE,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];

struct _NetworkMessageHandlerPrivate
{
	int sock;
	GThread *main_thread;
	gboolean is_running;
	gboolean thread_stopped;
};

#define PRIVATE( NetworkMessageHandler ) (NetworkMessageHandler->private)

static GObjectClass *parent_class = NULL;

void network_message_handler_finalize (GObject *);


void parse_xml_message (NetworkMessageHandler * mmh, guint8 * message);

void parse_bubble_array (NetworkMessageHandler * mmh, guint8 * message);

void parse_add_bubble (NetworkMessageHandler * mmh, guint8 * message);

void parse_winlost (NetworkMessageHandler * mmh, guint8 * message);

void parse_score (NetworkMessageHandler * mmh, guint8 * message);

void parse_waiting_added (NetworkMessageHandler * mmh, guint8 * message);

void parse_add_row (NetworkMessageHandler * mmh, guint8 * message);

void parse_shoot (NetworkMessageHandler * mmh, guint8 * message);

void parse_next_range (NetworkMessageHandler * mmh, guint8 * message);

gboolean read_chunk (NetworkMessageHandler * mmh, guint8 * data);

G_DEFINE_TYPE (NetworkMessageHandler, network_message_handler, G_TYPE_OBJECT);

NetworkMessageHandler *
network_message_handler_new (int sock)
{
	NetworkMessageHandler *mmh;

	mmh = NETWORK_MESSAGE_HANDLER (g_object_new
				       (NETWORK_TYPE_MESSAGE_HANDLER, NULL));

	PRIVATE (mmh)->sock = sock;
	PRIVATE (mmh)->thread_stopped = TRUE;
	return (mmh);

}

gboolean
network_message_handler_connect (NetworkMessageHandler * handler,
				 const gchar * host, int port)
{


	int sock;
	struct sockaddr_in sock_client;
	struct hostent *src_host;

#ifdef DEBUG
	g_print ("network-message-handler : connect sever \n");
#endif
	sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);


	if (sock == -1)
	{
		perror ("socket()");
		return FALSE;
	}


	bzero ((char *) &sock_client, sizeof (sock_client));
	sock_client.sin_family = AF_INET;
	sock_client.sin_port = (unsigned short) htons (port);
	src_host = (struct hostent *) gethostbyname (host);
	if (!src_host)
	{
		fprintf (stderr,
			 "network-message-handler :Not a valid Server IP...\n");
		return FALSE;
	}

	bcopy ((char *) src_host->h_addr,
	       (char *) &sock_client.sin_addr.s_addr, src_host->h_length);

	while (connect
	       (sock, (struct sockaddr *) &sock_client,
		sizeof (sock_client)) == -1)
	{
		if (errno != EAGAIN)
		{
			perror ("connect()");
			return FALSE;
		}
	}

	PRIVATE (handler)->sock = sock;
	return TRUE;
}


void
network_message_handler_finalize (GObject * object)
{
	NetworkMessageHandler *mmh = NETWORK_MESSAGE_HANDLER (object);

	if (PRIVATE (mmh)->sock != -1)
	{
		network_message_handler_disconnect (mmh);
	}

	if (PRIVATE (mmh)->thread_stopped == FALSE)
	{
		network_message_handler_join (mmh);
	}

	g_free (PRIVATE (mmh));

	if (G_OBJECT_CLASS (parent_class)->finalize)
	{
		(*G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
network_message_handler_init (NetworkMessageHandler* mmh)
{
	mmh->private = g_new0 (NetworkMessageHandlerPrivate, 1);
}

static void
network_message_handler_class_init (NetworkMessageHandlerClass * klass)
{
	GObjectClass *object_class;

	parent_class = g_type_class_peek_parent (klass);
	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = network_message_handler_finalize;

	signals[RECV_SHOOT] = g_signal_new ("recv-shoot",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST |
					    G_SIGNAL_NO_RECURSE,
					    G_STRUCT_OFFSET
					    (NetworkMessageHandlerClass,
					     recv_shoot), NULL, NULL,
					    monkey_net_marshal_VOID__UINT_UINT_FLOAT,
					    G_TYPE_NONE, 3, G_TYPE_UINT,
					    G_TYPE_UINT, G_TYPE_FLOAT);


	signals[RECV_ADD_BUBBLE] = g_signal_new ("recv-add-bubble",
						 G_TYPE_FROM_CLASS (klass),
						 G_SIGNAL_RUN_FIRST |
						 G_SIGNAL_NO_RECURSE,
						 G_STRUCT_OFFSET
						 (NetworkMessageHandlerClass,
						  recv_add_bubble), NULL,
						 NULL,
						 monkey_net_marshal_VOID__UINT_UINT,
						 G_TYPE_NONE, 2, G_TYPE_UINT,
						 G_TYPE_UINT);


	signals[RECV_WINLOST] = g_signal_new ("recv-winlost",
					      G_TYPE_FROM_CLASS (klass),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      G_STRUCT_OFFSET
					      (NetworkMessageHandlerClass,
					       recv_winlost), NULL, NULL,
					      monkey_net_marshal_VOID__UINT_BOOLEAN,
					      G_TYPE_NONE, 2, G_TYPE_UINT,
					      G_TYPE_BOOLEAN);
	signals[RECV_SCORE] = g_signal_new ("recv-score",
					      G_TYPE_FROM_CLASS (klass),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      G_STRUCT_OFFSET
					      (NetworkMessageHandlerClass,
					       recv_score), NULL, NULL,
					      monkey_net_marshal_VOID__UINT_UINT,
					      G_TYPE_NONE, 2, G_TYPE_UINT,
					      G_TYPE_UINT);

	signals[RECV_WAITING_ADDED] = g_signal_new ("recv-waiting-added",
						    G_TYPE_FROM_CLASS (klass),
						    G_SIGNAL_RUN_FIRST |
						    G_SIGNAL_NO_RECURSE,
						    G_STRUCT_OFFSET
						    (NetworkMessageHandlerClass,
						     recv_waiting_added),
						    NULL, NULL,
						    monkey_net_marshal_VOID__UINT_UINT_UINT,
						    G_TYPE_NONE, 3,
						    G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT);



	signals[RECV_START] = g_signal_new ("recv-start",
					    G_TYPE_FROM_CLASS (klass),
					    G_SIGNAL_RUN_FIRST |
					    G_SIGNAL_NO_RECURSE,
					    G_STRUCT_OFFSET
					    (NetworkMessageHandlerClass,
					     recv_start), NULL, NULL,
					    g_cclosure_marshal_VOID__VOID,
					    G_TYPE_NONE, 0, NULL);

	signals[RECV_MESSAGE] = g_signal_new ("recv-message",
					      G_TYPE_FROM_CLASS (klass),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      G_STRUCT_OFFSET
					      (NetworkMessageHandlerClass,
					       recv_message), NULL, NULL,
					      monkey_net_marshal_VOID__UINT_POINTER,
					      G_TYPE_NONE, 2, G_TYPE_UINT,
					      G_TYPE_POINTER);


	signals[RECV_XML_MESSAGE] = g_signal_new ("recv-xml-message",
						  G_TYPE_FROM_CLASS (klass),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET
						  (NetworkMessageHandlerClass,
						   recv_xml_message), NULL,
						  NULL,
						  monkey_net_marshal_VOID__UINT_POINTER,
						  G_TYPE_NONE, 2, G_TYPE_UINT,
						  G_TYPE_POINTER);

	signals[RECV_BUBBLE_ARRAY] = g_signal_new ("recv-bubble-array",
						   G_TYPE_FROM_CLASS (klass),
						   G_SIGNAL_RUN_FIRST |
						   G_SIGNAL_NO_RECURSE,
						   G_STRUCT_OFFSET
						   (NetworkMessageHandlerClass,
						    recv_bubble_array), NULL,
						   NULL,
						   monkey_net_marshal_VOID__UINT_UINT_POINTER_UINT,
						   G_TYPE_NONE, 4,
						   G_TYPE_UINT, G_TYPE_UINT,
						   G_TYPE_POINTER,
                                                   G_TYPE_UINT);

	signals[CONNECTION_CLOSED] = g_signal_new ("connection-closed",
						   G_TYPE_FROM_CLASS (klass),
						   G_SIGNAL_RUN_FIRST |
						   G_SIGNAL_NO_RECURSE,
						   G_STRUCT_OFFSET
						   (NetworkMessageHandlerClass,
						    connection_closed), NULL,
						   NULL,
						   g_cclosure_marshal_VOID__VOID,
						   G_TYPE_NONE, 0, NULL);

	signals[RECV_NEXT_RANGE] = g_signal_new ("recv-next-range",
						 G_TYPE_FROM_CLASS (klass),
						 G_SIGNAL_RUN_FIRST |
						 G_SIGNAL_NO_RECURSE,
						 G_STRUCT_OFFSET
						 (NetworkMessageHandlerClass,
						  recv_next_range), NULL,
						 NULL,
						 monkey_net_marshal_VOID__UINT_POINTER,
						 G_TYPE_NONE, 2, G_TYPE_UINT,
						 G_TYPE_POINTER);


	signals[RECV_ADD_ROW] = g_signal_new ("recv-add-row",
						 G_TYPE_FROM_CLASS (klass),
						 G_SIGNAL_RUN_FIRST |
						 G_SIGNAL_NO_RECURSE,
						 G_STRUCT_OFFSET
						 (NetworkMessageHandlerClass,
						  recv_add_row), NULL,
						 NULL,
						 monkey_net_marshal_VOID__UINT_POINTER,
						 G_TYPE_NONE, 2, G_TYPE_UINT,
						 G_TYPE_POINTER);
}

void
parse_message (NetworkMessageHandler * mmh, guint8 * message)
{

	struct Test
	{
		guint8 message_type;
		guint32 client_id;
		char message[32 - 5];
	};

	struct Test *t;

	t = (struct Test *) message;

	switch (t->message_type)
	{
	case SEND_MESSAGE:
		g_signal_emit (G_OBJECT (mmh), signals[RECV_MESSAGE], 0,
			       g_ntohl (t->client_id), t->message);
		break;
	case SEND_XML_MESSAGE:
		parse_xml_message (mmh, message);
		break;
	case SEND_BUBBLE_ARRAY:
		parse_bubble_array (mmh, message);
		break;
	case SEND_WAITING_ADDED:
		parse_waiting_added (mmh, message);
		break;
	case SEND_ADD_ROW:
		parse_add_row (mmh, message);
		break;
	case SEND_ADD_BUBBLE:
		parse_add_bubble (mmh, message);
		break;
	case SEND_SHOOT:
		parse_shoot (mmh, message);
		break;
	case SEND_WINLOST:
		parse_winlost (mmh, message);
		break;
	case SEND_START:
		g_signal_emit (G_OBJECT (mmh), signals[RECV_START], 0);
		break;
	case SEND_NEXT_RANGE:
		parse_next_range (mmh, message);
		break;
	case SEND_SCORE:
		parse_score (mmh, message);
		break;

	default:
		break;
	}


}

void
parse_xml_message (NetworkMessageHandler * mmh, guint8 * message)
{

	guint8 message2[CHUNK_SIZE];
	gpointer xml_message;
	gint size;
	gint count;
	xmlChar *xmlMessage;
	xmlParserCtxt *ctxt;
	xmlDocPtr doc;

	struct Test
	{
		guint8 message_type;
		guint32 client_id;
		guint32 chunk_count;
		gchar message[CHUNK_SIZE - (1 + 4 + 4)];
	};

	struct Test *t;

	t = (struct Test *) message;
	t->client_id = g_ntohl (t->client_id);
	t->chunk_count = g_ntohl (t->chunk_count);

	size = (t->chunk_count - 1) * CHUNK_SIZE;
	xml_message = g_malloc (size + 1);

	memset (xml_message, 0, size + 1);
	count = 0;
	while (count < (t->chunk_count - 1))
	{

		read_chunk (mmh, message2);
		memcpy ((xml_message + (CHUNK_SIZE * count)), message2,
			CHUNK_SIZE);
		count++;
	}


	xmlMessage = xmlStrdup ((const xmlChar *) (xml_message));

	g_free (xml_message);
	ctxt = xmlNewParserCtxt ();
	if (ctxt == NULL)
	{
		fprintf (stderr, "Failed to allocate parser context\n");
		return;
	}
	doc = xmlCtxtReadDoc (ctxt, xmlMessage, NULL, NULL, 0);


	g_signal_emit (G_OBJECT (mmh), signals[RECV_XML_MESSAGE], 0,
		       g_ntohl (t->client_id), doc);

}

void *
handler_loop (NetworkMessageHandler * mmh)
{
	guint8 message[CHUNK_SIZE];

	while (PRIVATE (mmh)->is_running)
	{
		memset (message, 0, CHUNK_SIZE);
		if (read_chunk (mmh, message))
		{
#ifdef DEBUG
			g_debug ("\nrecieve message %d\n", message[0]);
#endif
			parse_message (mmh, message);
		}
		// handle the message

	}

	PRIVATE (mmh)->thread_stopped = TRUE;
	network_message_handler_disconnect (mmh);
	g_signal_emit (G_OBJECT (mmh), signals[CONNECTION_CLOSED], 0);
	return 0;
}


void
network_message_handler_disconnect (NetworkMessageHandler * mmh)
{
	g_assert (IS_NETWORK_MESSAGE_HANDLER (mmh));

#ifdef DEBUG
	g_print ("call close ...\n");
#endif
	shutdown (PRIVATE (mmh)->sock, 2);
	close (PRIVATE (mmh)->sock);

	PRIVATE (mmh)->sock = -1;
	PRIVATE (mmh)->is_running = FALSE;

}

void
network_message_handler_start_listening (NetworkMessageHandler * mmh)
{
	GError **error;

	error = NULL;
	PRIVATE (mmh)->is_running = TRUE;
	PRIVATE (mmh)->thread_stopped = FALSE;
	PRIVATE (mmh)->main_thread =
		g_thread_create ((GThreadFunc) handler_loop, mmh, TRUE,
				 error);

}

void
write_chunk (NetworkMessageHandler * mmh, guint8 * data, guint chunk_count)
{

	guint size;
	guint p;
	guint writed;
	p = 0;
	writed = 0;
	size = chunk_count * CHUNK_SIZE;

	while (size > 0)
	{
		if ((writed =
		     write (PRIVATE (mmh)->sock, data + p, size)) < 1)
		{
			g_error ("write()");
		}
		size -= writed;
		p += writed;
	}

}

gboolean
read_chunk (NetworkMessageHandler * mmh, guint8 * data)
{

	guint size, p;
	guint readed;
	size = CHUNK_SIZE;
	readed = 1;
	p = 0;
	while (size > 0 && readed > 0)
	{
		if ((readed = read (PRIVATE (mmh)->sock, data + p, size)) < 1)
		{
			g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			       "Connection close %d\n", PRIVATE (mmh)->sock);
			PRIVATE (mmh)->is_running = FALSE;
			return FALSE;
		}
		else
		{
			p += readed;
			size -= readed;
		}


	}


	return TRUE;

}


void
network_message_handler_send_message (NetworkMessageHandler * mmh,
				      guint32 client_id,
				      const gchar * text, guint message_size)
{


	guint8 message[CHUNK_SIZE];
	struct Test
	{
		guint8 message_type;
		guint32 client_id;
		gchar message[CHUNK_SIZE - 5];
	};

	struct Test *t;

	g_assert (message_size < (CHUNK_SIZE - 5));

	t = (struct Test *) message;


	memset (t, 0, CHUNK_SIZE);
	t->message_type = SEND_MESSAGE;

	t->client_id = htonl (client_id);


	g_strlcat (t->message, text, CHUNK_SIZE - 5);

	write_chunk (mmh, message, 1);
}


void
network_message_handler_send_xml_message (NetworkMessageHandler * mmh,
					  guint32 client_id, xmlDoc * doc)
{

	xmlChar *mem;
	int size;
	guint32 computed_size;
	guint8 message[CHUNK_SIZE];
	gpointer xml_message;

	struct Test
	{
		guint8 message_type;
		guint32 client_id;
		guint32 chunk_count;
		gchar message[CHUNK_SIZE - (1 + 4 + 4)];
	};

	struct Test *t;


	t = (struct Test *) message;
	memset (t, 0, CHUNK_SIZE);
	t->message_type = SEND_XML_MESSAGE;

	t->client_id = htonl (client_id);

	mem = NULL;
	xmlDocDumpMemory (doc, &mem, &size);

	computed_size = size + CHUNK_SIZE;
	computed_size =
		ceil (((float) computed_size / CHUNK_SIZE)) * CHUNK_SIZE;
	t->chunk_count = htonl (computed_size / CHUNK_SIZE);

	xml_message = g_malloc (computed_size);
	memset (xml_message, 0, computed_size);
	memcpy (xml_message, t, CHUNK_SIZE);
	memcpy (xml_message + CHUNK_SIZE, mem, size);

	write_chunk (mmh, (guint8 *) xml_message, computed_size / CHUNK_SIZE);

	xmlFree (mem);

}

void
network_message_handler_join (NetworkMessageHandler * mmh)
{
	g_thread_join (PRIVATE (mmh)->main_thread);
}

void
network_message_handler_send_waiting_added (NetworkMessageHandler * mmh,
					    guint32 monkey_id,
                                            guint32 time,
					    guint8 bubbles_count)
{

	guint8 message[CHUNK_SIZE];

	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
                guint32 time;
		guint8 bubbles_count;
	};

	struct Test *t;


	t = (struct Test *) message;
	memset (t, 0, CHUNK_SIZE);

	t->message_type = SEND_WAITING_ADDED;

	t->monkey_id = htonl (monkey_id);
	t->time = htonl (time);
	t->bubbles_count = bubbles_count;
	write_chunk (mmh, message, 1);
}


void
parse_waiting_added (NetworkMessageHandler * mmh, guint8 * message)
{

	Color *bubbles;
	guint8 *columns;
	guint32 monkey_id;
	guint8 bubble_count;
        guint32 time;
	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
                guint32 time;
		guint8 bubble_count;
	};

	struct Test *t;

	t = (struct Test *) message;
	bubbles = g_malloc (t->bubble_count);

	columns = g_malloc (t->bubble_count);

	monkey_id = g_ntohl (t->monkey_id);
        time = g_ntohl (t->time);
	bubble_count = t->bubble_count;

#ifdef DEBUG
	g_print ("waiting added %d\n", bubble_count);
#endif
	g_signal_emit (G_OBJECT (mmh), signals[RECV_WAITING_ADDED], 0,
		       monkey_id,time, bubble_count);

}



void
parse_add_row (NetworkMessageHandler * mmh, guint8 * message)
{
	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		Color colors[7];
	};

        int i;
	struct Test *t;

	t = (struct Test *) message;

	t->monkey_id = g_ntohl (t->monkey_id);


        g_print("recieve row \n");
	for (i = 0; i < 7; i++)
	{
                if( t->colors[i] != NO_COLOR ) {
                g_print(" %d ",t->colors[i]);
                } else {
                g_print("   ");
                }
	}
        g_print("\n");
	g_signal_emit (G_OBJECT (mmh), signals[RECV_ADD_ROW], 0,
		       t->monkey_id, t->colors);
}

void
network_message_handler_send_add_row (NetworkMessageHandler * mmh,
					 guint32 monkey_id, Color * colors)
{

	guint8 message[CHUNK_SIZE];
	int i;
	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		Color colors[7];
	};

	struct Test *t;


	t = (struct Test *) message;
	memset (t, 0, CHUNK_SIZE);

	t->message_type = SEND_ADD_ROW;

	t->monkey_id = htonl (monkey_id);

        g_print("add row \n");
	for (i = 0; i < 7; i++)
	{
		t->colors[i] = colors[i];
                if( colors[i] != NO_COLOR ) {
                g_print(" %d ",colors[i]);
                } else {
                g_print("   ");
                }
	}
        g_print("\n");
	write_chunk (mmh, message, 1);

}


static int
calculate_size (int size)
{
	return (int) ceil (((float) size / CHUNK_SIZE)) * CHUNK_SIZE;
}

void
network_message_handler_send_bubble_array (NetworkMessageHandler * mmh,
					   guint32 monkey_id,
					   guint8 bubbles_count,
					   Color * bubbles,
                                           guint32 odd)
{

	guint8 *message;
	int i, j;
	int size;
	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint8 bubbles_count;
                guint8 odd;
	};

	struct Test *t;

	i = sizeof (struct Test) + bubbles_count;
	size = calculate_size (i);
	message = g_malloc (size);

	t = (struct Test *) message;
	memset (t, 0, size);

	t->message_type = SEND_BUBBLE_ARRAY;

	t->monkey_id = htonl (monkey_id);
	t->bubbles_count = bubbles_count;
        t->odd = odd;
	j = sizeof (struct Test);

	for (i = 0; i < bubbles_count; i++)
	{

		message[j] = bubbles[i];
		j++;

	}
	write_chunk (mmh, message, size / CHUNK_SIZE);

}

void
parse_bubble_array (NetworkMessageHandler * mmh, guint8 * message)
{

	int i, j;
	Color *bubbles;
	guint32 monkey_id;
	guint8 bubble_count;

        guint8 odd;

	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint8 bubble_count;
                guint8 odd;
	};

	struct Test *t;

	t = (struct Test *) message;
	bubbles = g_malloc (t->bubble_count);

	monkey_id = g_ntohl (t->monkey_id);
	bubble_count = t->bubble_count;
        odd = t->odd;
	j = sizeof (struct Test);

	for (i = 0; i < bubble_count; i++)
	{

		bubbles[i] = message[j];

		j++;
		if (j >= CHUNK_SIZE)
		{
			read_chunk (mmh, message);
			j = 0;

		}

	}

	g_signal_emit (G_OBJECT (mmh), signals[RECV_BUBBLE_ARRAY], 0,
		       monkey_id, bubble_count, bubbles,(guint32)odd);

}

void
network_message_handler_send_add_bubble (NetworkMessageHandler * mmh,
					 guint32 monkey_id, Color color)
{

	guint8 message[CHUNK_SIZE];

	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		Color bubble;
	};

	struct Test *t;


	t = (struct Test *) message;
	memset (t, 0, CHUNK_SIZE);

	t->message_type = SEND_ADD_BUBBLE;

	t->monkey_id = htonl (monkey_id);
	t->bubble = color;
	write_chunk (mmh, message, 1);

}

void
parse_add_bubble (NetworkMessageHandler * mmh, guint8 * message)
{
	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint8 bubble;
	};

	struct Test *t;

	t = (struct Test *) message;

	t->monkey_id = g_ntohl (t->monkey_id);

	g_signal_emit (G_OBJECT (mmh), signals[RECV_ADD_BUBBLE], 0,
		       t->monkey_id, t->bubble);

}


void
parse_next_range (NetworkMessageHandler * mmh, guint8 * message)
{
	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		Color colors[8];
	};

	struct Test *t;

	t = (struct Test *) message;

	t->monkey_id = g_ntohl (t->monkey_id);

	g_signal_emit (G_OBJECT (mmh), signals[RECV_NEXT_RANGE], 0,
		       t->monkey_id, t->colors);
}

void
network_message_handler_send_next_range (NetworkMessageHandler * mmh,
					 guint32 monkey_id, Color * colors)
{

	guint8 message[CHUNK_SIZE];
	int i;
	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		Color colors[8];
	};

	struct Test *t;


	t = (struct Test *) message;
	memset (t, 0, CHUNK_SIZE);

	t->message_type = SEND_NEXT_RANGE;

	t->monkey_id = htonl (monkey_id);

	for (i = 0; i < 8; i++)
	{
		t->colors[i] = colors[i];
	}
	write_chunk (mmh, message, 1);

}


void
network_message_handler_send_shoot (NetworkMessageHandler * mmh,
				    guint32 monkey_id,
				    guint32 time, gfloat angle)
{

	guint8 message[CHUNK_SIZE];

	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint32 time;
		gint32 angle;
	};

	struct Test *t;


	t = (struct Test *) message;
	memset (t, 0, CHUNK_SIZE);

	t->message_type = SEND_SHOOT;

	t->monkey_id = htonl (monkey_id);
	t->time = htonl (time);
	t->angle = htonl ((gint32) rint (angle * 100000));
	write_chunk (mmh, message, 1);
}



void
parse_shoot (NetworkMessageHandler * mmh, guint8 * message)
{


	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint32 time;
		gint32 angle;
	};
	struct Test *t;

	t = (struct Test *) message;

	t->monkey_id = g_ntohl (t->monkey_id);
	t->time = g_ntohl (t->time);
	t->angle = g_ntohl (t->angle);
	g_signal_emit (G_OBJECT (mmh), signals[RECV_SHOOT], 0,
		       t->monkey_id, t->time, ((gfloat) t->angle) / 100000);

}

void
network_message_handler_send_winlost (NetworkMessageHandler * mmh,
				      guint32 monkey_id, guint8 win_lost)
{
	guint8 message[CHUNK_SIZE];

	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint8 win_lost;
	};
	struct Test *t;

	t = (struct Test *) message;

	memset (t, 0, CHUNK_SIZE);

	t->message_type = SEND_WINLOST;
	t->monkey_id = htonl (monkey_id);
	t->win_lost = win_lost;

	write_chunk (mmh, message, 1);

}



void
parse_winlost (NetworkMessageHandler * mmh, guint8 * message)
{


	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint8 winlost;
	};
	struct Test *t;

	t = (struct Test *) message;

	t->monkey_id = g_ntohl (t->monkey_id);

	g_signal_emit (G_OBJECT (mmh), signals[RECV_WINLOST], 0,
		       t->monkey_id, t->winlost);

}



void
network_message_handler_send_score (NetworkMessageHandler * mmh,
				      guint32 monkey_id, guint8 score)
{
	guint8 message[CHUNK_SIZE];

	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint8 score;
	};
	struct Test *t;

	t = (struct Test *) message;

	memset (t, 0, CHUNK_SIZE);

	t->message_type = SEND_SCORE;
	t->monkey_id = htonl (monkey_id);
	t->score = score;

	write_chunk (mmh, message, 1);

}



void
parse_score (NetworkMessageHandler * mmh, guint8 * message)
{


	struct Test
	{
		guint8 message_type;
		guint32 monkey_id;
		guint8 score;
	};
	struct Test *t;

	t = (struct Test *) message;

	t->monkey_id = g_ntohl (t->monkey_id);

	g_signal_emit (G_OBJECT (mmh), signals[RECV_SCORE], 0,
		       t->monkey_id, t->score);

}


void
network_message_handler_send_start (NetworkMessageHandler * mmh)
{

	guint8 message[CHUNK_SIZE];



	memset (message, 0, CHUNK_SIZE);
	message[0] = SEND_START;
	write_chunk (mmh, message, 1);

}

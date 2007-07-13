/* this file is part of monkey-bubble
 *
 * AUTHORS
 *       Laurent Belminte        <laurent.belmonte@gmail.com>
 *
 * Copyright (C) 2007 Laurent Belmonte
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
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

#include <netdb.h>

#include <glib/gthread.h>
#include <sys/time.h>
#include <time.h>



#include "mb-net-connection.h"

#include <glib.h>
#include <glib-object.h>


#define MAX_WAITING_CONN 20

typedef struct _Private {
	gchar *host;
	guint port;
	gint socket;
	GThread *main_thread;
	gboolean running;
	gboolean stop;
	GMutex *start_mutex;
	GCond *start_cond;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	RECEIVE,
	NEW_CONNECTION,
	DISCONNECTED,
	RECEIVE_MESSAGE,
	N_SIGNALS
};

static GQuark error_quark;

static GObjectClass *parent_class = NULL;

static void mb_net_connection_get_property(GObject * object,
					   guint prop_id,
					   GValue * value,
					   GParamSpec * param_spec);
static void mb_net_connection_set_property(GObject * object,
					   guint prop_id,
					   const GValue * value,
					   GParamSpec * param_spec);


static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetConnection, mb_net_connection, G_TYPE_OBJECT, {
			});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_CONNECTION, Private))


static void _mb_net_connection_send_message(MbNetConnection * self,
					    guint32 s, const guint8 * data,
					    GError ** error);



static void mb_net_connection_finalize(MbNetConnection * self);

static void mb_net_connection_init(MbNetConnection * self);
void mb_net_connection_set_uri(MbNetConnection * self, const gchar * uri,
			       GError ** error);

gint mb_net_connection_bind(MbNetConnection * self, const gchar * uri,
			    GError ** error);

void mb_net_connection_close(MbNetConnection * self, GError ** error);


void mb_net_connection_stop(MbNetConnection * self, GError ** error);

static void mb_net_connection_init(MbNetConnection * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	priv->host = NULL;
	priv->socket = -1;
}


static void mb_net_connection_finalize(MbNetConnection * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);


	if (priv->socket != -1) {
		mb_net_connection_close(self, NULL);
	}

	if (priv->main_thread != NULL) {
		mb_net_connection_stop(self, NULL);
	}

	if (priv->host != NULL) {
		g_free(priv->host);
	}
	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}


void mb_net_connection_close(MbNetConnection * self, GError ** error)
{
	Private *priv;
	priv = GET_PRIVATE(self);
	if (priv->socket != -1) {
		int ret;
		ret = shutdown(priv->socket, 2);
		if (ret == -1) {

			g_set_error(error, error_quark,
				    MB_NET_CONNECTION_SOCKET_ERROR,
				    "unable to close socket");
		}
	}

}

void mb_net_connection_stop(MbNetConnection * self, GError ** error)
{
	Private *priv;
	GError *err;
	err = NULL;
	priv = GET_PRIVATE(self);
	mb_net_connection_close(self, &err);

	if (err != NULL) {
		g_propagate_error(error, err);
		priv->socket = -1;
		return;
	}
	if (priv->main_thread != NULL && priv->running == TRUE) {
		priv->stop = TRUE;
		g_thread_join(priv->main_thread);
	}
	priv->socket = -1;

}


void
mb_net_connection_connect(MbNetConnection * self, const gchar * uri,
			  GError ** error)
{


	int sock;
	struct sockaddr_in sock_client;
	struct hostent *src_host;
	Private *priv;

	GError *err;
	err = NULL;
	priv = GET_PRIVATE(self);

	mb_net_connection_set_uri(self, uri, &err);
	if (err != NULL) {
		g_propagate_error(error, err);
		return;
	}
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


	if (sock == -1) {
		perror("socket()");

		g_set_error(error, error_quark,
			    MB_NET_CONNECTION_SOCKET_ERROR,
			    "unable to create socket");
		return;
	}


	bzero((char *) &sock_client, sizeof(sock_client));
	sock_client.sin_family = AF_INET;
	sock_client.sin_port = (unsigned short) htons(priv->port);
	src_host = (struct hostent *) gethostbyname(priv->host);
	if (!src_host) {

		perror("invalid hostname ");
		g_set_error(error, error_quark,
			    MB_NET_CONNECTION_SOCKET_ERROR,
			    "invalide name");
		close(sock);
		return;
	}

	bcopy((char *) src_host->h_addr,
	      (char *) &sock_client.sin_addr.s_addr, src_host->h_length);

	while (connect
	       (sock, (struct sockaddr *) &sock_client,
		sizeof(sock_client)) == -1) {
		if (errno != EAGAIN) {
			perror("error on connect()");

			g_set_error(error, error_quark,
				    MB_NET_CONNECTION_SOCKET_ERROR,
				    "unable to connect socket");
			close(sock);
			return;
		}
	}

	GET_PRIVATE(self)->socket = sock;
}

gint
mb_net_connection_bind(MbNetConnection * self, const gchar * uri,
		       GError ** error)
{
	Private *priv;
	GError *err;
	err = NULL;
	priv = GET_PRIVATE(self);

	struct sockaddr_in sock_client;
	gint sock;

	mb_net_connection_set_uri(self, uri, &err);
	if (err != NULL) {
		g_propagate_error(error, err);
		return -1;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("server socket create error ");

		g_set_error(error, error_quark,
			    MB_NET_CONNECTION_SOCKET_ERROR,
			    "unable to create socket");
		return -1;
	}

	int reuse_addr = 1;
	/* So that we can re-bind to it without TIME_WAIT problems */
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
		   sizeof(reuse_addr));
	bzero((char *) &sock_client, sizeof(sock_client));
	sock_client.sin_family = AF_INET;
	sock_client.sin_addr.s_addr = INADDR_ANY;

	sock_client.sin_port = htons(priv->port);

	if (bind(sock,
		 (struct sockaddr *) &sock_client,
		 sizeof(sock_client)) == -1) {

		close(sock);
		perror("bind server socket error ");
		g_set_error(error, error_quark,
			    MB_NET_CONNECTION_SOCKET_ERROR,
			    "unable to bind socket");
		return -1;
	}


	if (listen(sock, MAX_WAITING_CONN) == -1) {
		close(sock);
		perror("server socket listen() error ");

		g_set_error(error, error_quark,
			    MB_NET_CONNECTION_SOCKET_ERROR,
			    "unable to listen socket");
		return -1;
	}
	return sock;
}

void *_accept_loop(MbNetConnection * self)
{

	Private *priv;
	struct sockaddr_in sock_client;
	int sock;
	guint lg_info;

	priv = GET_PRIVATE(self);

	priv->running = TRUE;

	g_mutex_lock(priv->start_mutex);


	lg_info = sizeof(sock_client);
	g_cond_signal(priv->start_cond);


	g_mutex_unlock(priv->start_mutex);

	while (priv->stop == FALSE && priv->socket != -1) {


		fd_set set;
		struct timeval timeout;	/* Timeout for select */
		timeout.tv_sec = 6;
		timeout.tv_usec = 0;
		int ssock = priv->socket;

		FD_ZERO(&set);
		FD_SET(ssock, &set);
		if ((select(ssock + 1, &set, NULL, NULL, &timeout) >
		     0) && FD_ISSET(ssock, &set)) {
			sock =
			    accept(ssock, (struct sockaddr *) &sock_client,
				   &lg_info);


			if (sock != -1) {
				MbNetConnection *con;
				con =
				    MB_NET_CONNECTION(g_object_new
						      (MB_NET_TYPE_CONNECTION,
						       NULL));
				GET_PRIVATE(con)->socket = sock;
				g_signal_emit(self,
					      _signals[NEW_CONNECTION], 0,
					      con);
				g_object_unref(con);

			} else {
				priv->stop = TRUE;
			}

		}
	}
	priv->main_thread = NULL;
	priv->running = FALSE;
	return 0;

}

void
mb_net_connection_accept_on(MbNetConnection * self, const gchar * uri,
			    GError ** error)
{
	Private *priv;
	GError *err;

	priv = GET_PRIVATE(self);
	err = NULL;

	priv->socket = mb_net_connection_bind(self, uri, &err);
	if (err != NULL) {
		g_propagate_error(error, err);
		return;
	}

	priv->stop = FALSE;

	priv->start_cond = g_cond_new();
	priv->start_mutex = g_mutex_new();

	g_mutex_lock(priv->start_mutex);
	priv->main_thread =
	    g_thread_create((GThreadFunc) _accept_loop, self, TRUE, &err);

	g_cond_wait(priv->start_cond, priv->start_mutex);
	g_mutex_unlock(priv->start_mutex);
	g_mutex_free(priv->start_mutex);
	g_cond_free(priv->start_cond);
	priv->start_cond = NULL;
	priv->start_mutex = NULL;
	if (err != NULL) {
		g_propagate_error(error, err);
		return;
	}

}



void _read_message(MbNetConnection * self)
{
	Private *priv;
	guint32 size;
	priv = GET_PRIVATE(self);

	if (read(priv->socket, &size, sizeof(size)) < 1) {
		return;
	} else {


		size = g_ntohl(size);
		guint32 s = size;
		guint8 *data = g_new0(guint8, size);
		guint p;
		guint readed;
		readed = 1;
		p = 0;

		while (size > 0 && readed > 0) {
			if ((readed =
			     read(priv->socket, data + p, size)) < 1) {
				perror("read()");
				g_free(data);
				return;
			} else {
				p += readed;
				size -= readed;
			}

			g_signal_emit(self, _signals[RECEIVE], 0, s, data);
			MbNetMessage *m =
			    mb_net_message_create_from(data, s);
			g_signal_emit(self, _signals[RECEIVE_MESSAGE], 0,
				      m);
			g_object_unref(m);
			g_free(data);
		}


	}
}

void *_listen_loop(MbNetConnection * self)
{

	Private *priv;

	priv = GET_PRIVATE(self);

	priv->running = TRUE;

	g_mutex_lock(priv->start_mutex);


	g_cond_signal(priv->start_cond);


	g_mutex_unlock(priv->start_mutex);

	while (priv->stop == FALSE && priv->socket != -1) {


		fd_set set;
		struct timeval timeout;	/* Timeout for select */
		timeout.tv_sec = 6;
		timeout.tv_usec = 0;
		int ssock = priv->socket;

		FD_ZERO(&set);
		FD_SET(ssock, &set);
		if ((select(ssock + 1, &set, NULL, NULL, &timeout) >
		     0) && FD_ISSET(ssock, &set)) {
			_read_message(self);
		}
	}

	priv->main_thread = NULL;
	priv->running = FALSE;
	return 0;

}

void mb_net_connection_listen(MbNetConnection * self, GError ** error)
{
	Private *priv;
	GError *err;

	priv = GET_PRIVATE(self);
	err = NULL;

	priv->stop = FALSE;

	priv->start_cond = g_cond_new();
	priv->start_mutex = g_mutex_new();

	g_mutex_lock(priv->start_mutex);
	priv->main_thread =
	    g_thread_create((GThreadFunc) _listen_loop, self, TRUE, &err);

	g_cond_wait(priv->start_cond, priv->start_mutex);
	g_mutex_unlock(priv->start_mutex);
	g_mutex_free(priv->start_mutex);
	g_cond_free(priv->start_cond);
	priv->start_cond = NULL;
	priv->start_mutex = NULL;
	if (err != NULL) {
		g_propagate_error(error, err);
		return;
	}

}


void
mb_net_connection_set_uri(MbNetConnection * self, const gchar * uri,
			  GError ** error)
{
	gchar **str_array;
	Private *priv;
	int i;

	priv = GET_PRIVATE(self);
	if (priv->host != NULL) {
		g_free(priv->host);
		priv->host = NULL;
	}
	str_array = g_strsplit(uri, "://", 2);
	for (i = 0; str_array[i] != NULL; i++) {
	}

	if (i != 2) {
		g_strfreev(str_array);
		g_set_error(error, error_quark, MB_NET_CONNECTION_BAD_URI,
			    "bad URI %s \n", uri);
		return;
	}

	gchar *proto = g_strdup(str_array[0]);
	if (g_str_equal(proto, "mb") == FALSE) {
		g_strfreev(str_array);
		g_set_error(error, error_quark, MB_NET_CONNECTION_BAD_URI,
			    "bad proto %s but as to be mb\n", proto);
		g_free(proto);
		return;
	}

	gchar *host = g_strdup(str_array[1]);
	// proto is not used .. 
	g_free(proto);
	g_strfreev(str_array);

	str_array = g_strsplit(host, ":", -1);

	for (i = 0; str_array[i] != NULL; i++) {
	}

	if (i != 2 && i != 1) {
		g_strfreev(str_array);
		g_set_error(error, error_quark, MB_NET_CONNECTION_BAD_URI,
			    "bad hostname %s \n", host);
		g_free(host);
		return;
	}

	priv->host = g_strdup(str_array[0]);

	if (i == 2) {
		priv->port = g_ascii_strtoull(str_array[1], NULL, 10);
	} else {
		priv->port = 6666;
	}

	g_free(host);

	g_strfreev(str_array);
}


void
mb_net_connection_send_message(MbNetConnection * self,
			       MbNetMessage * message, GError ** error)
{
	_mb_net_connection_send_message(self, mb_net_message_size(message),
					mb_net_message_data(message),
					error);
}

void
_mb_net_connection_send_message(MbNetConnection * self, guint32 s,
				const guint8 * data, GError ** error)
{
	Private *priv;

	priv = GET_PRIVATE(self);

	guint size;
	guint p;
	guint writed;
	p = 0;
	writed = 0;
	size = s;
	guint32 ssize = htonl(s);
	write(priv->socket, &ssize, sizeof(ssize));
	while (size > 0) {
		writed = write(priv->socket, data + p, size);
		if (writed == -1) {
			perror("write()");
			g_set_error(error, error_quark,
				    MB_NET_CONNECTION_SOCKET_ERROR,
				    "write socket error\n");
			return;
		}
		size -= writed;
		p += writed;
	}


}




static void
mb_net_connection_get_property(GObject * object, guint prop_id,
			       GValue * value, GParamSpec * param_spec)
{
	MbNetConnection *self;

	self = MB_NET_CONNECTION(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_connection_set_property(GObject * object, guint prop_id,
			       const GValue * value,
			       GParamSpec * param_spec)
{
	MbNetConnection *self;

	self = MB_NET_CONNECTION(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_connection_class_init(MbNetConnectionClass *
			     mb_net_connection_class)
{
	GObjectClass *g_object_class;

	parent_class = g_type_class_peek_parent(mb_net_connection_class);


	g_type_class_add_private(mb_net_connection_class, sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_connection_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_connection_set_property;
	g_object_class->get_property = mb_net_connection_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_connection_finalize;


	_signals[RECEIVE] = g_signal_new("receive",
					 MB_NET_TYPE_CONNECTION,
					 G_SIGNAL_RUN_LAST,
					 G_STRUCT_OFFSET
					 (MbNetConnectionClass,
					  receive), NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT_POINTER,
					 G_TYPE_NONE, 2,
					 G_TYPE_UINT, G_TYPE_POINTER);

	_signals[RECEIVE_MESSAGE] = g_signal_new("receive-message",
						 MB_NET_TYPE_CONNECTION,
						 G_SIGNAL_RUN_LAST,
						 G_STRUCT_OFFSET
						 (MbNetConnectionClass,
						  receive_message), NULL,
						 NULL,
						 g_cclosure_marshal_VOID__POINTER,
						 G_TYPE_NONE, 1,
						 G_TYPE_POINTER);
	_signals[NEW_CONNECTION] =
	    g_signal_new("new-connection", MB_NET_TYPE_CONNECTION,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetConnectionClass,
					 new_connection), NULL, NULL,
			 g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
			 G_TYPE_POINTER);


	_signals[DISCONNECTED] =
	    g_signal_new("disconnected", MB_NET_TYPE_CONNECTION,
			 G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetConnectionClass,
					 disconnected), NULL, NULL,
			 g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
			 NULL);

	error_quark =
	    g_quark_from_static_string("MB_NET_CONNECTION_ERROR");
}

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* message-handler.h - 
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
#ifndef _NETWORK_MESSAGE_HANDLER_H_
#define _NETWORK_MESSAGE_HANDLER_H_

#include <glib-object.h>
#include "color.h"
#include <libxml/tree.h>

G_BEGIN_DECLS


typedef struct _NetworkMessageHandlerPrivate NetworkMessageHandlerPrivate;

typedef struct _NetworkMessageHandler NetworkMessageHandler;
typedef struct _NetworkMessageHandlerClass NetworkMessageHandlerClass;

GType network_message_handler_get_type(void);
NetworkMessageHandler * network_message_handler_new(int sock);

gboolean network_message_handler_connect(NetworkMessageHandler * handler,const gchar * host,int port);
void network_message_handler_join(NetworkMessageHandler * mnh);
void network_message_handler_start_listening(NetworkMessageHandler * mmh);

void network_message_handler_disconnect(NetworkMessageHandler * mmh);

void network_message_handler_send_message (NetworkMessageHandler * mmh,
					  guint32 client_id,
					  const gchar * message,
					  guint size);

void network_message_handler_send_waiting_added (NetworkMessageHandler * mmh,
                                                guint32 monkey_id,
                                                 guint32 time,
                                                guint8 bubbles_count);


void network_message_handler_send_add_row(NetworkMessageHandler * mmh,
                                                guint32 monkey_id,
                                                Color * bubbles);

void network_message_handler_send_add_bubble  (NetworkMessageHandler * mmh,
					      guint32 monkey_id,
					      Color color);

void network_message_handler_send_shoot       (NetworkMessageHandler * mmh,
					      guint32 monkey_id,
					      guint32 time,
					      gfloat angle);

void network_message_handler_send_bubble_array( NetworkMessageHandler * mmh,
                                               guint32 monkey_id,
                                               guint8  bubbles_count,
                                                Color * bubbles,
                                                guint32 odd);


void network_message_handler_send_next_range( NetworkMessageHandler * mmh,
                                              guint32 monkey_id,
                                              Color * bubbles);

void network_message_handler_send_winlost     (NetworkMessageHandler * mmh,
                                               guint32 monkey_id,
                                               guint8 win_lost);

void network_message_handler_send_start       (NetworkMessageHandler * mmh);

void network_message_handler_send_xml_message(NetworkMessageHandler * mmh,
                                              guint32 client_id,
                                              xmlDoc * doc);



#define NETWORK_TYPE_MESSAGE_HANDLER   (network_message_handler_get_type())

#define NETWORK_MESSAGE_HANDLER(object)(G_TYPE_CHECK_INSTANCE_CAST((object),NETWORK_TYPE_MESSAGE_HANDLER,NetworkMessageHandler))
#define NETWORK_MESSAGE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NETWORK_TYPE_MESSAGE_HANDLER,NetworkMessageHandlerClass))
#define IS_NETWORK_MESSAGE_HANDLER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), NETWORK_TYPE_MESSAGE_HANDLER))
#define IS_NETWORK_MESSAGE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NETWORK_TYPE_MESSAGE_HANDLER))
#define NETWORK_MESSAGE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NETWORK_TYPE_MESSAGE_HANDLER, NetworkMessageHandlerClass))


struct _NetworkMessageHandler
{
        GObject parent_instance;
        NetworkMessageHandlerPrivate * private;
};

struct _NetworkMessageHandlerClass
{
        GObjectClass parent_class;
        void (* recv_shoot)         (NetworkMessageHandler * mmh,
                                     guint32 monkey_id,
                                     guint32 time,
                                     gfloat angle);

        void (* recv_add_bubble)    (NetworkMessageHandler * mmh,
                                     guint32 monkey_id,
                                     Color color);

        // i need a good name ..
        void (* recv_next_range)    (NetworkMessageHandler * mmh,
                                     guint32 monkey_id,
                                     Color * line);

        void (* recv_waiting_added) (NetworkMessageHandler * mmh,
                                     guint32 monkey_id,
                                     guint32 time,
                                     guint32 bubbles_count);


        void (* recv_add_row) (NetworkMessageHandler * mmh,
                                     guint32 monkey_id,
                                     Color * line);

        void (* recv_winlost)       (NetworkMessageHandler * mmh,
                                     guint32 monkey_id,
                                     guint8 win_lost);

        void (* recv_start)         (NetworkMessageHandler * mmh);

        void (*recv_bubble_array) (NetworkMessageHandler * mmh,
                                   guint32 monkey_id,
                                   guint32 bubble_count,
                                   Color * bubbles);

        void (* recv_message)          (NetworkMessageHandler * mmh,
                                        guint32 client_id,
                                        gchar * message);

        void (* recv_xml_message)          (NetworkMessageHandler * mmh,
                                            guint32 client_id,
                                            xmlDoc * doc);

        void (* connection_closed)         (NetworkMessageHandler * mmh);

};


G_END_DECLS

#endif

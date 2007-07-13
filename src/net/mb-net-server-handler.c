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

#include "mb-net-server-handler.h"

#include <glib.h>
#include <glib-object.h>

#include <net/monkey-net-marshal.h>

typedef struct _Private
{
  int i;
} Private;



enum
{
  PROP_ATTRIBUTE
};

enum
{
  ASK_GAME_LIST,
  GAME_LIST,
  ASK_REGISTER_CLIENT,
  REGISTER_CLIENT_RESPONSE,
  N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_server_handler_get_property (GObject * object,
						guint prop_id,
						GValue * value,
						GParamSpec * param_spec);
static void mb_net_server_handler_set_property (GObject * object,
						guint prop_id,
						const GValue * value,
						GParamSpec * param_spec);

static void mb_net_server_handler_iface_init (MbNetHandlerInterface * iface);
static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE (MbNetServerHandler, mb_net_server_handler,
			 MB_NET_TYPE_ABSTRACT_HANDLER,
			 {
			 G_IMPLEMENT_INTERFACE (MB_NET_TYPE_HANDLER,
						mb_net_server_handler_iface_init)});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_SERVER_HANDLER, Private))




static void mb_net_server_handler_finalize (MbNetServerHandler * self);

static void mb_net_server_handler_init (MbNetServerHandler * self);
static void _receive (MbNetHandler * handler, MbNetConnection * con,
		      MbNetMessage * m);



static void
mb_net_server_handler_init (MbNetServerHandler * self)
{
  Private *priv;
  priv = GET_PRIVATE (self);
}


static void
mb_net_server_handler_finalize (MbNetServerHandler * self)
{
  Private *priv;
  priv = GET_PRIVATE (self);

  // finalize super
  if (G_OBJECT_CLASS (parent_class)->finalize)
    {
      (*G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (self));
    }
}

void
mb_net_server_handler_send_game_list (MbNetServerHandler * self,
				      MbNetConnection * con,
				      guint32 tohandler_id, GList * games)
{
//      for();
}

static void
_receive (MbNetHandler * handler, MbNetConnection * con, MbNetMessage * m)
{
  MbNetServerHandler *self;
  Private *priv;
  self = MB_NET_SERVER_HANDLER (handler);
  priv = GET_PRIVATE (self);

  guint32 code = mb_net_message_read_int (m);

  if (code == ASK_GAME_LIST)
    {
      guint32 id = mb_net_message_read_int (m);
      g_signal_emit (self, _signals[ASK_GAME_LIST], 0, con, id);
    }
  else if (code == GAME_LIST)
    {
      guint32 id = mb_net_message_read_int (m);
      g_print ("id %d \n", id);
    }				/*else if ( code == REGISTER_CLIENT ) {
				   guint32 id =
				   mb_netabstract_handler_read_int(message, size,
				   sizeof(guint32));
				   } */
}

void
mb_net_server_handler_send_ask_game_list (MbNetServerHandler * self,
					  MbNetConnection * con,
					  guint32 handler_id)
{
  MbNetMessage *m = mb_net_message_create ();
  mb_net_message_add_int (m, handler_id);
  mb_net_message_add_int (m, ASK_GAME_LIST);

  mb_net_connection_send_message (con, m, NULL);
  g_object_unref (m);
}

static void
mb_net_server_handler_get_property (GObject * object, guint prop_id,
				    GValue * value, GParamSpec * param_spec)
{
  MbNetServerHandler *self;

  self = MB_NET_SERVER_HANDLER (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, param_spec);
      break;
    }
}

static void
mb_net_server_handler_set_property (GObject * object, guint prop_id,
				    const GValue * value,
				    GParamSpec * param_spec)
{
  MbNetServerHandler *self;

  self = MB_NET_SERVER_HANDLER (object);

  switch (prop_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, param_spec);
      break;
    }
}


static void
mb_net_server_handler_iface_init (MbNetHandlerInterface * iface)
{
  iface->receive = _receive;
}


static void
mb_net_server_handler_class_init (MbNetServerHandlerClass *
				  mb_net_server_handler_class)
{
  GObjectClass *g_object_class;

  parent_class = g_type_class_peek_parent (mb_net_server_handler_class);


  g_type_class_add_private (mb_net_server_handler_class, sizeof (Private));

  g_object_class = G_OBJECT_CLASS (mb_net_server_handler_class);

  /* setting up property system */
  g_object_class->set_property = mb_net_server_handler_set_property;
  g_object_class->get_property = mb_net_server_handler_get_property;
  g_object_class->finalize =
    (GObjectFinalizeFunc) mb_net_server_handler_finalize;

  _signals[ASK_GAME_LIST] = g_signal_new ("ask-game-list",
					  MB_NET_TYPE_CONNECTION,
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET
					  (MbNetServerHandlerClass,
					   ask_game_list), NULL,
					  NULL,
					  monkey_net_marshal_VOID__POINTER_UINT,
					  G_TYPE_NONE, 2,
					  G_TYPE_POINTER, G_TYPE_UINT);

  _signals[GAME_LIST] = g_signal_new ("game-list",
				      MB_NET_TYPE_CONNECTION,
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET
				      (MbNetServerHandlerClass,
				       ask_game_list), NULL,
				      NULL,
				      monkey_net_marshal_VOID__POINTER_UINT_POINTER,
				      G_TYPE_NONE, 3,
				      G_TYPE_POINTER,
				      G_TYPE_UINT, G_TYPE_POINTER);
}

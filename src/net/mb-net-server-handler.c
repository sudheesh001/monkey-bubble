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
#include <net/mb-net-holders.h>
#include <glib.h>
#include <glib-object.h>

#include <net/monkey-net-marshal.h>

typedef struct _Private {
	int i;
} Private;



enum {
	PROP_ATTRIBUTE
};

enum {
	GAME_LIST,
	ASK_GAME_LIST,
	ASK_REGISTER_PLAYER,
	REGISTER_PLAYER_RESPONSE,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;

static void mb_net_server_handler_get_property(GObject * object,
					       guint prop_id,
					       GValue * value,
					       GParamSpec * param_spec);
static void mb_net_server_handler_set_property(GObject * object,
					       guint prop_id,
					       const GValue * value,
					       GParamSpec * param_spec);

static void mb_net_server_handler_iface_init(MbNetHandlerInterface *
					     iface);
static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetServerHandler, mb_net_server_handler,
			MB_NET_TYPE_ABSTRACT_HANDLER, {
			G_IMPLEMENT_INTERFACE(MB_NET_TYPE_HANDLER,
					      mb_net_server_handler_iface_init)});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_SERVER_HANDLER, Private))




static void mb_net_server_handler_finalize(MbNetServerHandler * self);

static void mb_net_server_handler_init(MbNetServerHandler * self);
static void _receive(MbNetHandler * handler, MbNetConnection * con,
		     guint32 src_id, guint32 dest_id, guint32 action_id,
		     MbNetMessage * m);



static void mb_net_server_handler_init(MbNetServerHandler * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
}


static void mb_net_server_handler_finalize(MbNetServerHandler * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);

	// finalize super
	if (G_OBJECT_CLASS(parent_class)->finalize) {
		(*G_OBJECT_CLASS(parent_class)->finalize) (G_OBJECT(self));
	}
}

static void
_receive(MbNetHandler * handler, MbNetConnection * con, guint32 src_id,
	 guint32 dest_id, guint32 action_id, MbNetMessage * m)
{
	MbNetServerHandler *self;
	Private *priv;
	self = MB_NET_SERVER_HANDLER(handler);
	priv = GET_PRIVATE(self);

	guint32 code = action_id;
	if (code == ASK_GAME_LIST) {
		g_signal_emit(self, _signals[ASK_GAME_LIST], 0, con,
			      src_id);
	} else if (code == GAME_LIST) {
		MbNetGameListHolder *holder =
		    mb_net_game_list_holder_parse(m);
		g_signal_emit(self, _signals[GAME_LIST], 0, con, src_id,
			      holder);
		mb_net_game_list_holder_free(holder);
	} else if (code == ASK_REGISTER_PLAYER) {
		MbNetPlayerHolder *holder = mb_net_player_holder_parse(m);
		g_signal_emit(self, _signals[ASK_REGISTER_PLAYER], 0, con,
			      src_id, holder);
		mb_net_player_holder_free(holder);
	} else if (code == REGISTER_PLAYER_RESPONSE) {
		MbNetPlayerHolder *holder = mb_net_player_holder_parse(m);
		guint ok = mb_net_message_read_boolean(m);
		g_signal_emit(self, _signals[REGISTER_PLAYER_RESPONSE], 0,
			      con, src_id, holder, ok);
		mb_net_player_holder_free(holder);
	}
}

static guint32 _get_id(MbNetServerHandler * self)
{
	return mb_net_handler_get_id(MB_NET_HANDLER(self));
}

void mb_net_server_handler_send_game_list
    (MbNetServerHandler * self,
     MbNetConnection * con, guint32 handler_id,
     MbNetGameListHolder * holder) {
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, GAME_LIST);
	mb_net_game_list_holder_serialize(holder, m);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);

}

void mb_net_server_handler_send_ask_game_list
    (MbNetServerHandler * self,
     MbNetConnection * con, guint32 handler_id) {
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, ASK_GAME_LIST);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}


void mb_net_server_handler_send_ask_register_player
    (MbNetServerHandler * self, MbNetConnection * con, guint32 handler_id,
     MbNetPlayerHolder * holder) {
	MbNetMessage *m = mb_net_message_new(_get_id(self), handler_id,
					     ASK_REGISTER_PLAYER);
	mb_net_player_holder_serialize(holder, m);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}


void mb_net_server_handler_send_register_player_response
    (MbNetServerHandler * self, MbNetConnection * con, guint32 handler_id,
     MbNetPlayerHolder * holder, gboolean ok) {
	MbNetMessage *m = mb_net_message_new(_get_id(self), handler_id,
					     REGISTER_PLAYER_RESPONSE);
	mb_net_player_holder_serialize(holder, m);
	mb_net_message_add_boolean(m, ok);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

static void
mb_net_server_handler_get_property(GObject * object, guint prop_id,
				   GValue * value, GParamSpec * param_spec)
{
	MbNetServerHandler *self;

	self = MB_NET_SERVER_HANDLER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_server_handler_set_property(GObject * object, guint prop_id,
				   const GValue * value,
				   GParamSpec * param_spec)
{
	MbNetServerHandler *self;

	self = MB_NET_SERVER_HANDLER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}


static void mb_net_server_handler_iface_init(MbNetHandlerInterface * iface)
{
	iface->receive = _receive;
}


static void
mb_net_server_handler_class_init(MbNetServerHandlerClass *
				 mb_net_server_handler_class)
{
	GObjectClass *g_object_class;

	parent_class =
	    g_type_class_peek_parent(mb_net_server_handler_class);


	g_type_class_add_private(mb_net_server_handler_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_server_handler_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_server_handler_set_property;
	g_object_class->get_property = mb_net_server_handler_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_server_handler_finalize;

	_signals[ASK_GAME_LIST] = g_signal_new("ask-game-list",
					       MB_NET_TYPE_SERVER_HANDLER,
					       G_SIGNAL_RUN_LAST,
					       G_STRUCT_OFFSET
					       (MbNetServerHandlerClass,
						ask_game_list), NULL,
					       NULL,
					       monkey_net_marshal_VOID__POINTER_UINT,
					       G_TYPE_NONE, 2,
					       G_TYPE_POINTER,
					       G_TYPE_UINT);

	_signals[GAME_LIST] = g_signal_new("game-list",
					   MB_NET_TYPE_SERVER_HANDLER,
					   G_SIGNAL_RUN_LAST,
					   G_STRUCT_OFFSET
					   (MbNetServerHandlerClass,
					    game_list), NULL,
					   NULL,
					   monkey_net_marshal_VOID__POINTER_UINT_POINTER,
					   G_TYPE_NONE, 3,
					   G_TYPE_POINTER, G_TYPE_UINT,
					   G_TYPE_POINTER);

	_signals[ASK_REGISTER_PLAYER] = g_signal_new("ask-register-player",
						     MB_NET_TYPE_SERVER_HANDLER,
						     G_SIGNAL_RUN_LAST,
						     G_STRUCT_OFFSET
						     (MbNetServerHandlerClass,
						      ask_register_player),
						     NULL, NULL,
						     monkey_net_marshal_VOID__POINTER_UINT_POINTER,
						     G_TYPE_NONE, 3,
						     G_TYPE_POINTER,
						     G_TYPE_UINT,
						     G_TYPE_POINTER);

	_signals[REGISTER_PLAYER_RESPONSE] =
	    g_signal_new("register-player-response",
			 MB_NET_TYPE_SERVER_HANDLER, G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetServerHandlerClass,
					 register_player_response), NULL,
			 NULL,
			 monkey_net_marshal_VOID__POINTER_UINT_POINTER_UINT,
			 G_TYPE_NONE, 4, G_TYPE_POINTER, G_TYPE_UINT,
			 G_TYPE_POINTER, G_TYPE_UINT);

}

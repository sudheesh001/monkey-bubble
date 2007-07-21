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

#include "mb-net-match-handler.h"

#include <glib.h>
#include <glib-object.h>
#include <net/monkey-net-marshal.h>
#include <net/mb-net-abstract-handler.h>
typedef struct _Private {
	int i;
} Private;




enum {
	PROP_ATTRIBUTE
};

enum {
	PENALITY,
	PENALITY_BUBBLES,
	NEXT_ROW,
	NEW_CANNON_BUBBLE,
	SHOOT,
	MATCH_INIT,
	WINLOST,
	READY,
	START,
	OBSERVER_PLAYER_BUBBLES,
	OBSERVER_PLAYER_WINLOST,
	N_SIGNALS
};

static GObjectClass *parent_class = NULL;


void _parse_penality(MbNetMatchHandler * self, MbNetConnection * con,
		     guint32 handler_id, MbNetMessage * m);
void _parse_penality_bubbles(MbNetMatchHandler * self,
			     MbNetConnection * con, guint32 handler_id,
			     MbNetMessage * m);
void _parse_next_row(MbNetMatchHandler * self, MbNetConnection * con,
		     guint32 handler_id, MbNetMessage * m);
void _parse_shoot(MbNetMatchHandler * self, MbNetConnection * con,
		  guint32 handler_id, MbNetMessage * m);
void _parse_match_init(MbNetMatchHandler * self, MbNetConnection * con,
		       guint32 handler_id, MbNetMessage * m);

static void _parse_observer_player_bubbles(MbNetMatchHandler * self,
					   MbNetConnection * con,
					   guint32 handler_id,
					   MbNetMessage * m);

static void mb_net_match_handler_get_property(GObject * object,
					      guint prop_id,
					      GValue * value,
					      GParamSpec * param_spec);
static void mb_net_match_handler_set_property(GObject * object,
					      guint prop_id,
					      const GValue * value,
					      GParamSpec * param_spec);


static void mb_net_match_handler_iface_init(MbNetHandlerInterface * iface);
static guint _signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_CODE(MbNetMatchHandler, mb_net_match_handler,
			MB_NET_TYPE_ABSTRACT_HANDLER, {
			G_IMPLEMENT_INTERFACE(MB_NET_TYPE_HANDLER,
					      mb_net_match_handler_iface_init)});

#define GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), MB_NET_TYPE_MATCH_HANDLER, Private))




static void mb_net_match_handler_finalize(MbNetMatchHandler * self);

static void mb_net_match_handler_init(MbNetMatchHandler * self);



static void mb_net_match_handler_init(MbNetMatchHandler * self)
{
	Private *priv;
	priv = GET_PRIVATE(self);
}


static void mb_net_match_handler_finalize(MbNetMatchHandler * self)
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
	MbNetMatchHandler *self;
	Private *priv;
	self = MB_NET_MATCH_HANDLER(handler);
	priv = GET_PRIVATE(self);

	guint32 code = action_id;

	switch (code) {
	case PENALITY:
		_parse_penality(self, con, src_id, m);
		break;
	case PENALITY_BUBBLES:
		_parse_penality_bubbles(self, con, src_id, m);
		break;
	case NEXT_ROW:
		_parse_next_row(self, con, src_id, m);
		break;
	case NEW_CANNON_BUBBLE:{
			int color = mb_net_message_read_int(m);
			g_signal_emit(self, _signals[NEW_CANNON_BUBBLE], 0,
				      con, src_id, color);
			break;
		}

	case SHOOT:
		_parse_shoot(self, con, src_id, m);
		break;
	case WINLOST:{
			gboolean win = mb_net_message_read_boolean(m);
			g_signal_emit(self, _signals[WINLOST], 0, con,
				      src_id, win);
			break;
		}
	case MATCH_INIT:
		_parse_match_init(self, con, src_id, m);
		break;
	case OBSERVER_PLAYER_BUBBLES:
		_parse_observer_player_bubbles(self, con, src_id, m);
		break;

	case READY:{
			guint32 player_id = mb_net_message_read_int(m);
			g_signal_emit(self, _signals[READY], 0, con,
				      src_id, player_id);
			break;
		}
	case START:
		g_signal_emit(self, _signals[START], 0, con, src_id);
		break;
	}

}


static guint32 _get_id(MbNetMatchHandler * self)
{
	return mb_net_handler_get_id(MB_NET_HANDLER(self));
}

void mb_net_match_handler_send_penality(MbNetMatchHandler * self,
					MbNetConnection * con,
					guint32 handler_id, guint32 time,
					guint32 penality)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, PENALITY);
	mb_net_message_add_int(m, time);
	mb_net_message_add_int(m, penality);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}


void _parse_penality(MbNetMatchHandler * self, MbNetConnection * con,
		     guint32 handler_id, MbNetMessage * m)
{
	guint32 time = mb_net_message_read_int(m);
	guint32 penality = mb_net_message_read_int(m);
	g_signal_emit(self, _signals[PENALITY], 0, con, handler_id, time,
		      penality);
}


void mb_net_match_handler_send_next_row(MbNetMatchHandler * self,
					MbNetConnection * con,
					guint32 handler_id,
					Color * bubbles)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, NEXT_ROW);
	int i = 0;
	for (i = 0; i < 8; i++) {
		mb_net_message_add_int(m, bubbles[i]);
	}
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void _parse_next_row(MbNetMatchHandler * self, MbNetConnection * con,
		     guint32 handler_id, MbNetMessage * m)
{
	Color *bubbles = g_new0(Color, 8);
	int i = 0;
	for (i = 0; i < 8; i++) {
		bubbles[i] = mb_net_message_read_int(m);
	}

	g_signal_emit(self, _signals[NEXT_ROW], 0, con, handler_id,
		      bubbles);
}

void mb_net_match_handler_send_new_cannon_bubble(MbNetMatchHandler * self,
						 MbNetConnection * con,
						 guint32 handler_id,
						 Color color)
{
	MbNetMessage *m = mb_net_message_new(_get_id(self), handler_id,
					     NEW_CANNON_BUBBLE);
	mb_net_message_add_int(m, color);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_match_handler_send_shoot(MbNetMatchHandler * self,
				     MbNetConnection * con,
				     guint32 handler_id, guint32 time,
				     gfloat radian)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, SHOOT);
	mb_net_message_add_int(m, time);
	int i = (gint32) (radian * 65536.0);
	mb_net_message_add_int(m, i);

	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void _parse_shoot(MbNetMatchHandler * self, MbNetConnection * con,
		  guint32 handler_id, MbNetMessage * m)
{
	guint32 time = mb_net_message_read_int(m);
	gint32 radian = mb_net_message_read_int(m);
	gfloat r = radian;
	r = r / 65536.0;
	g_signal_emit(self, _signals[SHOOT], 0, con, handler_id, time, r);
}


void mb_net_match_handler_send_match_init(MbNetMatchHandler * self,
					  MbNetConnection * con,
					  guint32 handler_id,
					  guint32 count, Color * bubbles,
					  gboolean odd, Color bubble1,
					  Color bubble2)
{

	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, MATCH_INIT);
	mb_net_message_add_int(m, count);

	int i = 0;
	for (i = 0; i < count; i++) {
		mb_net_message_add_int(m, bubbles[i]);
	}
	mb_net_message_add_boolean(m, odd);
	mb_net_message_add_int(m, bubble1);
	mb_net_message_add_int(m, bubble2);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void _parse_match_init(MbNetMatchHandler * self, MbNetConnection * con,
		       guint32 handler_id, MbNetMessage * m)
{
	guint size = mb_net_message_read_int(m);
	Color *bubbles = g_new0(Color, size);
	int i = 0;
	for (i = 0; i < size; i++) {
		bubbles[i] = mb_net_message_read_int(m);
	}
	gboolean odd = mb_net_message_read_boolean(m);
	Color bubble1 = mb_net_message_read_int(m);
	Color bubble2 = mb_net_message_read_int(m);
	g_signal_emit(self, _signals[MATCH_INIT], 0, con, handler_id, size,
		      bubbles, odd, bubble1, bubble2);

}

void mb_net_match_handler_send_observer_player_bubbles(MbNetMatchHandler *
						       self,
						       MbNetConnection *
						       con,
						       guint32 handler_id,
						       guint32 player_id,
						       guint32 count,
						       Color * bubbles,
						       gboolean odd)
{

	MbNetMessage *m = mb_net_message_new(_get_id(self), handler_id,
					     OBSERVER_PLAYER_BUBBLES);

	mb_net_message_add_int(m, player_id);
	mb_net_message_add_int(m, count);

	int i = 0;
	for (i = 0; i < count; i++) {
		mb_net_message_add_int(m, bubbles[i]);
	}
	mb_net_message_add_boolean(m, odd);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void _parse_observer_player_bubbles(MbNetMatchHandler * self,
				    MbNetConnection * con,
				    guint32 handler_id, MbNetMessage * m)
{
	guint player_id = mb_net_message_read_int(m);
	guint size = mb_net_message_read_int(m);
	Color *bubbles = g_new0(Color, size);
	int i = 0;
	for (i = 0; i < size; i++) {
		bubbles[i] = mb_net_message_read_int(m);
	}

	gboolean odd = mb_net_message_read_boolean(m);
	g_signal_emit(self, _signals[OBSERVER_PLAYER_BUBBLES], 0,
		      player_id, size, bubbles, odd);

}

void mb_net_match_handler_send_observer_player_winlost(MbNetMatchHandler *
						       self,
						       MbNetConnection *
						       con,
						       guint32 handler_id,
						       guint32 player_id,
						       gboolean winlost)
{
	MbNetMessage *m = mb_net_message_new(_get_id(self), handler_id,
					     OBSERVER_PLAYER_WINLOST);

	mb_net_message_add_int(m, player_id);
	mb_net_message_add_boolean(m, winlost);

	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void _parse_observer_player_winlost(MbNetMatchHandler * self,
				    MbNetConnection * con,
				    guint32 handler_id, MbNetMessage * m)
{
	guint player_id = mb_net_message_read_int(m);
	gboolean winlost = mb_net_message_read_boolean(m);
	g_signal_emit(self, _signals[OBSERVER_PLAYER_WINLOST], 0,
		      player_id, winlost);

}

void mb_net_match_handler_send_penality_bubbles(MbNetMatchHandler * self,
						MbNetConnection * con,
						guint32 handler_id,
						Color * bubbles)
{
	MbNetMessage *m = mb_net_message_new(_get_id(self), handler_id,
					     PENALITY_BUBBLES);
	int i = 0;
	for (i = 0; i < 7; i++) {
		mb_net_message_add_int(m, bubbles[i]);
	}
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void _parse_penality_bubbles(MbNetMatchHandler * self,
			     MbNetConnection * con, guint32 handler_id,
			     MbNetMessage * m)
{
	Color *bubbles = g_new0(Color, 8);
	int i = 0;
	for (i = 0; i < 7; i++) {
		bubbles[i] = mb_net_message_read_int(m);
	}

	g_signal_emit(self, _signals[PENALITY_BUBBLES], 0, con, handler_id,
		      bubbles);
}

void mb_net_match_handler_send_winlost(MbNetMatchHandler * self,
				       MbNetConnection * con,
				       guint32 handler_id, gboolean win)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, WINLOST);

	mb_net_message_add_boolean(m, win);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_match_handler_send_ready(MbNetMatchHandler * self,
				     MbNetConnection * con,
				     guint32 handler_id, guint32 player_id)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, READY);
	mb_net_message_add_int(m, player_id);
	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);
}

void mb_net_match_handler_send_start(MbNetMatchHandler * self,
				     MbNetConnection * con,
				     guint32 handler_id)
{
	MbNetMessage *m =
	    mb_net_message_new(_get_id(self), handler_id, START);

	mb_net_connection_send_message(con, m, NULL);
	g_object_unref(m);

}


static void
mb_net_match_handler_get_property(GObject * object, guint prop_id,
				  GValue * value, GParamSpec * param_spec)
{
	MbNetMatchHandler *self;

	self = MB_NET_MATCH_HANDLER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}

static void
mb_net_match_handler_set_property(GObject * object, guint prop_id,
				  const GValue * value,
				  GParamSpec * param_spec)
{
	MbNetMatchHandler *self;

	self = MB_NET_MATCH_HANDLER(object);

	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id,
						  param_spec);
		break;
	}
}


static void mb_net_match_handler_iface_init(MbNetHandlerInterface * iface)
{
	iface->receive = _receive;
}


static void
mb_net_match_handler_class_init(MbNetMatchHandlerClass *
				mb_net_match_handler_class)
{
	GObjectClass *g_object_class;

	parent_class =
	    g_type_class_peek_parent(mb_net_match_handler_class);


	g_type_class_add_private(mb_net_match_handler_class,
				 sizeof(Private));

	g_object_class = G_OBJECT_CLASS(mb_net_match_handler_class);

	/* setting up property system */
	g_object_class->set_property = mb_net_match_handler_set_property;
	g_object_class->get_property = mb_net_match_handler_get_property;
	g_object_class->finalize =
	    (GObjectFinalizeFunc) mb_net_match_handler_finalize;


	_signals[PENALITY] = g_signal_new("penality",
					  MB_NET_TYPE_MATCH_HANDLER,
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET
					  (MbNetMatchHandlerClass,
					   penality),
					  NULL, NULL,
					  monkey_net_marshal_VOID__POINTER_UINT_UINT_UINT,
					  G_TYPE_NONE, 4,
					  G_TYPE_POINTER,
					  G_TYPE_UINT, G_TYPE_UINT,
					  G_TYPE_UINT);

	_signals[PENALITY_BUBBLES] = g_signal_new("penality-bubbles",
						  MB_NET_TYPE_MATCH_HANDLER,
						  G_SIGNAL_RUN_LAST,
						  G_STRUCT_OFFSET
						  (MbNetMatchHandlerClass,
						   penality_bubbles),
						  NULL, NULL,
						  monkey_net_marshal_VOID__POINTER_UINT_POINTER,
						  G_TYPE_NONE, 3,
						  G_TYPE_POINTER,
						  G_TYPE_UINT,
						  G_TYPE_POINTER);

	_signals[NEXT_ROW] = g_signal_new("next-row",
					  MB_NET_TYPE_MATCH_HANDLER,
					  G_SIGNAL_RUN_LAST,
					  G_STRUCT_OFFSET
					  (MbNetMatchHandlerClass,
					   next_row),
					  NULL, NULL,
					  monkey_net_marshal_VOID__POINTER_UINT_POINTER,
					  G_TYPE_NONE, 3,
					  G_TYPE_POINTER,
					  G_TYPE_UINT, G_TYPE_POINTER);
	_signals[NEW_CANNON_BUBBLE] = g_signal_new("new-cannon-bubble",
						   MB_NET_TYPE_MATCH_HANDLER,
						   G_SIGNAL_RUN_LAST,
						   G_STRUCT_OFFSET
						   (MbNetMatchHandlerClass,
						    new_cannon_bubble),
						   NULL, NULL,
						   monkey_net_marshal_VOID__POINTER_UINT_UINT,
						   G_TYPE_NONE, 3,
						   G_TYPE_POINTER,
						   G_TYPE_UINT,
						   G_TYPE_UINT);

	_signals[SHOOT] = g_signal_new("shoot",
				       MB_NET_TYPE_MATCH_HANDLER,
				       G_SIGNAL_RUN_LAST,
				       G_STRUCT_OFFSET
				       (MbNetMatchHandlerClass,
					shoot),
				       NULL, NULL,
				       monkey_net_marshal_VOID__POINTER_UINT_UINT_FLOAT,
				       G_TYPE_NONE, 4,
				       G_TYPE_POINTER,
				       G_TYPE_UINT, G_TYPE_UINT,
				       G_TYPE_FLOAT);


	_signals[MATCH_INIT] = g_signal_new("match-init",
					    MB_NET_TYPE_MATCH_HANDLER,
					    G_SIGNAL_RUN_LAST,
					    G_STRUCT_OFFSET
					    (MbNetMatchHandlerClass,
					     match_init),
					    NULL, NULL,
					    monkey_net_marshal_VOID__POINTER_UINT_UINT_POINTER_UINT_UINT_UINT,
					    G_TYPE_NONE, 7,
					    G_TYPE_POINTER,
					    G_TYPE_UINT, G_TYPE_UINT,
					    G_TYPE_POINTER, G_TYPE_UINT,
					    G_TYPE_UINT, G_TYPE_UINT);

	_signals[WINLOST] = g_signal_new("winlost",
					 MB_NET_TYPE_MATCH_HANDLER,
					 G_SIGNAL_RUN_LAST,
					 G_STRUCT_OFFSET
					 (MbNetMatchHandlerClass,
					  winlost),
					 NULL, NULL,
					 monkey_net_marshal_VOID__POINTER_UINT_UINT,
					 G_TYPE_NONE, 3,
					 G_TYPE_POINTER,
					 G_TYPE_UINT, G_TYPE_UINT);

	_signals[READY] = g_signal_new("ready",
				       MB_NET_TYPE_MATCH_HANDLER,
				       G_SIGNAL_RUN_LAST,
				       G_STRUCT_OFFSET
				       (MbNetMatchHandlerClass,
					ready),
				       NULL, NULL,
				       monkey_net_marshal_VOID__POINTER_UINT_UINT,
				       G_TYPE_NONE, 3,
				       G_TYPE_POINTER, G_TYPE_UINT,
				       G_TYPE_UINT);

	_signals[START] = g_signal_new("start",
				       MB_NET_TYPE_MATCH_HANDLER,
				       G_SIGNAL_RUN_LAST,
				       G_STRUCT_OFFSET
				       (MbNetMatchHandlerClass,
					start),
				       NULL, NULL,
				       monkey_net_marshal_VOID__POINTER_UINT,
				       G_TYPE_NONE, 2,
				       G_TYPE_POINTER, G_TYPE_UINT);

	_signals[OBSERVER_PLAYER_BUBBLES] =
	    g_signal_new("observer-player-bubbles",
			 MB_NET_TYPE_MATCH_HANDLER, G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetMatchHandlerClass,
					 observer_player_bubbles), NULL,
			 NULL,
			 monkey_net_marshal_VOID__UINT_UINT_POINTER_UINT,
			 G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_UINT,
			 G_TYPE_POINTER, G_TYPE_UINT);

	_signals[OBSERVER_PLAYER_WINLOST] =
	    g_signal_new("observer-player-winlost",
			 MB_NET_TYPE_MATCH_HANDLER, G_SIGNAL_RUN_LAST,
			 G_STRUCT_OFFSET(MbNetMatchHandlerClass,
					 observer_player_bubbles), NULL,
			 NULL,
			 monkey_net_marshal_VOID__UINT_UINT,
			 G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);

}

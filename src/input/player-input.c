/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* this file is part of criawips a gnome presentation application
 *
 * AUTHORS
 *       Laurent Belmonte        <laurent.belmonte@aliacom.fr>
 *
 * Copyright (C) 2004 Laurent Belmonte
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

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include "player-input.h"

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;


enum {
	INPUT_NOTIFY_PRESSED,
	INPUT_NOTIFY_RELEASED,
	LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];


struct MbPlayerInputPrivate 
{
	gint notify_id;
	GConfClient * gconf_client;
	gchar * left_key_name;
	gchar * right_key_name;
	gchar * shoot_key_name;


	guint left_key;
	GdkModifierType left_key_modifier;


	guint right_key;
	GdkModifierType right_key_modifier;

	guint shoot_key;
	GdkModifierType shoot_key_modifier;
	
};


static gboolean key_released(GtkWidget *widget,
			     GdkEventKey *event,
			     gpointer user_data);

static gboolean key_pressed(GtkWidget *widget,
			    GdkEventKey *event,
			    gpointer user_data);

static void client_notified(GConfClient *client,
			    guint cnxn_id,
			    GConfEntry *entry,
			    gpointer user_data);

static void conf_keyboard(MbPlayerInput * self);

MbPlayerInput *
mb_player_input_new  (GtkWidget * window,
		      GConfClient * client,
		      const gchar * conf_key_prefix,
		      const gchar * player_name)
{
	MbPlayerInput * self;

	self = MB_PLAYER_INPUT( g_object_new(MB_TYPE_PLAYER_INPUT,NULL) );

	PRIVATE(self)->gconf_client = client;

	PRIVATE(self)->notify_id =
		gconf_client_notify_add( PRIVATE(self)->gconf_client,
					 conf_key_prefix,
					 client_notified,
					 self,NULL,NULL);

	PRIVATE(self)->left_key_name =
		g_strdup_printf("%s/%s_left",conf_key_prefix,
				 player_name);

	PRIVATE(self)->right_key_name =
		g_strdup_printf("%s/%s_right",conf_key_prefix,
				 player_name);
	
	PRIVATE(self)->shoot_key_name =
		g_strdup_printf("%s/%s_shoot",conf_key_prefix,
				 player_name);

	conf_keyboard(self);

	g_signal_connect( window ,
			  "key-press-event",
			  GTK_SIGNAL_FUNC (key_pressed),
			  self);
	
	g_signal_connect( window ,
			  "key-release-event",
			  GTK_SIGNAL_FUNC (key_released),
			  self);


	return self;
}

static void 
mb_player_input_instance_init(MbPlayerInput * self) 
{
	PRIVATE(self) = g_new0 (MbPlayerInputPrivate, 1);			
}


static void
mb_player_input_finalize (GObject * object) 
{
	MbPlayerInput * self;

	self = MB_PLAYER_INPUT(object);

	g_free(PRIVATE(self));
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void
client_notified(GConfClient *client,guint cnxn_id,GConfEntry *entry,gpointer user_data) 
{
	const gchar * key;
	MbPlayerInput * self;
	
	self = MB_PLAYER_INPUT(user_data);
	
	key = gconf_entry_get_key( entry);

	if( g_str_equal( key, PRIVATE(self)->left_key_name) ||
	    g_str_equal( key, PRIVATE(self)->shoot_key_name) ||
	    g_str_equal( key, PRIVATE(self)->right_key_name) ) {

		conf_keyboard(self);


	}

}


static void 
conf_keyboard(MbPlayerInput * self)
{

	gchar * str;
	guint accel;
	GdkModifierType modifier;

	str = gconf_client_get_string(PRIVATE(self)->gconf_client,
				      PRIVATE(self)->left_key_name,
				      NULL);

	if( str != NULL) {
	
		gtk_accelerator_parse (str,
				       &accel,
				       &modifier);
		
		PRIVATE(self)->left_key = accel;
		PRIVATE(self)->left_key_modifier =modifier;
		g_free(str);

	}


	str = gconf_client_get_string(PRIVATE(self)->gconf_client,
				      PRIVATE(self)->right_key_name,
				      NULL);
	
	if( str != NULL) {
	
		gtk_accelerator_parse (str,
				       &accel,
				       &modifier);
		
		PRIVATE(self)->right_key = accel;
		PRIVATE(self)->right_key_modifier =modifier;
		g_free(str);

	} 



	str = gconf_client_get_string(PRIVATE(self)->gconf_client,
				      PRIVATE(self)->shoot_key_name,
				      NULL);
	
	if( str != NULL) {
	
		gtk_accelerator_parse (str,
				       &accel,
				       &modifier);
		
		PRIVATE(self)->shoot_key = accel;
		PRIVATE(self)->shoot_key_modifier =modifier;
		g_free(str);

	} 

}


static gboolean 
key_released(GtkWidget *widget,
	     GdkEventKey *event,
	     gpointer user_data) 
{
	
	MbPlayerInput * self;

	self = MB_PLAYER_INPUT(user_data);


	if( event->keyval == PRIVATE(self)->left_key) {
	
		g_signal_emit( self,signals[INPUT_NOTIFY_RELEASED],0,
			       LEFT_KEY);
	} else if( event->keyval == PRIVATE(self)->right_key) {
		g_signal_emit( self,signals[INPUT_NOTIFY_RELEASED],0,
			       RIGHT_KEY);
	} else if( event->keyval == PRIVATE(self)->shoot_key) {
		g_signal_emit( self,signals[INPUT_NOTIFY_RELEASED],0,
			       SHOOT_KEY);
	}

	return FALSE;
}

static gboolean
key_pressed(GtkWidget *widget,
	    GdkEventKey *event,
	    gpointer user_data)
{
	MbPlayerInput * self;

	self = MB_PLAYER_INPUT(user_data);


	if( event->keyval == PRIVATE(self)->left_key) {
	
		g_signal_emit( self,signals[INPUT_NOTIFY_PRESSED],0,
			       LEFT_KEY);
	} else if( event->keyval == PRIVATE(self)->right_key) {
		g_signal_emit( self,signals[INPUT_NOTIFY_PRESSED],0,
			       RIGHT_KEY);
	} else if( event->keyval == PRIVATE(self)->shoot_key) {
		g_signal_emit( self,signals[INPUT_NOTIFY_PRESSED],0,
			       SHOOT_KEY);
	}

	return FALSE;
}

static void
mb_player_input_class_init (MbPlayerInputClass	* mb_player_input_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(mb_player_input_class);

	g_object_class = G_OBJECT_CLASS(mb_player_input_class);
	g_object_class->finalize = mb_player_input_finalize;


    signals[INPUT_NOTIFY_PRESSED]= g_signal_new ("notify-pressed",
						  G_TYPE_FROM_CLASS (mb_player_input_class),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (MbPlayerInputClass, input_notify_pressed),
						  NULL, NULL,
						  g_cclosure_marshal_VOID__INT,
						  G_TYPE_NONE,
						  1, G_TYPE_INT);


    signals[INPUT_NOTIFY_RELEASED]= g_signal_new ("notify-released",
						  G_TYPE_FROM_CLASS (mb_player_input_class),
						  G_SIGNAL_RUN_FIRST |
						  G_SIGNAL_NO_RECURSE,
						  G_STRUCT_OFFSET (MbPlayerInputClass, input_notify_released),
						  NULL, NULL,
						  g_cclosure_marshal_VOID__INT,
						  G_TYPE_NONE,
						  1, G_TYPE_INT);

}


GType
mb_player_input_get_type (void)
{
	static GType	type = 0;

	if (!type)
	{
		const GTypeInfo info = {
			sizeof (MbPlayerInputClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)mb_player_input_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (MbPlayerInput),
			1,
			(GInstanceInitFunc) mb_player_input_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"MbPlayerInput",
				&info,
				0);
	}

	return type;
}


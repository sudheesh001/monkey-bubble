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


#include "input-manager.h"
#include "player-input.h"

#define PRIVATE(self) (self->private )
static GObjectClass* parent_class = NULL;

struct MbInputManagerPrivate 
{
	GConfClient * gconf_client;
	MbPlayerInput * left;
	MbPlayerInput * right;
};


MbInputManager *
mb_input_manager_new  (GtkWidget * window)
{
	MbInputManager * self;

	self = MB_INPUT_MANAGER( g_object_new(MB_TYPE_INPUT_MANAGER,NULL) );

	PRIVATE(self)->gconf_client = gconf_client_get_default();


	gconf_client_add_dir (PRIVATE(self)->gconf_client, CONF_KEY_PREFIX,
			      GCONF_CLIENT_PRELOAD_NONE, NULL);

	
	PRIVATE(self)->left =
		mb_player_input_new( window,
				     PRIVATE(self)->gconf_client,
				     CONF_KEY_PREFIX,
				     "player_1");


	PRIVATE(self)->right = 
		mb_player_input_new( window,
				     PRIVATE(self)->gconf_client,
				     CONF_KEY_PREFIX,
				     "player_2");
	return self;
}

MbPlayerInput * mb_input_manager_get_left(MbInputManager * m) {
	return PRIVATE(m)->left;
}

MbPlayerInput * mb_input_manager_get_right(MbInputManager * m) {
	return PRIVATE(m)->right;
}

static MbInputManager * instance = NULL;

MbInputManager *
mb_input_manager_get_instance(void) 
{
        return instance;
}

void
mb_input_manager_instance_set_window(GtkWidget * window) 
{
	instance = mb_input_manager_new(window);
}

static void 
mb_input_manager_instance_init(MbInputManager * self) 
{
	PRIVATE(self) = g_new0 (MbInputManagerPrivate, 1);			
}


static void
mb_input_manager_finalize (GObject * object) 
{
	MbInputManager * self;

	self = MB_INPUT_MANAGER(object);

	g_free(PRIVATE(self));
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}

}


static void
mb_input_manager_class_init (MbInputManagerClass	* mb_input_manager_class)
{
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(mb_input_manager_class);

	g_object_class = G_OBJECT_CLASS(mb_input_manager_class);
	g_object_class->finalize = mb_input_manager_finalize;

}


GType
mb_input_manager_get_type (void)
{
	static GType	type = 0;

	if (!type)
	{
		const GTypeInfo info = {
			sizeof (MbInputManagerClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)mb_input_manager_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (MbInputManager),
			1,
			(GInstanceInitFunc) mb_input_manager_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"MbInputManager",
				&info,
				0);
	}

	return type;
}


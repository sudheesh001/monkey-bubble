/*
 * AUTHORS
 *       Laurent Belmonte        <laurent.belmonte@aliacom.fr>
 *       Thomas Cataldo          <thomas.cataldo@aliacom.fr>
 *
 * Copyright (C) 2004 Laurent Belmonte, Thomas Cataldo
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

#include <glib-object.h>
#include <glib.h>

#include "game-sound.h"

static GObjectClass* parent_class = NULL;

struct MbGameSoundPrivate {
  
};

MbGameSound *
mb_game_sound_new (void) {
	MbGameSound * self;

	self = MB_GAME_SOUND(g_object_new(MB_TYPE_GAME_SOUND, NULL));

	return self;
}

static void bubble_sticked(Monkey * m,
			   Bubble * b,
			   MbGameSound * msg) 
{
  SoundManager * manager;
  manager = sound_manager_get_instance();

  sound_manager_play_fx(manager,MB_SAMPLE_STICK);
}

static void bubbles_exploded(  Monkey * monkey,
			       GList * exploded,
			       GList * fallen,
			       MbGameSound * msg)
{
  SoundManager * manager;
  manager = sound_manager_get_instance();

  sound_manager_play_fx(manager,MB_SAMPLE_EXPLODE);

}



static void bubble_shot( Monkey * monkey,
			 Bubble * bubble,
			 MbGameSound * msg) 
{
  SoundManager * manager;
  manager = sound_manager_get_instance();

  sound_manager_play_fx(manager,MB_SAMPLE_SHOOT);

}

static void bubble_wall_collision(Playground * p,
				  MbGameSound * msg)
{
  SoundManager * manager;
  manager = sound_manager_get_instance();

  sound_manager_play_fx(manager,MB_SAMPLE_REBOUND);

}

void mb_game_sound_connect_monkey(MbGameSound * mgs,Monkey * m)
{

  g_signal_connect( m,
		    "bubble-sticked",
		    G_CALLBACK(bubble_sticked),
		    mgs);

  g_signal_connect(m,
		   "bubbles-exploded",
		   G_CALLBACK(bubbles_exploded),
		   mgs);

  g_signal_connect( m,
		    "bubble-shot",
		    G_CALLBACK(bubble_shot),
		    mgs);


  g_signal_connect( monkey_get_playground(m),
		    "bubble-wall-collision",
		    G_CALLBACK(bubble_wall_collision),
		    mgs);
}


static void 
mb_game_sound_instance_init(MbGameSound * self) {
	self->priv = g_new0(MbGameSoundPrivate, 1);			
}


static void
mb_game_sound_dispose (GObject * object) {
	MbGameSound * self;

	self = MB_GAME_SOUND(object);

	g_free(self->priv);

	if (G_OBJECT_CLASS (parent_class)->dispose) {
		(* G_OBJECT_CLASS (parent_class)->dispose) (object);
	}

}


static void
mb_game_sound_class_init (MbGameSoundClass* mb_game_sound_class) {
	GObjectClass	* g_object_class;

	parent_class = g_type_class_peek_parent(mb_game_sound_class);

	g_object_class = G_OBJECT_CLASS(mb_game_sound_class);
	g_object_class->dispose = mb_game_sound_dispose;
}


GType mb_game_sound_get_type (void) {
	static GType	type = 0;

	if (!type) {
		const GTypeInfo info = {
			sizeof (MbGameSoundClass),
			NULL,	/* base initializer */
			NULL,	/* base finalizer */
			(GClassInitFunc)mb_game_sound_class_init,
			NULL,	/* class finalizer */
			NULL,	/* class data */
			sizeof (MbGameSound),
			1,
			(GInstanceInitFunc) mb_game_sound_instance_init,
			0
		};

		type = g_type_register_static (
				G_TYPE_OBJECT,
				"MbGameSound",
				&info,
				0);
	}

	return type;
}


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

#ifndef GAME_SOUND_H
#define GAME_SOUND_H

#include <glib-object.h>
#include "monkey.h"
#include "sound-manager.h"
G_BEGIN_DECLS

typedef struct MbGameSoundPrivate MbGameSoundPrivate;

typedef struct _MbGameSound MbGameSound;
typedef struct _MbGameSoundClass MbGameSoundClass;

MbGameSound *     mb_game_sound_new                 (void);

void mb_game_sound_connect_monkey(MbGameSound * mgs,Monkey * m);

GType		mb_game_sound_get_type	       (void);

#define MB_TYPE_GAME_SOUND			(mb_game_sound_get_type ())
#define MB_GAME_SOUND(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_TYPE_GAME_SOUND, MbGameSound))
#define MB_GAME_SOUND_CLASS(klass)		(G_TYPE_CHACK_CLASS_CAST((klass), MB_TYPE_GAME_SOUND, MbGameSoundClass))
#define MB_IS_GAME_SOUND(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_TYPE_GAME_SOUND))
#define MB_IS_GAME_SOUND_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_TYPE_GAME_SOUND))
#define MB_GAME_SOUND_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_TYPE_GAME_SOUND, MbGameSoundClass))

struct _MbGameSound {
	GObject		   base_instance;
	MbGameSoundPrivate * priv;
};

struct _MbGameSoundClass {
	GObjectClass	  base_class;
};


G_END_DECLS

#endif /* GAME_SOUND_H */

/* mn-game-manager.h - 
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
#ifndef _MN_GAME_MANAGER_H_
#define _MN_GAME_MANAGER_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_MN_GAME_MANAGER   (mn_game_manager_get_type())

#define MN_GAME_MANAGER(object)(G_TYPE_CHECK_INSTANCE_CAST((object),TYPE_MN_GAME_MANAGER,MnGameManager))
#define MN_GAME_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MN_GAME_MANAGER,MnGameManagerClass))
#define IS_MN_GAME_MANAGER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_MN_GAME_MANAGER))
#define IS_MN_GAME_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MN_GAME_MANAGER))
#define MN_GAME_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MN_GAME_MANAGER, MnGameManagerClass))

typedef struct MnGameManagerPrivate MnGameManagerPrivate;


typedef struct {
  GObject parent_instance;
  MnGameManagerPrivate * private;
} MnGameManager;


typedef struct {
  GObjectClass parent_class;
} MnGameManagerClass;


GType mn_game_manager_get_type(void);
MnGameManager * mn_game_manager_new(void);

gboolean mn_game_manager_start_server(MnGameManager * manager);

void mn_game_manager_stop_server(MnGameManager * manager);

void mn_game_manager_join(MnGameManager * manager);
G_END_DECLS

#endif

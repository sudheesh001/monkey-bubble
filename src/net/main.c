/* main.c - 
 * Copyright (C) 2002 Christophe Segui
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
 
#include "mn-game-manager.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>


int main(int argc, char **argv) {
	MnGameManager * manager;

    g_type_init();
    g_thread_init(NULL);

    manager = mn_game_manager_new();

    mn_game_manager_start_server(manager);
	 //    mn_game_manager_join(manager);
	 gtk_main();
    return(0);
}

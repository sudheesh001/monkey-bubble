/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* monkey-server.c - 
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

#include "monkey.h"
 
#include "monkey-message-handler.h"
#include "monkey-network-game.h"

enum GameState {
        PLAYING = 1,
        WAITING_NEW_CLIENT = 2,
        STARTING = 3,
        NO_STATE = 4
};

struct NetworkMonkey {
        guint       monkey_id;
        Monkey *    monkey;
        GMutex *    mutex;
        gchar *     name;
};

struct MonkeyClient {
        MonkeyMessageHandler * handler;
        GList *                listen_monkeys;
        gchar *                name;
};


struct MonkeyNetworkGamePrivate {
        GList *         network_monkeys;
        GList *         clients;
        
        enum GameState       state;

};

#define PRIVATE( MonkeyNetworkGame ) (MonkeyNetworkGame->private)

static GObjectClass* parent_class = NULL;

void monkey_network_game_finalize(GObject *);


void start_waiting_connect_loop(MonkeyNetworkGame * mng) {

        // START THE THREAD
        
        PRIVATE(mng)->state = WAITING_NEW_CLIENT;

}

MonkeyNetworkGame *monkey_network_game_new() {
        MonkeyNetworkGame * mng;
        
        mng = 
                MONKEY_NETWORK_GAME(g_object_new(TYPE_MONKEY_NETWORK_GAME
                                                    , NULL));


        PRIVATE(mng)->clients = NULL;
        PRIVATE(mng)->network_monkeys = NULL;

        PRIVATE(mng)->state = NO_STATE;

        start_waiting_connect_loop(mng);
        return mng;
        
}


void monkey_network_game_finalize(GObject *object) {
        MonkeyNetworkGame * mng = (MonkeyNetworkGame *) object;
	
        g_free(PRIVATE(mng));

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
        }		
}

static void monkey_network_game_instance_init(MonkeyNetworkGame * mng) {
        mng->private =g_new0 (MonkeyNetworkGamePrivate, 1);
}

static void monkey_network_game_class_init (MonkeyNetworkGameClass *klass) {
        GObjectClass* object_class;
        
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = monkey_network_game_finalize;
}

GType monkey_network_game_get_type(void) {
        static GType monkey_network_game_type = 0;
        
        if (!monkey_network_game_type) {
                static const GTypeInfo monkey_network_game_info = {
                        sizeof(MonkeyNetworkGameClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) monkey_network_game_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(MonkeyNetworkGame),
                        1,              /* n_preallocs */
                        (GInstanceInitFunc) monkey_network_game_instance_init,
                };
                
                monkey_network_game_type = g_type_register_static(G_TYPE_OBJECT,
                                                                     "MonkeyNetworkGame",
                                                                     &monkey_network_game_info, 0
                                                                     );

        }
        
        return monkey_network_game_type;
}

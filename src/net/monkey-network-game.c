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


#define FRAME_DELAY 10
#define INIT_BUBBLES_COUNT 8+7+8+7

struct MonkeyNetworkGamePrivate {
        GList * monkeys;
        GList * clients;
        NetworkClient * owner;
};

#define PRIVATE( MonkeyNetworkGame ) (MonkeyNetworkGame->private)

static GObjectClass* parent_class = NULL;

void monkey_network_game_finalize(GObject *);


static void recv_shoot(MonkeyMessageHandler * handler,
                       guint32 monkey_id,
                       guint32 time,
                       gfloat angle) {
        g_print("RECV SHOOT angle %f *******************\n\n*********\n",angle);

        monkey_message_handler_send_add_bubble(handler,
                                               0,
                                               1);

}

void start_new_game(MonkeyNetworkGame * g) {
        GList * next;
        Bubble  * bubbles[INIT_BUBBLES_COUNT];
        Color  colors[INIT_BUBBLES_COUNT];
        int i;

	 for(i = 0; i < INIT_BUBBLES_COUNT; i++ ) {
                 Color c = rand()%COLORS_COUNT;
                 colors[i] = c;
                 bubbles[i] = bubble_new(c,0,0);
         }

        next = PRIVATE(g)->clients;
        
        while( next != NULL) {
                NetworkClient * nc;
                Monkey * m;
                nc = (NetworkClient *)next->data;
                m = monkey_new();
                board_init( playground_get_board( monkey_get_playground( m )),
                            bubbles,INIT_BUBBLES_COUNT);
                
                monkey_message_handler_send_bubble_array(nc->handler,
                                                         nc->client_id,
                                                         INIT_BUBBLES_COUNT,
                                                         colors);

                monkey_message_handler_send_add_bubble(nc->handler,
                                                       nc->client_id,
                                                       1);


                monkey_message_handler_send_add_bubble(nc->handler,
                                                       nc->client_id,
                                                       1);


                g_signal_connect( G_OBJECT(nc->handler),
                                  "recv-shoot",
                                  G_CALLBACK( recv_shoot ),
                                  m);

                next = g_list_next(next);

        }


        next = PRIVATE(g)->clients;
        
        while( next != NULL) {
                NetworkClient * nc;
                nc = (NetworkClient *)next->data;

                monkey_message_handler_send_start(nc->handler);
                next = g_list_next(next);

        }
               
}

MonkeyNetworkGame *monkey_network_game_new(NetworkGame * game) {
        MonkeyNetworkGame * mng;
        
        mng = 
                MONKEY_NETWORK_GAME(g_object_new(TYPE_MONKEY_NETWORK_GAME
                                                    , NULL));


        // listen all players
        PRIVATE(mng)->clients = game->clients;
        PRIVATE(mng)->owner = game->game_owner;
        start_new_game(mng);
        // create all monkeys

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

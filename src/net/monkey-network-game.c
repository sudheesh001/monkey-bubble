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
#include "clock.h"

#define FRAME_DELAY 10
#define INIT_BUBBLES_COUNT 8+7+8+7


enum {
        GAME_STOPPED,
        LAST_SIGNAL
};

static guint32 signals[LAST_SIGNAL];


struct MonkeyNetworkGamePrivate {
        GList * monkeys;
        GList * clients;
        GList * lostList;
        GMutex * listMutex;
        GMutex * lostMutex;
        NetworkClient * owner;
        Clock * clock;
};

struct Client {
        Monkey * monkey;
        MonkeyNetworkGame * game;
        MonkeyMessageHandler * handler;
        int monkey_id;
        GMutex * mutex;
};

#define PRIVATE( MonkeyNetworkGame ) (MonkeyNetworkGame->private)

static GObjectClass* parent_class = NULL;

void monkey_network_game_finalize(GObject *);


static Color what_bubble(Monkey * monkey) {
  gint * colors_count;
  gint rnd,count;


  colors_count = board_get_colors_count(playground_get_board(monkey_get_playground(monkey)));

  rnd = rand()%COLORS_COUNT;
  count = 0;
  while( rnd >= 0 ) {
    count++;
    count %= COLORS_COUNT;

    while(colors_count[count] == 0) {
      count++;
      count %= COLORS_COUNT;
    }
    rnd--;
  }
  
  return count;
}

static void add_bubble(struct Client * c) {
  gint * colors_count;
  gint rnd,count;
  Monkey * monkey;


  monkey = c->monkey;
  colors_count = board_get_colors_count(playground_get_board(monkey_get_playground(monkey)));

  rnd = rand()%COLORS_COUNT;
  count = 0;
  while( rnd >= 0 ) {
    count++;
    count %= COLORS_COUNT;

    while(colors_count[count] == 0) {
      count++;
      count %= COLORS_COUNT;
    }
    rnd--;
  }
  
  shooter_add_bubble(monkey_get_shooter(monkey),bubble_new(count,0,0));

}


static void recv_shoot(MonkeyMessageHandler * handler,
                       guint32 monkey_id,
                       guint32 time,
                       gfloat angle,
                       struct Client * c) {
        g_print("RECV SHOOT angle %f *******************\n\n*********\n",angle);

        g_mutex_lock(c->mutex);
        shooter_set_angle(monkey_get_shooter(c->monkey),
                          angle);

        monkey_update(c->monkey,time);
        monkey_shoot( c->monkey,time);
        add_bubble(c);

        g_print("bubble added\n");
        g_mutex_unlock(c->mutex);
}

void bubble_added(Shooter * s,
                  Bubble * b,
                  struct Client * c) {
        
        monkey_message_handler_send_add_bubble(c->handler,
                                               c->monkey_id,
                                               bubble_get_color(b));
        
        
        

}

void bubble_sticked(Monkey * monkey,Bubble * b,
                    struct Client * c) {



        if( ( monkey_get_shot_count(monkey) % 8 ) == 0 ) {
                            g_print("go down_n");

                monkey_set_board_down( monkey);
        }        

}

void game_lost(Monkey * m,
               struct Client * c) {

        MonkeyNetworkGame * game;

        game = c->game;
        g_print("lost \n");

        g_mutex_lock(PRIVATE(game)->lostMutex);
        monkey_message_handler_send_winlost( c->handler,
                                             c->monkey_id,
                                             1);

        PRIVATE(game)->lostList = g_list_append( PRIVATE(game)->lostList,
                                                 c);

        g_mutex_unlock(PRIVATE(game)->lostMutex);


        
}

void bubbles_exploded(  Monkey * monkey,
                   GList * exploded,
                   GList * fallen,
                   struct Client * c) {

        
        int i;
        int to_go;
	GList * next;

        to_go = MAX(0,-3 + g_list_length(exploded)  + g_list_length(fallen)*1.5);

        if( to_go != 0) {

                Color * colors;
        
                colors = g_malloc( sizeof(Color)*to_go );

                for( i = 0 ; i < to_go ; i++ ) {
                        colors[i] = rand()%COLORS_COUNT;
                }


                //   g_mutex_lock( PRIVATE( c->game )->listMutex);
                next = PRIVATE( c->game )->clients;

                while( next != NULL) {
                        struct Client * client;
                        Monkey * other;
                        guint8 * columns;

                        client = (struct Client *) next->data;

                        if( client != c ) {
                        g_mutex_lock(client->mutex);
                        g_print("lock \n");
                        other = client->monkey;
                        

                        columns = monkey_add_bubbles( other, 
                                                      to_go,
                                                      colors); 
                        
                        
                        monkey_message_handler_send_waiting_added(client->handler,
                                                                  client->monkey_id, 
                                                                  to_go,
                                                                  colors,
                                                                  columns);

                        g_print("unlock \n");
                        g_mutex_unlock(client->mutex);
                        g_print("unlocked \n");
                        
                        g_free(columns); 
                        }
                        next = g_list_next(next);
                }
                
                //  g_mutex_unlock( PRIVATE( c->game )->listMutex);

                g_free(colors);
         
        }
                
        g_print("exploeded %d\n",g_list_length(exploded));
}


static void bubble_shot( Monkey * monkey,
                         Bubble * bubble,
                         struct Client * c) {
}

void create_monkey(MonkeyNetworkGame * g,
                   struct Client * client,
                   Color * colors,
                   int count) {

        Monkey * m;
        Shooter * s;

        Bubble  ** bubbles;
        int i;
        Color c;
        bubbles = g_malloc( sizeof(Bubble *) * count);
	 for(i = 0; i < count; i++ ) {
                 bubbles[i] = bubble_new(colors[i],0,0);
         }
        m = monkey_new(TRUE);

        board_init( playground_get_board( monkey_get_playground( m )),
                    bubbles,count);
        
        monkey_message_handler_send_bubble_array(client->handler,
                                                 client->monkey_id,
                                                 INIT_BUBBLES_COUNT,
                                                 colors);
                
        s = monkey_get_shooter(m);

        client->monkey =m;

        client->mutex = g_mutex_new();
        g_signal_connect( G_OBJECT( s),
                          "bubble-added",
                          G_CALLBACK(bubble_added),
                          client);



        g_signal_connect( G_OBJECT( m),
                          "bubble-sticked",
                          G_CALLBACK(bubble_sticked),
                          client);

        g_signal_connect( G_OBJECT(m),
                          "game-lost",
                          G_CALLBACK(game_lost),
                          client);
        
        g_signal_connect( G_OBJECT( m),
                          "bubbles-exploded",
                          G_CALLBACK(bubbles_exploded),
                          client);

        g_signal_connect( G_OBJECT( m),
                          "bubble-shot",
                          G_CALLBACK(bubble_shot),
                          client);


        c = what_bubble( m );
        shooter_add_bubble(s,bubble_new(c,0,0));

        c = what_bubble( m );
        shooter_add_bubble(s,bubble_new(c,0,0));

        g_signal_connect( G_OBJECT(client->handler),
                          "recv-shoot",
                          G_CALLBACK( recv_shoot ),
                          client);
        
        
}

void update_client(gpointer d,gpointer ud) {
        struct Client * client;
        
        client = (struct Client *) d;
        MonkeyNetworkGame * g;

        g = MONKEY_NETWORK_GAME(ud);

        g_mutex_lock(client->mutex);

        
        monkey_update(client->monkey,clock_get_time(PRIVATE(g)->clock));


        g_mutex_unlock(client->mutex);        

}

gboolean update_idle(gpointer d) {
        MonkeyNetworkGame * g;
        gboolean stop_game;

        stop_game = FALSE;
        g = MONKEY_NETWORK_GAME(d);
        g_mutex_lock( PRIVATE(g)->listMutex);

        g_mutex_lock( PRIVATE(g)->lostMutex);
        
        if( PRIVATE(g)->lostList != NULL) {
                GList * next;
                
                next = PRIVATE(g )->lostList;

                while( next != NULL ) {
                        struct Client * client;
                        
                        client = (struct Client *) next->data;
                        PRIVATE(g)->clients = g_list_remove( PRIVATE(g)->clients,client);
        
                        PRIVATE(g)->lostList = g_list_remove( PRIVATE(g)->lostList,client);
        
                        if( g_list_length( PRIVATE(g)->clients) == 1 ) {
                        
                                client =(struct Client *) PRIVATE(g)->clients->data;
                                monkey_message_handler_send_winlost( client->handler,
                                                                     client->monkey_id,
                                                                     0);
                                PRIVATE(g)->clients = g_list_remove( PRIVATE(g)->clients,client);
                        
                                stop_game = TRUE;
                        }

                        next = PRIVATE(g)->lostList;
                }
                
        }

        g_mutex_unlock( PRIVATE(g)->lostMutex);

        if( stop_game == TRUE) {
                g_print("stop the game \n");
        }

        
        g_list_foreach( PRIVATE(g)->clients,
                        update_client,
                        g);

        g_mutex_unlock( PRIVATE(g)->listMutex);

        return !stop_game;
}

static void idle_stopped(gpointer data) {

        MonkeyNetworkGame * g;

        g = MONKEY_NETWORK_GAME(data);

        g_signal_emit( G_OBJECT(g),signals[GAME_STOPPED],0);
}

void start_update_thread(MonkeyNetworkGame * g) {
        g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE,
                           10,update_idle,g,
                           idle_stopped);
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
                struct Client * client;
                
                client = (struct Client *)next->data;
                create_monkey(g,client,colors,INIT_BUBBLES_COUNT);
                next = g_list_next(next);

        }


        next = PRIVATE(g)->clients;
        
        while( next != NULL) {
                struct Client * nc;
                nc = (struct Client *)next->data;

                monkey_message_handler_send_start(nc->handler);

                g_print("send started \n");
                next = g_list_next(next);

        }

        clock_start(PRIVATE(g)->clock);
        start_update_thread(g);
}

MonkeyNetworkGame *monkey_network_game_new(NetworkGame * game) {
        MonkeyNetworkGame * mng;
        GList * next;
        mng = 
                MONKEY_NETWORK_GAME(g_object_new(TYPE_MONKEY_NETWORK_GAME
                                                    , NULL));


        PRIVATE(mng)->clock = clock_new();
        PRIVATE(mng)->clients = NULL;
        PRIVATE(mng)->lostList = NULL;
        next = game->clients;

        PRIVATE(mng)->listMutex = g_mutex_new();
        PRIVATE(mng)->lostMutex = g_mutex_new();

        while(next != NULL) {
                NetworkClient * nc;
                struct Client * client;

                nc = (NetworkClient *) next->data;
                client = g_malloc( sizeof(struct Client));

                client->monkey = NULL;
                client->handler = nc->handler;
                client->game = mng;
                client->monkey_id = nc->client_id;

                PRIVATE(mng)->clients = 
                        g_list_append( PRIVATE(mng)->clients,
                                       client);

                next = g_list_next(next);
        }

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


        signals[GAME_STOPPED]= g_signal_new ("game-stopped",
                                             G_TYPE_FROM_CLASS (klass),
                                             G_SIGNAL_RUN_FIRST |
                                             G_SIGNAL_NO_RECURSE,
                                             G_STRUCT_OFFSET (MonkeyNetworkGameClass, game_stopped),
                                             NULL, NULL,
                                             g_cclosure_marshal_VOID__VOID,
                                             G_TYPE_NONE,

                                             0,NULL);
        g_print("class initialised \n\n\n");
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

/* game_network_player_manager.c
 * Copyright (C) 2002 Laurent Belmonte
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
#include <gtk/gtk.h>
#include "game-network-player-manager.h"
#include "game-network-player.h"
#include "game.h"
#include "game-manager.h"
#include "ui-main.h"

#define PRIVATE(game_network_player_manager) (game_network_player_manager->private )

static GObjectClass* parent_class = NULL;

struct GameNetworkPlayerManagerPrivate {
  MonkeyCanvas * canvas;
  GtkWidget * window;
  GameNetworkPlayer * current_game;
  int current_level;
  gint current_score;
  NetworkMessageHandler * handler;
  Monkey * monkey;
    int client_id;

    gulong add_bubble_handler_id;
};

static void game_network_player_manager_state_changed(Game * game,GameNetworkPlayerManager * g);


static void game_network_player_manager_game_manager_iface_init(GameManagerClass * i);

static void game_network_player_manager_finalize(GObject* object);
static void game_network_player_manager_instance_init(GameNetworkPlayerManager * game_network_player_manager) {
  game_network_player_manager->private =g_new0 (GameNetworkPlayerManagerPrivate, 1);			
}


static void game_network_player_manager_class_init (GameNetworkPlayerManagerClass *klass) {
  GObjectClass* object_class;

  parent_class = g_type_class_peek_parent(klass);
  object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = game_network_player_manager_finalize;
}


GType game_network_player_manager_get_type(void) {
  static GType game_network_player_manager_type = 0;
    
  if (!game_network_player_manager_type) {
    static const GTypeInfo game_network_player_manager_info = {
      sizeof(GameNetworkPlayerManagerClass),
      NULL,           /* base_init */
      NULL,           /* base_finalize */
      (GClassInitFunc) game_network_player_manager_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof(GameNetworkPlayerManager),
      1,              /* n_preallocs */
      (GInstanceInitFunc) game_network_player_manager_instance_init,
    };


    static const GInterfaceInfo iface_game_manager = {
      (GInterfaceInitFunc) game_network_player_manager_game_manager_iface_init,
      NULL,
      NULL
    };
      
    game_network_player_manager_type = g_type_register_static(G_TYPE_OBJECT,
							"GameNetworkPlayerManager",
							&game_network_player_manager_info,
							0);
	 
	 
    g_type_add_interface_static(game_network_player_manager_type,
				TYPE_GAME_MANAGER,
				&iface_game_manager);
      
      
  }
    
  return game_network_player_manager_type;
}


GameNetworkPlayerManager * game_network_player_manager_new(GtkWidget * window,MonkeyCanvas * canvas,
																			  NetworkMessageHandler * handler,int client_id) {

  GameNetworkPlayerManager * game_network_player_manager;
  game_network_player_manager = GAME_NETWORK_PLAYER_MANAGER (g_object_new (TYPE_GAME_NETWORK_PLAYER_MANAGER, NULL));

  PRIVATE(game_network_player_manager)->canvas = canvas;
  PRIVATE(game_network_player_manager)->window = window;
  PRIVATE(game_network_player_manager)->current_game = NULL;

  PRIVATE(game_network_player_manager)->handler = handler;
  PRIVATE(game_network_player_manager)->client_id = client_id;
  return game_network_player_manager;
}




static void game_network_player_manager_finalize(GObject* object) {

  GameNetworkPlayerManager * game_network_player_manager = GAME_NETWORK_PLAYER_MANAGER(object);

  /* TODO : free current game */
  g_free(game_network_player_manager->private);

  if (G_OBJECT_CLASS (parent_class)->finalize) {
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
  }
}


static void game_network_player_manager_game_manager_iface_init(GameManagerClass * i) {
  game_manager_class_virtual_init(i,
				  game_network_player_manager_start,
				  game_network_player_manager_stop);
}

static void game_network_player_manager_state_changed(Game * game,
						GameNetworkPlayerManager * manager) {

  UiMain * ui_main =  ui_main_get_instance();

  g_print("STATE changed \n");
  if( game_get_state( game ) == GAME_FINISHED ) {
      g_print("GAME finished\n");
      PRIVATE(manager)->current_score = game_network_player_get_score( GAME_NETWORK_PLAYER(game));

      ui_main_set_game(ui_main,NULL);
      //   g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(manager)->handler ),
      //					     G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,manager);
      
      monkey_canvas_clear(PRIVATE(manager)->canvas);

      g_object_unref(G_OBJECT(PRIVATE(manager)->current_game));
      PRIVATE(manager)->current_game = NULL;
  }
				
}


gboolean start_timeout(gpointer data) {
  GameNetworkPlayerManager * manager;
  GameNetworkPlayer * game;

  manager = GAME_NETWORK_PLAYER_MANAGER(data);


  UiMain * ui_main =  ui_main_get_instance();

  monkey_canvas_clear( PRIVATE(manager)->canvas);
  monkey_canvas_paint( PRIVATE(manager)->canvas);
	 
  ui_main_set_game(ui_main,
						 GAME( game = game_network_player_new( PRIVATE(manager)->window,
																			PRIVATE(manager)->canvas,
																			PRIVATE(manager)->monkey,
																			PRIVATE(manager)->handler,
																			PRIVATE(manager)->client_id,
																			PRIVATE(manager)->current_score))
						 );

  monkey_canvas_paint( PRIVATE(manager)->canvas);

  g_print("GameNetworkPlaeyrManager start timeout \n");
  game_start( GAME(game) );
  

  g_signal_connect( G_OBJECT(game), "state-changed",
		    G_CALLBACK(game_network_player_manager_state_changed),manager);
  PRIVATE(manager)->current_game = game;

  return FALSE;
}

static void recv_add_bubble(NetworkMessageHandler * handler,
		     guint32 monkey_id,
		     Color bubble,
		     GameNetworkPlayerManager * manager) {

    if( PRIVATE(manager)->current_game == NULL) {
		  shooter_add_bubble(monkey_get_shooter(PRIVATE(manager)->monkey),bubble_new(bubble,0,0));
    }
  
}

void recv_start(NetworkMessageHandler * handler,
		GameNetworkPlayerManager * manager) {
  g_print("game-network-player-manager : recv start \n");

  g_idle_add(start_timeout,manager);
}

void recv_bubble_array(NetworkMessageHandler * handler,
		       guint32 monkey_id,
		       guint8 bubble_count,
		       Color * colors,
		       GameNetworkPlayerManager * manager) {

  Monkey * m;
  Bubble ** bubbles;

  int i;
  g_print("game-network-player-manager : recv bubble array %d,%d\n",monkey_id,bubble_count);
  
  m = PRIVATE(manager)->monkey;
  bubbles = g_malloc(bubble_count*( sizeof(Bubble *)));
  
  for(i = 0 ; i < bubble_count; i++) {
    bubbles[i] = bubble_new(colors[i],0,0);
  }

  board_init( playground_get_board( monkey_get_playground( m )),
	      bubbles,bubble_count);



}



static void 
game_created_ok(GameNetworkPlayerManager * manager) 
{
	 xmlDoc * doc;
	 xmlNode * text, * root;
	


	 PRIVATE(manager)->monkey = monkey_new(TRUE);


 
	 doc = xmlNewDoc("1.0");
	 root = xmlNewNode(NULL,
			   "message");
	 
	 xmlDocSetRootElement(doc, root);
    
	 
	 xmlNewProp(root,"name","game_created_ok");
	 
	 text = xmlNewText("1");
	 xmlAddChild(root,text);
	 
	 
	 network_message_handler_send_xml_message(PRIVATE(manager)->handler,
															PRIVATE(manager)->client_id,
															doc);
	 
	 xmlFreeDoc(doc);
}



static void
recv_xml_message(NetworkMessageHandler * handler,
					  guint32 client_id,
					  xmlDoc * message,
					  GameNetworkPlayerManager * manager) {


        xmlNode * root;
        char * message_name;
        
        root = message->children;
        
        g_assert( g_str_equal(root->name,"message"));
        
        
        message_name = xmlGetProp(root,"name");
        
		  if(g_str_equal(message_name,"game_created") ) {
				
				int game_id;
            
				sscanf(root->children->content,"%d",&game_id);
				g_print("game-network-player-manager.c : game started %d \n",game_id);                
				
				game_created_ok(manager);
        }


}
void game_network_player_manager_start(GameManager * g) {
	 
  GameNetworkPlayerManager * manager;
	 
  manager = GAME_NETWORK_PLAYER_MANAGER(g);

  PRIVATE(manager)->current_level = 0;
  PRIVATE(manager)->current_score = 0;


  g_signal_connect( G_OBJECT(PRIVATE(manager)->handler),
						  "recv-xml-message",
						  G_CALLBACK( recv_xml_message ),
						  manager);
  
  g_signal_connect( G_OBJECT( PRIVATE(manager)->handler), "recv-bubble-array",
		    G_CALLBACK(recv_bubble_array),manager);

  PRIVATE(manager)->add_bubble_handler_id = 
      g_signal_connect( G_OBJECT( PRIVATE(manager)->handler), "recv-add-bubble",
			G_CALLBACK(recv_add_bubble),manager);

  g_signal_connect( G_OBJECT( PRIVATE(manager)->handler), "recv-start",
		    G_CALLBACK(recv_start),manager);

}

void game_network_player_manager_stop(GameManager * g) {
  GameNetworkPlayerManager * manager;
  UiMain * ui_main =  ui_main_get_instance();


  manager = GAME_NETWORK_PLAYER_MANAGER(g);
  game_stop( GAME(PRIVATE(manager)->current_game));

  g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(manager)->current_game ),
                                         G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,manager);

  g_object_unref( PRIVATE(manager)->current_game);
  PRIVATE(manager)->current_game = NULL;
  ui_main_set_game(ui_main,NULL);
}

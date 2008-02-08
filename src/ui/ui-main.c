/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* ui-main.h - 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "ui-main.h"
#include "monkey-canvas.h"
#include "monkey.h"
#include "game.h"
#include "game-1-player.h"
#include "game-1-player-manager.h"
#include "game-2-player-manager.h"
#include "game-network-player-manager.h"
#include "game-2-player.h"
#include "keyboard-properties.h"
#include "sound-manager.h"

#ifdef GNOME
#include <libgnomeui/libgnomeui.h>
#include <libgnomeui/gnome-about.h>
#include <libgnome/gnome-score.h>
#include <libgnome/gnome-sound.h>
#include <libgnome/gnome-help.h>
#endif
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>
#include <glib/gi18n.h>

#include <string.h>
#include <stdlib.h>

#ifdef MAEMO
#include <hildon/hildon-program.h>
#include "global.h"
#include "state.h"
#include <conic.h>
#endif

#include "ui-network-client.h"
#include "ui-network-server.h"
#include "simple-server.h"

#define PRIVATE(main) (main->private)

static GObjectClass* parent_class = NULL;

void ui_main_game_changed(Game * game,UiMain * ui_main);

static void new_1_player_game      (GtkAction* action,
				    UiMain   * uimain);
#ifdef GNOME
static void new_2_player_game      (GtkAction* action,
				    UiMain   * uimain);
#endif

static void new_network_game       (GtkAction* action,
				    UiMain   * uimain);
#ifdef GNOME
static void new_network_server     (GtkAction* action,
				    UiMain   * uimain);
#endif

#ifdef GNOME
static void show_high_scores       (GtkAction* action,
				    UiMain   * uimain);
#endif

static void pause_game             (GtkAction* action,
				    UiMain   * uimain);
#ifdef GNOME
static void stop_game              (GtkAction* action,
				    UiMain   * uimain);
#endif
static void quit_program           (GtkAction* action,
				    UiMain   * uimain);

#ifdef GNOME
static void about                  (GtkAction* action,
				    UiMain   * uimain);

static void show_error_dialog (GtkWindow *transient_parent,
                               const char *message_format, ...);

static void show_help_content       (GtkAction* action,
				     UiMain   * uimain);

static void show_preferences_dialog (GtkAction* action,
				     UiMain   * uimain);
#endif

static void ui_main_new_1_player_game(UiMain * ui_main);

struct UiMainPrivate {
        GtkAccelGroup * accel_group;
        GtkItemFactory * item_factory;
        GtkWidget * menu;
        GtkWidget * status_bar;
        MonkeyCanvas * canvas;
        GtkWidget * window;
        Block * main_image;
        Game * game;
        GameManager * manager;
        gboolean fullscreen;
        SoundManager * sm;
        GladeXML * glade_xml;
	GtkActionGroup* actions;
#ifdef MAEMO
	ConIcConnection *ic;
#endif
};

static void ui_main_class_init(UiMainClass* klass);
static void ui_main_init(UiMain* main);
static void ui_main_finalize(GObject* object);


static void ui_main_draw_main(UiMain * ui_main);

static UiMain* ui_main_new(void);


#ifdef MAEMO
static void ui_main_topmost_cb(GObject *self, GParamSpec *property_param, gpointer null)
{
       HildonProgram *program = HILDON_PROGRAM(self);

       if (program == NULL) return;

       if (hildon_program_get_is_topmost(program)) {
               hildon_program_set_can_hibernate(program, FALSE);
       } else {
               if (state.game == 1 && global.game!=NULL) {
                       //game_1_player_save(global.game); //TODO: enable saving
                       state.loadmap=1;
               }
               state_save();
               hildon_program_set_can_hibernate(program, TRUE);
       }
}

void continue_game(void) {
        UiMain * ui_main;
        ui_main = ui_main_get_instance();

        ui_main_new_1_player_game(ui_main);
}
#endif

GType ui_main_get_type(void) {
        static GType ui_main_type = 0;

        if(!ui_main_type) {

                static const GTypeInfo ui_main_info = {
                        sizeof(UiMainClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) ui_main_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof(UiMain),
                        1,              /* n_preallocs */
                        (GInstanceInitFunc) ui_main_init,
                };
    

                ui_main_type = g_type_register_static(G_TYPE_OBJECT,
                                                      "UiMain",
                                                      &ui_main_info, 0);


        }
        return ui_main_type;
}


UiMain * ui_main_get_instance(void) {
        static UiMain * instance = NULL;

        if( !instance) {
                instance = ui_main_new();
        }

        return instance;
}

static void
window_destroy_cb (GtkWidget* window,
		   UiMain   * uimain)
{
	quit_program (NULL, uimain);
}

static UiMain*
ui_main_new (void)
{
	GtkUIManager* ui_manager;
	GError* error = NULL;
	GtkActionEntry  entries[] = {
		{"Game"},
#ifdef MAEMO
		{"GameNew", NULL, N_("New Game"),
		 NULL, NULL,
		 G_CALLBACK (new_1_player_game)
		},
		{"GameJoin", NULL, N_("Join network game"),
		 NULL, NULL,
		 G_CALLBACK (new_network_game)
		},
		{"GamePause", NULL, N_("Pause"),
		 NULL, NULL,
		 G_CALLBACK (pause_game)
		},
		{"ApplicationQuit", NULL, N_("Quit"),
		 NULL, NULL,
		 G_CALLBACK (quit_program)
		}
#elif defined(GNOME)
		{"GameNew1Player", NULL, N_("New 1 player")},
		{"GameNew2Player", NULL, N_("New 2 players")},
		{"GameNetworkNew", NULL, N_("New network game")},
		{"GameNetworkJoin", NULL, N_("Join _network game")},
		{"GameSettings", GTK_STOCK_PREFERENCES},
		{"GameScores", GTK_STOCK_INDEX, N_("_High Scores")},
		{"GamePause", NULL, N_("Pause game")},
		{"GameResume", NULL, N_("Resume game")},
		{"GameStop", NULL, N_("Stop game")},
		{"GameQuit", GTK_STOCK_QUIT},
		{"HelpContent", GTK_STOCK_HELP, N_("_Contents"),
		 NULL, NULL,
		 G_CALLBACK (show_help_content)},
		{"HelpAbout", NULL, N_("_About"),
		 NULL, NULL,
		 G_CALLBACK (about)}
#endif
	};
#ifdef MAEMO
	HildonProgram * program;
	GtkWidget * container;
	GtkWidget * main_menu;
#endif
        UiMain * ui_main;
        GtkWidget * vbox;
#ifdef GNOME
        GtkWidget * item;
        KeyboardProperties * kp;
#endif

        ui_main = UI_MAIN(g_object_new(UI_TYPE_MAIN, NULL));

#ifdef GNOME
        PRIVATE(ui_main)->glade_xml = glade_xml_new(DATADIR"/monkey-bubble/glade/monkey-bubble.glade","main_window",NULL);
        
        PRIVATE(ui_main)->window = glade_xml_get_widget( PRIVATE(ui_main)->glade_xml, "main_window");
#endif
#ifdef MAEMO
	PRIVATE(ui_main)->glade_xml = glade_xml_new(DATADIR"/monkey-bubble/glade/monkey-bubble.glade","main_vbox",NULL);

	container = glade_xml_get_widget( PRIVATE(ui_main)->glade_xml, "main_vbox");
	program = HILDON_PROGRAM(hildon_program_get_instance());
	PRIVATE(ui_main)->window = hildon_window_new();
	g_signal_connect (PRIVATE (ui_main)->window, "destroy",
			  G_CALLBACK (window_destroy_cb), ui_main);
	hildon_program_add_window(program, HILDON_WINDOW(PRIVATE(ui_main)->window));
	gtk_container_add(GTK_CONTAINER(PRIVATE(ui_main)->window),
				container);
	g_set_application_name(_("Monkey Bubble"));
	g_signal_connect(G_OBJECT(program), "notify::is-topmost", G_CALLBACK(ui_main_topmost_cb), NULL);
	PRIVATE(ui_main)->ic = NULL;

	gtk_widget_hide (glade_xml_get_widget (PRIVATE(ui_main)->glade_xml, "main_menubar"));
#endif

        vbox = glade_xml_get_widget( PRIVATE(ui_main)->glade_xml,"main_vbox");

        PRIVATE(ui_main)->canvas =monkey_canvas_new();
	gtk_widget_show (GTK_WIDGET (PRIVATE(ui_main)->canvas));
        PRIVATE(ui_main)->main_image = 
                monkey_canvas_create_block_from_image(
                                                      PRIVATE(ui_main)->canvas,
                                                      DATADIR"/monkey-bubble/gfx/splash.svg",
                                                      640,480,
                                                      0,0);

        gtk_box_pack_end (GTK_BOX (vbox),
                          GTK_WIDGET(PRIVATE(ui_main)->canvas),
                          TRUE,
                          TRUE, 0);

	ui_manager = gtk_ui_manager_new ();

	PRIVATE (ui_main)->actions = gtk_action_group_new ("main");
	gtk_action_group_add_actions (PRIVATE (ui_main)->actions, entries,
				      G_N_ELEMENTS (entries), ui_main);
	gtk_ui_manager_insert_action_group (ui_manager, PRIVATE (ui_main)->actions, 0);

#ifdef MAEMO
	gtk_ui_manager_add_ui_from_string (ui_manager,
					   "<ui><popup name='main_menu' action='Game'>"
					     "<menuitem action='GameNew' />"
					     "<menuitem action='GameJoin' />"
					     "<menuitem action='GamePause' />"
					     "<menuitem action='ApplicationQuit' />"
					   "</popup></ui>",
					   -1,
					   &error);
#endif
#ifdef GNOME
	gtk_ui_manager_add_ui_from_string (ui_manager,
					   "<ui><popup name='game_menu' action='Game'>"
					     "<menuitem action='GameNew1Player' />"
					     "<menuitem action='GameNew2Player' />"
					     "<separator />"
					     "<menuitem action='GameNetworkNew' />"
					     "<menuitem action='GameNetworkJoin' />"
					     "<separator />"
					     "<menuitem action='GameSettings' />"
					     "<menuitem action='GameScores' />"
					     "<separator />"
					     "<menuitem action='GamePause' />"
					     "<menuitem action='GameResume' />"
					     "<menuitem action='GameStop' />"
					     "<menuitem action='GameQuit' />"
					   "</popup><popup name='help_menu'>"
					     "<menuitem action='HelpContent' />"
					     "<menuitem action='HelpAbout' />"
					   "</popup></ui>",
					   -1,
					   &error);
#endif

	if (error) {
		g_warning ("there was an error constructing the user interface: %s",
			   error->message);
		g_error_free (error);
		error = NULL;
	}

#ifdef MAEMO
	/* Setting menu */
	main_menu = gtk_ui_manager_get_widget (ui_manager, "/ui/main_menu");

	hildon_window_set_menu(HILDON_WINDOW(PRIVATE(ui_main)->window), GTK_MENU(main_menu));
#endif

#ifdef GNOME
        PRIVATE(ui_main)->menu = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"main_menubar");
        g_object_ref(PRIVATE(ui_main)->menu);
        kp = keyboard_properties_get_instance();

        PRIVATE(ui_main)->accel_group = g_object_ref (gtk_ui_manager_get_accel_group (ui_manager));
        gtk_window_add_accel_group(GTK_WINDOW(PRIVATE(ui_main)->window),
                                   PRIVATE(ui_main)->accel_group);

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"game_menu_menu");
        gtk_menu_set_accel_group( GTK_MENU(item),
                                  PRIVATE(ui_main)->accel_group);

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"new_1_player");
        g_signal_connect (item, "activate",
			  G_CALLBACK (new_1_player_game), ui_main);
        gtk_menu_item_set_accel_path( GTK_MENU_ITEM(item),
                                      ACCEL_PATH_NEW_1_PLAYER);
	gtk_action_set_accel_path (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameNew1Player"),
				   ACCEL_PATH_NEW_1_PLAYER);

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"new_2_players");
        g_signal_connect (item, "activate",
			  G_CALLBACK (new_2_player_game), ui_main);
        gtk_menu_item_set_accel_path( GTK_MENU_ITEM(item),
                                      ACCEL_PATH_NEW_2_PLAYERS);
	gtk_action_set_accel_path (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameNew2Player"),
				   ACCEL_PATH_NEW_2_PLAYERS);


        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"new_network_game");
        g_signal_connect (item, "activate",
			  G_CALLBACK (new_network_game), ui_main);

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"new_network_server");
        g_signal_connect (item, "activate",
			  G_CALLBACK (new_network_server), ui_main);

	item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml, "high_scores1");
	g_signal_connect (item, "activate",
			  G_CALLBACK (show_high_scores), ui_main);

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"pause_game");
        g_signal_connect (item, "activate",
			  G_CALLBACK (pause_game), ui_main);
        gtk_menu_item_set_accel_path( GTK_MENU_ITEM(item),
                                      ACCEL_PATH_PAUSE_GAME);
	gtk_action_set_accel_path (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GamePause"),
				   ACCEL_PATH_PAUSE_GAME);
	gtk_action_set_accel_path (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameResume"),
				   ACCEL_PATH_PAUSE_GAME);

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"stop_game");
        g_signal_connect (item, "activate",
			  G_CALLBACK (stop_game), ui_main);
        gtk_menu_item_set_accel_path( GTK_MENU_ITEM(item),
                                      ACCEL_PATH_STOP_GAME);
	gtk_action_set_accel_path (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameStop"),
				   ACCEL_PATH_STOP_GAME);

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"main_quit");
        g_signal_connect (item, "activate",
			  G_CALLBACK (quit_program), ui_main);
        gtk_menu_item_set_accel_path( GTK_MENU_ITEM(item),
                                      ACCEL_PATH_QUIT_GAME);
	gtk_action_set_accel_path (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameQuit"),
				   ACCEL_PATH_QUIT_GAME);

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"game_preferences");
        g_signal_connect (item, "activate",
			  G_CALLBACK (show_preferences_dialog), ui_main);

        g_signal_connect (PRIVATE (ui_main)->window, "delete-event",
			  G_CALLBACK (window_destroy_cb), NULL);

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (glade_xml_get_widget (PRIVATE (ui_main)->glade_xml, "help")),
				   gtk_ui_manager_get_widget (ui_manager, "/ui/help_menu"));
#endif

	g_object_unref (ui_manager);

        ui_main_enabled_games_item(ui_main ,TRUE);

        PRIVATE(ui_main)->game = NULL;

        ui_main_set_game( ui_main,NULL);
        PRIVATE(ui_main)->manager = NULL;

        gtk_widget_push_visual (gdk_rgb_get_visual ());
        gtk_widget_push_colormap (gdk_rgb_get_cmap ());

        gtk_widget_pop_visual ();
        gtk_widget_pop_colormap ();

        ui_main_draw_main(ui_main);

        PRIVATE(ui_main)->fullscreen = FALSE;

        gtk_window_set_policy (GTK_WINDOW (PRIVATE(ui_main)->window), TRUE, TRUE, FALSE);


        PRIVATE(ui_main)->sm = sound_manager_get_instance();
        sound_manager_play_music(PRIVATE(ui_main)->sm,MB_MUSIC_SPLASH);

        return ui_main;
}

static void ui_main_draw_main(UiMain * ui_main) {

        Layer * root_layer;
        monkey_canvas_clear( PRIVATE(ui_main)->canvas);
        root_layer = monkey_canvas_get_root_layer( PRIVATE(ui_main)->canvas);


        monkey_canvas_add_block(PRIVATE(ui_main)->canvas,
                                root_layer,
                                PRIVATE(ui_main)->main_image,
                                0,0);

        monkey_canvas_paint( PRIVATE(ui_main)->canvas);
}

MonkeyCanvas * ui_main_get_canvas(UiMain * ui_main) {
        return PRIVATE(ui_main)->canvas;
}

GtkWidget* ui_main_get_window(UiMain * ui_main) {
        return PRIVATE(ui_main)->window;
}


static void
ui_main_finalize (GObject* object)
{
	UiMain* self = UI_MAIN (object);

	g_object_unref (PRIVATE (self)->actions);

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (G_OBJECT_CLASS (parent_class)->finalize) (object);
        }
}

static void ui_main_class_init (UiMainClass *klass) {
        GObjectClass* object_class;
    
        parent_class = g_type_class_peek_parent(klass);
        object_class = G_OBJECT_CLASS(klass);
        object_class->finalize = ui_main_finalize;
}

static void ui_main_init (UiMain *ui_main) {
        ui_main->private = g_new0(UiMainPrivate, 1);
}



static void ui_main_stop_game(UiMain * ui_main);

static void ui_main_new_1_player_game(UiMain * ui_main) {

        GameManager * manager;

        manager = GAME_MANAGER(
                               game_1_player_manager_new(PRIVATE(ui_main)->window, 
                                                         PRIVATE(ui_main)->canvas));

        ui_main_set_game_manager(ui_main,manager);
}

#ifdef GNOME
static void ui_main_new_2_player_game(UiMain * ui_main) {

        GameManager * manager;




        manager = GAME_MANAGER(
                               game_2_player_manager_new(PRIVATE(ui_main)->window, 
                                                         PRIVATE(ui_main)->canvas));

        ui_main_set_game_manager(ui_main,manager);
}
#endif


void ui_main_set_game_manager(UiMain * ui_main,GameManager * manager) {
        Layer * root_layer;


        if( PRIVATE(ui_main)->game != NULL) {
                ui_main_stop_game(ui_main);
        }
	 
        root_layer = monkey_canvas_get_root_layer(PRIVATE(ui_main)->canvas);
	 
        monkey_canvas_clear(PRIVATE(ui_main)->canvas);
	 

	 
        PRIVATE(ui_main)->manager = manager;

        g_object_ref( manager);
        game_manager_start(manager);
	 
	 
        sound_manager_play_music(PRIVATE(ui_main)->sm,MB_MUSIC_GAME);
	 
}

void
ui_main_enabled_games_item (UiMain  * ui_main,
			    gboolean  enabled)
{
        GtkWidget * item;

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"pause_game");
        gtk_widget_set_sensitive(item,!enabled);

        item = gtk_bin_get_child( GTK_BIN(item));
        gtk_label_set_text( GTK_LABEL(item), _("Pause game"));

        item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"stop_game");
        gtk_widget_set_sensitive(item,!enabled);

#ifdef GNOME
	gtk_action_set_sensitive (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameStop"),
				  !enabled);
	gtk_action_set_sensitive (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GamePause"),
				  !enabled);
	gtk_action_set_visible   (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GamePause"),
				  TRUE);
	gtk_action_set_visible   (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameResume"),
				  FALSE);
#endif
}

static void
new_1_player_game (GtkAction* action,
		   UiMain   * ui_main)
{
#ifdef MAEMO
	state_clear();
#endif

        ui_main_new_1_player_game(ui_main);
}

#ifdef GNOME
static void
new_2_player_game (GtkAction* action,
		   UiMain   * ui_main)
{
        ui_main_new_2_player_game(ui_main);
}
#endif

static void
quit_program (GtkAction* action,
	      UiMain   * uimain)
{
	// FIXME: use gtk_main_quit()
        exit(0);
}

static void
pause_game (GtkAction* action,
	    UiMain   * ui_main)
{
        if( PRIVATE(ui_main)->game != NULL) {
                if( game_get_state( PRIVATE(ui_main)->game) == GAME_PAUSED) {
                        game_pause(PRIVATE(ui_main)->game,FALSE);
                } else {
                        game_pause(PRIVATE(ui_main)->game,TRUE);
                }
        }
}

static void ui_main_stop_game(UiMain * ui_main) {
      
        game_manager_stop(PRIVATE(ui_main)->manager);


        g_object_unref( PRIVATE(ui_main)->manager );
        PRIVATE(ui_main)->manager = NULL;

        monkey_canvas_clear(PRIVATE(ui_main)->canvas);

        sound_manager_play_music(PRIVATE(ui_main)->sm,MB_MUSIC_SPLASH);
        ui_main_draw_main(ui_main);
}

#ifdef GNOME
static void
stop_game (GtkAction* action,
	   UiMain   * uimain)
{
        ui_main_stop_game(uimain);
}

Block * ui_main_get_main_image(UiMain *ui_main) {
        return(PRIVATE(ui_main)->main_image);
}
#endif

void ui_main_set_game(UiMain *ui_main, Game *game) {

        if( PRIVATE(ui_main)->game != NULL ) {

                g_signal_handlers_disconnect_matched(  G_OBJECT( PRIVATE(ui_main)->game ),
                                                       G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,ui_main);
                g_object_unref(PRIVATE(ui_main)->game);

                ui_main_enabled_games_item(ui_main,TRUE);

    
        }

        PRIVATE(ui_main)->game = game;
        if( game != NULL) {
                g_object_ref(PRIVATE(ui_main)->game);

                ui_main_enabled_games_item(ui_main,FALSE);

                g_signal_connect( G_OBJECT( PRIVATE(ui_main)->game),
                                  "state-changed",G_CALLBACK( ui_main_game_changed),
                                  ui_main);
        }
}

void
ui_main_game_changed(Game  * game,
                     UiMain* ui_main)
{
        GtkWidget * item;

        if( game_get_state(game) == GAME_PAUSED ) {
                item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"pause_game");

                item = gtk_bin_get_child( GTK_BIN(item));
                gtk_label_set_text( GTK_LABEL(item), _("Resume game"));

#ifdef GNOME
		gtk_action_set_visible (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameResume"),
					TRUE);
		gtk_action_set_visible (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GamePause"),
					FALSE);
#endif
        } else if( game_get_state(game) == GAME_PLAYING ) {
                item = glade_xml_get_widget(PRIVATE(ui_main)->glade_xml,"pause_game");

                item = gtk_bin_get_child( GTK_BIN(item));
                gtk_label_set_text( GTK_LABEL(item), _("Pause game"));

#ifdef GNOME
		gtk_action_set_visible (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GameResume"),
					FALSE);
		gtk_action_set_visible (gtk_action_group_get_action (PRIVATE (ui_main)->actions, "GamePause"),
					TRUE);
#endif
        } else if( game_get_state(game) == GAME_STOPPED ) {
        }
}

#ifdef MAEMO
static void network_connected(ConIcConnection *cnx, ConIcConnectionEvent *event, gpointer user_data)
{
        UiNetworkClient  * ngl;

	switch (con_ic_connection_event_get_status(event)) {
		case CON_IC_STATUS_CONNECTED:
		  ngl = ui_network_client_new();
		  break;
		default:
		  break;
	}
}
#endif

static void
new_network_game(GtkAction* action,
		 UiMain   * uimain)
{
#ifdef MAEMO
	PRIVATE(uimain)->ic = con_ic_connection_new();
	g_signal_connect(PRIVATE(uimain)->ic, "connection-event", (GCallback)network_connected, NULL);
	con_ic_connection_connect(PRIVATE(uimain)->ic, CON_IC_CONNECT_FLAG_NONE);
#endif

#ifdef GNOME
        UiNetworkClient  * ngl;
        ngl = ui_network_client_new();
#endif
}


#ifdef GNOME
static void
new_network_server (GtkAction* action,
		    UiMain   * ui_main)
{
        UiNetworkServer  * ngl;
        NetworkSimpleServer * server;

        server = network_simple_server_new();

        if( network_simple_server_start( server ) == TRUE) {
                ngl = ui_network_server_new(server);
        } else {
                GladeXML * gx;

                g_object_unref(server);
                gx = glade_xml_new(DATADIR"/monkey-bubble/glade/monkey-bubble.glade",
                                   "create_server_warning",
                                   NULL);
                glade_xml_signal_autoconnect(gx);
                gtk_widget_show( glade_xml_get_widget(gx,"create_server_warning"));
                g_object_unref(gx);
        }
}

static void
show_high_scores (GtkAction* action,
		  UiMain   * ui_main)
{
	GtkWidget* dialog = NULL;
	gint       number;
	gchar    **names;
	gfloat   * scores;
	time_t   * times;

	// FIXME: set the parent window and transient mode
	number = gnome_score_get_notable(PACKAGE, NULL,
					 &names, &scores, &times);
	dialog = gnome_scores_new(number, names, scores, times, FALSE);
	gnome_scores_set_logo_pixmap(GNOME_SCORES(dialog), DATADIR "/monkey-bubble/gfx/monkey.png");
	gtk_widget_show(dialog);
}

static void
show_preferences_dialog (GtkAction* action,
			 UiMain   * ui_main)
{
        if( PRIVATE(ui_main)->game != NULL && game_get_state(PRIVATE(ui_main)->game) != GAME_PAUSED ) {
                game_pause(PRIVATE(ui_main)->game,TRUE);      
        }
  
        keyboard_properties_show( keyboard_properties_get_instance(),GTK_WINDOW(PRIVATE(ui_main)->window));
}

static void
about (GtkAction* action,
       UiMain   * ui_main)
{

        const gchar* authors[] = {
                "Laurent Belmonte <laurent.belmonte@aliacom.fr>",
                "Sven Herzberg <herzi@gnome-de.org>",
                "Thomas Cataldo <thomas.cataldo@aliacom.fr>",
                NULL
        };
        const gchar* documenters [] = {
                "Thomas Cataldo <thomas.cataldo@aliacom.fr>",
                NULL
        };
        const gchar* translator_credits = _("translator_credits");
        GdkPixbuf	*logo = gdk_pixbuf_new_from_file(DATADIR"/monkey-bubble/gfx/monkey.png",NULL);
	
        gtk_widget_show (
                         gnome_about_new (
                                          PACKAGE,
                                          VERSION,
                                          "Copyright (C) 2003 - Laurent Belmonte <laurent.belmonte@aliacom.fr>",
                                          _("Monkey Bubble is an Arcade Game for the GNOME Desktop Environment. Simply remove all Bubbles by the creation of unicolor triplets."),
                                          authors,
                                          documenters,
                                          strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
                                          logo)
                         );

        g_object_unref( logo);
}

static void
show_help_content (GtkAction* action,
		   UiMain   * ui_main)
{
        GError *err = NULL;

        gnome_help_display ("monkey-bubble", NULL, &err);

        if (err) {
                ui_main = ui_main_get_instance();
                show_error_dialog (GTK_WINDOW (PRIVATE(ui_main)->window),
                                   _("There was an error displaying help: %s"),
                                   err->message);
                g_error_free (err);
        }
}

static void show_error_dialog (GtkWindow *transient_parent,
                               const char *message_format, ...) {
        char *message;
        va_list args;
    
        if (message_format)	{
                va_start (args, message_format);
                message = g_strdup_vprintf (message_format, args);
                va_end (args);
        } else {
                message = NULL;
        }
    
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new (transient_parent,
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         message);
    
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (gtk_widget_destroy), NULL);
    
        gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
    
        gtk_widget_show_all (dialog);
}
#endif

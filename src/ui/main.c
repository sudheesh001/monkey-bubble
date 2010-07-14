
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "sound-manager.h"
#include "bubble.h"
#include "color.h"
#include "playground.h"
#include "monkey.h"
#include "ui-main.h"
#include "input-manager.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#ifdef GNOME
#include <libgnome/gnome-score.h>
#include <libgnomeui/gnome-ui-init.h>
#endif
#include <glib/gthread.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef MAEMO
#include <stdlib.h>  /* exit() */
#include <libosso.h>

#include "state.h"
#include "global.h"

#include "game-1-player.h"

#define APPNAME "monkey_bubble"
#define APPVERSION "0.0.1"
#define SERVICENAME "org.gnome.Games.MonkeyBubble"

struct GlobalData global;

static void _top_cb(const char *args, gpointer data)
{
}

static void _hw_cb(osso_hw_state_t * state, gpointer data)
{
       if(state->shutdown_ind)
       {
               state_save();
               gtk_main_quit();
       }
}

osso_context_t *osso_init(void)
{
    osso_context_t *osso =
        osso_initialize(SERVICENAME, APPVERSION, TRUE, NULL);

    if (OSSO_OK != osso_application_set_top_cb(osso, _top_cb, NULL))
        return NULL;

    if (OSSO_OK != osso_hw_set_event_cb(osso, NULL, _hw_cb, NULL))
        return NULL;

    return osso;
}
#endif

int main(int  argc, char **argv)
{
  UiMain * ui_main;
  GtkWidget * window;
  SoundManager * manager;
  GError* error = NULL;
  GOptionContext * context;

#ifdef MAEMO
  setlocale(LC_ALL, "");
  bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  g_thread_init(NULL);

  context = g_option_context_new ("");
  g_option_context_set_summary (context, _("Monkey Bubble is a Bust'a'Move clone for GNOME"));
  g_option_context_add_group   (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
    return 1;
  }
  g_option_context_free (context);

#ifdef GNOME
  if(gnome_score_init(PACKAGE)) {
	g_message("You'll have to play without highscore support");
  }
#endif
  
#ifdef MAEMO
  global.osso = osso_init();
  if (!global.osso) {
       perror("osso_init");
       exit(1);
  }
  global.game = NULL;
#endif

#ifdef GNOME
  /* to get help working */
  gnome_program_init (PACKAGE, VERSION,
		      LIBGNOMEUI_MODULE,
		      argc, argv,
		      GNOME_PROGRAM_STANDARD_PROPERTIES,
		      NULL);
#endif

  gtk_window_set_default_icon_name ("monkey-bubble");

  manager = sound_manager_get_instance();
  sound_manager_init(manager,TRUE);

  ui_main = ui_main_get_instance();

  window = ui_main_get_window(ui_main);

  mb_input_manager_instance_set_window(window);
  gdk_rgb_init ();

  gtk_widget_show (window);

#ifdef MAEMO
  if (state.game == 1) {
       continue_game();
  }
#endif

  gtk_main ();


  return (0);
}


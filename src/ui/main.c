
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

#include <esd.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <glib/gi18n.h>
#include <glib/gthread.h>
#include <libgnome/gnome-score.h>
#include <libgnomeui/gnome-ui-init.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

int main(int  argc, char **argv)
{
  UiMain * ui_main;
  GtkWidget * window;
  SoundManager * manager;
  GError* error = NULL;
  GOptionContext * context;
#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  g_thread_init(NULL);

  context = g_option_context_new ("");
  g_option_context_set_summary (context, _("Monkey Bubble is a Bust'a'Move clone for GNOME"));
  g_option_context_add_group   (context, gtk_get_option_group (TRUE));
  g_option_context_add_group   (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
    return 1;
  }
  g_option_context_free (context);

  if(gnome_score_init(PACKAGE)) {
	g_message("You'll have to play without highscore support");
  }

  /* to get help working */
  gnome_program_init (PACKAGE, VERSION,
		      LIBGNOMEUI_MODULE,
		      argc, argv,
		      GNOME_PROGRAM_STANDARD_PROPERTIES,
		      NULL);

  gtk_window_set_default_icon_name ("monkey-bubble");

  manager = sound_manager_get_instance();
  sound_manager_init(manager,TRUE);

  ui_main = ui_main_get_instance();

  window = ui_main_get_window(ui_main);

  mb_input_manager_instance_set_window(window);
  gdk_rgb_init ();

  gtk_widget_show_all (window);

  gtk_main ();


  return (0);
}


/* This file is part of monkey-bubble
 *
 * Copyright (C) 2010  Sven Herzberg
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <libgnomeui/gnome-scores.h>

#define N_SCORES 10

int
main (int   argc,
      char**argv)
{
  GtkWidget* dialog;
  gchar    * names[N_SCORES];
  double     scores[N_SCORES];
  time_t     times[N_SCORES];

  gtk_init (&argc, &argv);

  dialog = gnome_scores_new (G_N_ELEMENTS (names),
                             names,
                             scores,
                             times,
                             FALSE);
}

/* vim:set et sw=2 cino=t0,f0,(0,{s,>2s,n-1s,^-1s,e2s: */

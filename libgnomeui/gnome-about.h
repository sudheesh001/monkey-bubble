/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* gnome-about.h - An about box widget for gnome.

   Copyright (C) 2001 CodeFactory AB
   Copyright (C) Anders Carlsson <andersca@codefactory.se>

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Anders Carlsson <andersca@codefactory.se>
*/

#ifndef __GNOME_ABOUT_H__
#define __GNOME_ABOUT_H__

#include <gtk/gtkdialog.h>

G_BEGIN_DECLS

#define GNOME_TYPE_ABOUT            (gnome_about_get_type ())
#define GNOME_ABOUT(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), GNOME_TYPE_ABOUT, GnomeAbout))
#define GNOME_ABOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ABOUT, GnomeAboutClass))
#define GNOME_IS_ABOUT(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), GNOME_TYPE_ABOUT))
#define GNOME_IS_ABOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ABOUT))
#define GNOME_ABOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_ABOUT, GnomeAboutClass))

typedef struct _GnomeAbout GnomeAbout;
typedef struct _GnomeAboutClass GnomeAboutClass;

struct _GnomeAbout {
	GtkDialog parent_instance;

	gchar *name;
	gchar *version;
	gchar *copyright;
	gchar *comments;
	
	GSList *authors;
	GSList *translators;
	GSList *documenters;

	gint displaying_state;
	
	GtkWidget *drawing_area;
	
	GdkPixbuf *background_pixbuf;
	GdkPixbuf *rendered_background_pixbuf;
	gdouble gradient_start_opacity, gradient_end_opacity;
	gdouble gradient_start_position, gradient_end_position;

	GdkPixbuf *logo_pixbuf;
};

struct _GnomeAboutClass {
	GtkDialogClass parent_class;
};

GType gnome_about_get_type (void);

GtkWidget *gnome_about_new (const gchar  *name,
			    const gchar  *version,
			    const gchar  *copyright,
			    const gchar  *comments,
			    const gchar **authors,
			    const gchar **documenters,
			    const gchar  *translator_credits,
			    GdkPixbuf    *logo_pixbuf);

G_END_DECLS

#endif /* __GNOME_ABOUT_H__ */

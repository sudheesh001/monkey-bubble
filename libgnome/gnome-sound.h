/*
 * Copyright (C) 1997-1998 Stuart Parmenter and Elliot Lee
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef __GNOME_SOUND_H__
#define __GNOME_SOUND_H__ 1

#include <glib/gmacros.h>
#include <glib/gerror.h>
#include <gobject/gtypemodule.h>
#include <libgnome/gnome-sound-driver.h>

G_BEGIN_DECLS

typedef struct _GnomeSoundPlugin             GnomeSoundPlugin;

struct _GnomeSoundPlugin {
    gulong version;
    const gchar *name;

    GnomeSoundDriver * (*init) (GTypeModule *module,
				const gchar *backend_args,
				GError **error);

    void (*shutdown) (void);
};

GnomeSoundDriver *gnome_sound_get_default_driver (void);

GnomeSoundDriver *gnome_sound_init (const gchar *driver_name,
				    const gchar *backend_args,
				    GError **error);

gboolean gnome_sound_enabled (void);

G_END_DECLS

#endif /* __GNOME_SOUND_H__ */

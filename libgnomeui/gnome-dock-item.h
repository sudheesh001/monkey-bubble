/*
 * !!! MASTER CAUTION !!!!
 *
 * These files have been moved to libbonoboui and renamed to bonobo-dock*.[ch].
 * We'll provide a compatibility wrapper with #defines in libgnomecompat soon, but
 * at the moment I don't want to break the build here since libbonoboui doesn't work yet.
 *
 * If you do any changes in these files here, your work will be lost !
 */

/* WARNING ____ IMMATURE API ____ liable to change */

/* gnome-dock-item.h
 *
 * Copyright (C) 1998 Ettore Perazzoli
 * Copyright (C) 1998 Elliot Lee
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * All rights reserved.
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
/*
  @NOTATION@
*/

#ifndef _GNOME_DOCK_ITEM_H
#define _GNOME_DOCK_ITEM_H

#include <gdk/gdk.h>
#include <gtk/gtkbin.h>



G_BEGIN_DECLS

#define GNOME_TYPE_DOCK_ITEM            (gnome_dock_item_get_type())
#define GNOME_DOCK_ITEM(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_DOCK_ITEM, GnomeDockItem))
#define GNOME_DOCK_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DOCK_ITEM, GnomeDockItemClass))
#define GNOME_IS_DOCK_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_DOCK_ITEM))
#define GNOME_IS_DOCK_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DOCK_ITEM))
#define GNOME_DOCK_ITEM_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_DOCK_ITEM, GnomeDockItemClass))

typedef enum
{
  GNOME_DOCK_ITEM_BEH_NORMAL = 0,
  GNOME_DOCK_ITEM_BEH_EXCLUSIVE = 1 << 0,
  GNOME_DOCK_ITEM_BEH_NEVER_FLOATING = 1 << 1,
  GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL = 1 << 2,
  GNOME_DOCK_ITEM_BEH_NEVER_HORIZONTAL = 1 << 3,
  GNOME_DOCK_ITEM_BEH_LOCKED = 1 << 4
  /* MAINT: Update the size of the bit field in the GnomeDockItem structure if you add items to this */
} GnomeDockItemBehavior;

/* obsolete, for compatibility; don't use */
#define GNOME_DOCK_ITEM_BEH_NEVER_DETACH GNOME_DOCK_ITEM_BEH_NEVER_FLOATING

#define GNOME_DOCK_ITEM_NOT_LOCKED(x) (! (GNOME_DOCK_ITEM(x)->behavior \
                                          & GNOME_DOCK_ITEM_BEH_LOCKED))

typedef struct _GnomeDockItem        GnomeDockItem;
typedef struct _GnomeDockItemPrivate GnomeDockItemPrivate;
typedef struct _GnomeDockItemClass   GnomeDockItemClass;

struct _GnomeDockItem
{
  GtkBin bin;

  gchar                *name;

  GdkWindow            *bin_window; /* parent window for children */
  GdkWindow            *float_window;
  GtkShadowType         shadow_type;

  /* Start drag position (wrt widget->window).  */
  gint16                  dragoff_x, dragoff_y;

  /* Position of the floating window.  */
  gint16                  float_x, float_y;

  GnomeDockItemBehavior behavior : 5;
  GtkOrientation        orientation : 1;

  guint                 float_window_mapped : 1;
  guint                 is_floating : 1;
  guint                 in_drag : 1;
  /* If TRUE, the pointer must be grabbed on "map_event".  */
  guint                 grab_on_map_event : 1;

  /*< private >*/
  GnomeDockItemPrivate *_priv;
};

struct _GnomeDockItemClass
{
  GtkBinClass parent_class;

  void (* dock_drag_begin) (GnomeDockItem *item);
  void (* dock_drag_motion) (GnomeDockItem *item, gint x, gint y);
  void (* dock_drag_end) (GnomeDockItem *item);
  void (* dock_detach) (GnomeDockItem *item);
  void (* orientation_changed) (GnomeDockItem *item, GtkOrientation new_orientation);
};

/* Public methods.  */
guint          gnome_dock_item_get_type        (void) G_GNUC_CONST;
GtkWidget     *gnome_dock_item_new             (const gchar *name,
                                                GnomeDockItemBehavior behavior);
void           gnome_dock_item_construct       (GnomeDockItem *new_dock_item,
						const gchar *name,
						GnomeDockItemBehavior behavior);

GtkWidget     *gnome_dock_item_get_child       (GnomeDockItem *dock_item);

char          *gnome_dock_item_get_name        (GnomeDockItem *dock_item);

void           gnome_dock_item_set_shadow_type (GnomeDockItem *dock_item,
                                                GtkShadowType type);

GtkShadowType  gnome_dock_item_get_shadow_type (GnomeDockItem *dock_item);
 
gboolean       gnome_dock_item_set_orientation (GnomeDockItem *dock_item,
                                                GtkOrientation orientation);

GtkOrientation gnome_dock_item_get_orientation (GnomeDockItem *dock_item);

GnomeDockItemBehavior
               gnome_dock_item_get_behavior    (GnomeDockItem *dock_item);

/* Private methods.  */
gboolean       gnome_dock_item_detach          (GnomeDockItem *item,
                                                gint x, gint y);
                                               
void           gnome_dock_item_attach          (GnomeDockItem *item,
                                                GtkWidget *parent,
                                                gint x, gint y);
                                               
void           gnome_dock_item_grab_pointer    (GnomeDockItem *item);
                                               
void           gnome_dock_item_drag_floating   (GnomeDockItem *item,
                                                gint x, gint y);

void           gnome_dock_item_handle_size_request
                                               (GnomeDockItem *item,
                                                GtkRequisition *requisition);

void           gnome_dock_item_get_floating_position
                                               (GnomeDockItem *item,
                                                gint *x, gint *y);

G_END_DECLS

#endif /* _GNOME_DOCK_ITEM_H */

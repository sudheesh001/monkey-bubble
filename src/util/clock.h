/* clock.h
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

#ifndef CLOCK_H
#define CLOCK_H

#include <gtk/gtk.h>
G_BEGIN_DECLS


#define TYPE_MB_CLOCK            (mb_clock_get_type())

#define MB_CLOCK(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), TYPE_MB_CLOCK,MbClock))
#define MB_MB_CLOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MB_CLOCK,MbClockClass))
#define IS_MB_CLOCK(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_MB_CLOCK))
#define IS_MB_MB_CLOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MB_CLOCK))
#define MB_MB_CLOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MB_CLOCK, MbClockClass))

typedef struct MbClockPrivate MbClockPrivate;



typedef struct {
  GObject parent_instance;
  MbClockPrivate * private;
} MbClock;

typedef struct {
  GObjectClass parent_class;
} MbClockClass;


GType mb_clock_get_type(void);

MbClock * mb_clock_new(void);

void mb_clock_start(MbClock * clock);
void mb_clock_pause(MbClock * clock,gboolean paused);
gint mb_clock_get_time(MbClock * clock);

G_END_DECLS





#endif

/* this file is part of monkey-bubble
 *
 * AUTHORS
 *       Laurent Belminte        <laurent.belmonte@gmail.com>
 *
 * Copyright (C) 2007 Laurent Belmonte
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <glib-object.h>

#ifndef _MB_UI__NET_H
#define _MB_UI__NET_H

G_BEGIN_DECLS typedef struct _MbUiNet MbUiNet;
typedef struct _MbUiNetClass MbUiNetClass;

GType mb_ui_net_get_type(void);
MbUiNet *mb_ui_net_new();
void mb_ui_net_host(MbUiNet * self, GError ** error);
void mb_ui_net_connect(MbUiNet * self);
#define MB_UI_TYPE_NET			(mb_ui_net_get_type())
#define MB_UI_NET(object)		(G_TYPE_CHECK_INSTANCE_CAST((object), MB_UI_TYPE_NET, MbUiNet))
#define MB_UI_NET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), MB_UI_TYPE_NET, MbUiNetClass))
#define MB_UI_IS_NET(object)		(G_TYPE_CHECK_INSTANCE_TYPE((object), MB_UI_TYPE_NET))
#define MB_UI_IS_NET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), MB_UI_TYPE_NET))
#define MB_UI_NET_GET_CLASS(object)	(G_TYPE_INSTANCE_GET_CLASS((object), MB_UI_TYPE_NET, MbUiNetClass))

struct _MbUiNet {
	GObject base_instance;
};

struct _MbUiNetClass {
	GObjectClass base_class;

	/* signals */
};

G_END_DECLS
#endif				/* !_MB_UI::_NET_H */

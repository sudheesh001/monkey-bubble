/* monkey-message-handler.h - 
 * Copyright (C) 2004 Laurent Belmonte <lolo3d@tuxfamily.org>
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
#ifndef _MONKEY_MESSAGE_HANDLER_H_
#define _MONKEY_MESSAGE_HANDLER_H_

#include <glib-object.h>
#include "color.h"

G_BEGIN_DECLS

#define TYPE_MONKEY_MESSAGE_HANDLER   (monkey_message_handler_get_type())

#define MONKEY_MESSAGE_HANDLER(object)(G_TYPE_CHECK_INSTANCE_CAST((object),TYPE_MONKEY_MESSAGE_HANDLER,MonkeyMessageHandler))
#define MONKEY_MESSAGE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_MONKEY_MESSAGE_HANDLER,MonkeyMessageHandlerClass))
#define IS_MONKEY_MESSAGE_HANDLER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), TYPE_MONKEY_MESSAGE_HANDLER))
#define IS_MONKEY_MESSAGE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_MONKEY_MESSAGE_HANDLER))
#define MONKEY_MESSAGE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_MONKEY_MESSAGE_HANDLER, MonkeyMessageHandlerClass))

typedef struct MonkeyMessageHandlerPrivate MonkeyMessageHandlerPrivate;

typedef struct {
  GObject parent_instance;
  MonkeyMessageHandlerPrivate * private;
} MonkeyMessageHandler;

typedef struct {
  GObjectClass parent_class;
  void (* recv_shoot)         (MonkeyMessageHandler * mmh,
			       guint32 monkey_id,
			       guint32 time,
			       gfloat angle);

  void (* recv_add_bubble)    (MonkeyMessageHandler * mmh,
			       guint32 monkey_id,
			       Color color);

  void (* recv_winlost)       (MonkeyMessageHandler * mmh,
			       guint32 monkey_id,
			       guint8 win_lost);

  void (* recv_start)         (MonkeyMessageHandler * mmh);

  void (* recv_message)          (MonkeyMessageHandler * mmh,
				  guint32 client_id,
				  gchar * message);

  void (* connection_closed)         (MonkeyMessageHandler * mmh);

} MonkeyMessageHandlerClass;


GType monkey_message_handler_get_type(void);
MonkeyMessageHandler * monkey_message_handler_new(int sock);

void monkey_message_handler_join(MonkeyMessageHandler * mnh);
void monkey_message_handler_start_listening(MonkeyMessageHandler * mmh);
void monkey_message_handler_send_message (MonkeyMessageHandler * mmh,
					  guint32 client_id,
					  const gchar * message,
					  guint size);

void monkey_message_handler_send_add_bubble  (MonkeyMessageHandler * mmh,
					      guint32 monkey_id,
					      Color color);

void monkey_message_handler_send_shoot       (MonkeyMessageHandler * mmh,
					      guint32 monkey_id,
					      guint32 time,
					      gfloat angle);

void monkey_message_handler_send_winlost     (MonkeyMessageHandler * mmh,
					      guint32 monkey_id,
					      guint8 win_lost);

void monkey_message_handler_send_start       (MonkeyMessageHandler * mmh);

G_END_DECLS

#endif

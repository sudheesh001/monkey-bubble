NULL=


INCLUDES = \
	$(MONKEY_SRV_CFLAGS) \
	-I$(top_srcdir)/src/monkey \
	-I$(top_srcdir)/src/view \
	-I$(top_srcdir)/src/ui \
	-I$(top_srcdir)/src/util \
	-I$(top_srcdir)/src \
	-g -O2 -Wall -Werror -D_REENTRANT  -D_DEBUG_ -D_TIME_DEBUG_ \
	$(NULL)
# -g -O2 -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror -D_REENTRANT  -D_TIME_DEBUG_ -D_DEBUG_


bin_PROGRAMS = monkey-srv

noinst_LIBRARIES = libmonkeynet.a

marshal_sources = \
	monkey-net-marshal.h \
	monkey-net-marshal.c \
	$(NULL)

monkey_srv_SOURCES = \
	main.c 
	$(NULL)

monkey_srv_LDFLAGS =

monkey_srv_LDADD = \
	$(MONKEY_SRV_LIBS) \
	libmonkeynet.a \
	../monkey/libmonkey.a \
	../util/libmonkeyutil.a 

$(libmonkeynet_a_OBJECTS): $(marshal_sources)

libmonkeynet_a_SOURCES = \
	message-handler.c message-handler.h \
	simple-server.h simple-server.c \
	client.h client.c \
	game-manager-proxy.h game-manager-proxy.c \
	player.h player.c \
	game.h game.c \
	game-manager.h game-manager.c \
	mb-net-connection.h mb-net-connection.c \
	mb-net-message.h mb-net-message.c \
	mb-net-holders.h mb-net-holders.c \
	mb-net-handler.h mb-net-handler.c \
	mb-net-abstract-handler.h mb-net-abstract-handler.c \
	mb-net-server-handler.h mb-net-server-handler.c \
	mb-net-server.h mb-net-server.c \
	mb-net-game.h mb-net-game.c \
	mb-net-game-handler.h mb-net-game-handler.c \
	$(marshal_sources)

monkey-net-marshal.c monkey-net-marshal.h: monkey-marshal.list
	glib-genmarshal --prefix=monkey_net_marshal --body $^ > monkey-net-marshal.c
	glib-genmarshal --prefix=monkey_net_marshal --header $^ > monkey-net-marshal.h

CLEANFILES = $(marshal_sources)

EXTRA_DIST = monkey-marshal.list



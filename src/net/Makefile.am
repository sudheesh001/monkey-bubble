NULL=


INCLUDES = \
	$(MONKEY_SRV_CFLAGS) \
	-I$(top_srcdir)/src/monkey \
	-I$(top_srcdir)/src/view \
	-I$(top_srcdir)/src/ui \
	-I$(top_srcdir)/src/util \
	-g -O2 -Wall -Werror -D_REENTRANT  -D_DEBUG_ -D_TIME_DEBUG_ \
	$(NULL)
# -g -O2 -Wall -Wstrict-prototypes -Wmissing-prototypes -Werror -D_REENTRANT  -D_TIME_DEBUG_ -D_DEBUG_


noinst_PROGRAMS = monkey-srv test

noinst_LIBRARIES = libmonkeynet.a

marshal_sources = \
	monkey-net-marshal.h \
	monkey-net-marshal.c \
	$(NULL)

monkey_srv_SOURCES = \
	mn-game-manager.c mn-game-manager.h \
	monkey-message-handler.c monkey-message-handler.h \
	monkey-network-game.c monkey-network-game.h \
	main.c \
	$(marshal_sources) \
	$(NULL)

monkey_srv_LDFLAGS =

monkey_srv_LDADD = \
	$(MONKEY_SRV_LIBS) \
	../monkey/libmonkey.a \
	../util/libmonkeyutil.a

$(monkey_srv_OBJECTS): $(marshal_sources)


test_SOURCES = \
	mn-game-manager.c mn-game-manager.h \
	monkey-message-handler.c monkey-message-handler.h \
	monkey-network-game.c monkey-network-game.h \
	test-client.c \
	$(marshal_sources) \
	$(NULL)

test_LDFLAGS =

test_LDADD = \
	$(MONKEY_SRV_LIBS) \
	../monkey/libmonkey.a \
	../util/libmonkeyutil.a

$(test_OBJECTS): $(marshal_sources)


libmonkeynet_a_SOURCES = \
	mn-game-manager.c mn-game-manager.h \
	monkey-message-handler.c monkey-message-handler.h \
	monkey-network-game.c monkey-network-game.h \
	$(marshal_sources)

monkey-net-marshal.c monkey-net-marshal.h: monkey-marshal.list
	glib-genmarshal --prefix=monkey_net_marshal --body $^ > monkey-net-marshal.c
	glib-genmarshal --prefix=monkey_net_marshal --header $^ > monkey-net-marshal.h

CLEANFILES = $(marshal_sources)

EXTRA_DIST = monkey-marshal.list



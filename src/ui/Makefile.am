
NULL=

INCLUDES = \
	$(UI_CFLAGS) 					\
	-DDATADIR=\""$(datadir)"\"                      \
        -DLIBDIR=\""$(libdir)"\"                        \
        -DSYSCONFDIR=\""$(sysconfdir)"\"                \
        -DPREFIX=\""$(prefix)"\" 			\
        -I$(top_srcdir)/src/monkey			\
	-I$(top_srcdir)/src/view 			\
	-I$(top_srcdir)/src/audio			\
	-I$(top_srcdir)/src/util			\
	-I$(top_srcdir)/src/net 			\
	-I$(top_srcdir)/src/input                       \
	-Wall -Werror \
	$(PLATFORM_CFLAGS) \
	$(NULL)


bin_PROGRAMS = monkey-bubble

monkey_bubble_SOURCES = \
	game.c game.h \
	game-1-player.c game-1-player.h \
	game-network-player.c game-network-player.h \
	game-2-player.c game-2-player.h \
	game-manager.c game-manager.h \
	game-1-player-manager.c game-1-player-manager.h \
	game-2-player-manager.c game-2-player-manager.h \
	game-network-player-manager.c game-network-player-manager.h \
	main.c \
	game-sound.c game-sound.h \
	keyboard-properties.c keyboard-properties.h \
	ui-network-client.c ui-network-client.h \
	ui-network-server.c ui-network-server.h \
	ui-main.c ui-main.h \
	$(NULL)


monkey_bubble_LDFLAGS=


monkey_bubble_LDADD =  \
	$(UI_LIBS) \
	$(PLATFORM_LIBS) \
	../monkey/libmonkey.a \
	../view/libmonkeyview.a \
	../util/libmonkeyutil.a \
	../net/libmonkeynet.a \
	../audio/libmonkeyaudio.a \
	../input/libmbinput.a


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
	-Wall -Werror


bin_PROGRAMS = monkey-bubble

EGGFILES=			\
	eggaccelerators.c	\
	eggaccelerators.h	\
	eggcellrendererkeys.h 	\
	eggcellrendererkeys.c

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
	keyboard-properties.c keyboard-properties.h \
	network-game-launcher.c network-game-launcher.h \
	network-server-launcher.c network-server-launcher.h \
	ui-main.c ui-main.h \
	$(EGGFILES)


EGGDIR=$(srcdir)/../../../libegg/libegg

regenerate-built-sources:
	EGGFILES="$(EGGFILES)" EGGDIR="$(EGGDIR)" $(srcdir)/update-from-egg.sh

monkey_bubble_LDFLAGS=


monkey_bubble_LDADD =  \
	$(UI_LIBS) \
	../monkey/libmonkey.a \
	../view/libmonkeyview.a \
	../util/libmonkeyutil.a \
	../net/libmonkeynet.a \
	../audio/libmonkeyaudio.a

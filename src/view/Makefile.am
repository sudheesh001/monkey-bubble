NULL=


INCLUDES = \
	$(LIBMONKEYVIEW_A_CFLAGS) \
	$(AM_CFLAGS) \
	-I$(top_srcdir)/src/monkey \
	-g -O2 -Wall -Werror -D_REENTRANT -DDATADIR=\""$(datadir)"\"

noinst_LIBRARIES = libmonkeyview.a



libmonkeyview_a_SOURCES = \
	monkey-canvas.h monkey-canvas.c \
	monkey-view.h monkey-view.c \
	mini-view.h mini-view.c

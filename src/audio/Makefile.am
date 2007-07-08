NULL=

INCLUDES = \
	$(LIBMONKEYAUDIO_A_CFLAGS) \
	-I$(top_srcdir)/src/monkey \
	-g -O2 -Wall -D_REENTRANT -DDATADIR=\""$(datadir)"\"


noinst_LIBRARIES = libmonkeyaudio.a

libmonkeyaudio_a_SOURCES = \
	sound-manager.h sound-manager.c \
	gst-mbmem-content.h gst-mbmem-content.c \
	gst-mbmem-src.h gst-mbmem-src.c \
	mb-audio-engine.h mb-audio-engine.c \
	mb-music-bin.h mb-music-bin.c \
	$(NULL)


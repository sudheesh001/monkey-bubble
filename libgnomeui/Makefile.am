noinst_LTLIBRARIES=libgnomeui.la

libgnomeui_la_CPPFLAGS=\
        -I$(top_srcdir) \
        $(LIBMONKEYUTIL_A_CFLAGS) \
        $(PLATFORM_CFLAGS) \
        $(AM_CPPFLAGS)
libgnomeui_la_LIBADD=\
        $(top_builddir)/libgnome/libgnome.la \
        $(LIBMONKEYUTIL_A_LIBS) \
        $(PLATFORM_LIBS) \
        $(LIBADD)
libgnomeui_la_SOURCES=\
        gnome-scores.c \
        gnome-scores.h \
        $(NULL)

noinst_PROGRAMS=linktest
linktest_CPPFLAGS=$(libgnomeui_la_CPPFLAGS)
linktest_LDADD=\
        libgnomeui.la \
        $(libgnomeui_la_LIBADD)
linktest_SOURCES=\
        linktest.c \
        $(NULL)

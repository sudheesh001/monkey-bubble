NULL=

SUBDIRS = 	\
	data	\
	pixmaps	\
	sounds 	\
        src     \
        po      \
        help    \
	$(NULL)

applications_in_in_files = monkey-bubble.desktop.in.in
applications_in_files = $(applications_in_in_files:.desktop.in.in=.desktop.in)
applications_DATA = $(applications_in_in_files:.desktop.in.in=.desktop)

if MAEMO
applicationsdir = $(datadir)/applications/hildon
else
applicationsdir = $(datadir)/applications
endif

$(applications_in_files): $(applications_in_in_files) Makefile
if MAEMO
	sed 's#@BINDIR@#$(bindir)/#' $< > $@
else
	sed 's#@BINDIR@##' $< > $@
endif

@INTLTOOL_DESKTOP_RULE@

intltool_in_files=\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in		\
	$(NULL)

EXTRA_DIST=\
	$(applications_in_in_files) \
	$(intltool_in_files) \
	$(NULL)

CLEANFILES=\
	$(applications_DATA) \
	$(applications_in_files) \
	$(intltool_in_files:.in=) \
	$(NULL)

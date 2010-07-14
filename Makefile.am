NULL=

always_subdirs=\
	data \
	pixmaps \
        sounds \
        libgnome \
        libgnomeui \
        src \
        po \
        $(NULL)

SUBDIRS=\
	$(always_subdirs) \
	$(NULL)

if WITH_GNOME_DOC_UTILS
SUBDIRS+=help
endif

DIST_SUBDIRS=\
	$(always_subdirs) \
	help \
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
	sed 's#@BINDIR@#$(bindir)/#;s#\@VERSION\@#$(VERSION)#' $< > $@
else
	sed 's#@BINDIR@##;s#\@VERSION\@#$(VERSION)#' $< > $@
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

deb: dist
	-mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cp -r $(top_srcdir)/debian $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)/
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && \
		 make -f debian/rules debian/control && \
		 dpkg-buildpackage -rfakeroot || \
		 true
	-rm -rf $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION)

dist-local:
	@break_dist@ echo "FATAL:" 2>&1 > /dev/null
	@break_dist@ echo "FATAL: Trying to create broken tarball:" 2>&1 > /dev/null
	@break_dist@ echo "FATAL: ================================" 2>&1 > /dev/null
	@break_dist@ echo "FATAL: Some serious build-dependency (like gnome-doc-utils) is missing." 2>&1 > /dev/null
	@break_dist@ echo "FATAL: Refusing to 'make dist'" 2>&1 > /dev/null
	@break_dist@ echo "FATAL:" 2>&1 > /dev/null
	@break_dist@ false


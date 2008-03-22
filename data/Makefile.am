##
## data/Makefile.am for money-bubble
##

leveldir = $(datadir)/monkey-bubble
level_DATA = \
	levels

dbus_in_files=org.gnome.Games.MonkeyBubble.service.in
if MAEMO
dbusdir=$(datadir)/dbus-1/services
dbus_DATA=$(dbus_in_files:.service.in=.service)

$(dbus_DATA): $(dbus_in_files) Makefile
	sed 's#@BINDIR@#$(bindir)#' $< > $@
endif

gladexmldir = $(datadir)/monkey-bubble/glade
gladexml_DATA = \
	monkey-bubble.glade keybinding.glade netgame.glade netserver.glade

schemasdir   = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = monkey-bubble.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/data/$$p; \
		done \
	fi
	$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-touch $(DESTDIR)$(scoredir)/monkey-bubble.scores
	-chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/monkey-bubble.scores
	-chmod 664 $(DESTDIR)$(scoredir)/monkey-bubble.scores

EXTRA_DIST=\
	$(dbus_in_files) \
	$(level_DATA) \
	$(gladexml_DATA) \
	$(schemas_in_files) \
	$(NULL)
CLEANFILES=\
	$(dbus_DATA) \
	$(schemas_DATA) \
	$(NULL)

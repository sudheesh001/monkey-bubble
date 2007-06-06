SUBDIRS = bubbles frozen-bubble snake number
gfxdir = $(datadir)/monkey-bubble/gfx
gfx_DATA = \
	looser.svg \
	tomato.svg \
	banana.svg \
	layout_1_player.svg \
	layout_2_players.svg \
	layout_network_player.svg \
	pane.svg pause.svg \
	left_monkey.svg right_monkey.svg snake-body.svg \
	harm.svg \
	harm_up.svg harm_down.svg harm_shoot.svg \
	lost.svg win.svg \
	monkey.png splash.svg star.svg \
	hurry_up.svg

icondir=$(datadir)/icons/hicolor/32x32/apps
icon_DATA=monkey-bubble.png

gtk_update_icon_cache=gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = $(gfx_DATA) $(icon_DATA)


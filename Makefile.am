SUBDIRS = src

EXTRA_DIST = spotify-tray.spec.in \
			 spotify-tray.desktop.in

desktopdir = $(datadir)/applications
desktop_in_files = spotify-tray.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

rpm:
	rpmbuild -bb --build-in-place spotify-tray.spec

clean-local:
	-rm -f *.list

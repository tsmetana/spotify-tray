SUBDIRS = src

EXTRA_DIST = spotify-tray.spec.in \
			 spotify-tray.desktop.in \
			 LICENSE \
			 README.md

desktopdir = $(datadir)/applications
desktop_in_files = spotify-tray.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

RPMDIR = ${PWD}
RPMRESULTDIR = rpmbuild
rpm: dist-bzip2
	rpmbuild \
		--clean \
		--define "_sourcedir $(RPMDIR)" \
		--define "_specdir $(RPMDIR)" \
		--define "_builddir $(RPMDIR)" \
		--define "_srcrpmdir $(RPMDIR)/$(RPMRESULTDIR)" \
		--define "_rpmdir $(RPMDIR)/$(RPMRESULTDIR)" \
		-ba spotify-tray.spec

clean-local:
	-rm -f *.list
	-rm -rf $(RPMRESULTDIR)

AM_CPPFLAGS = \
	$(GTK_CFLAGS)
	
AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = spotify-tray

spotify_tray_SOURCES = \
	main.c

spotify_tray_LDFLAGS = \
	$(GTK_LDFLAGS) $(APPINDICATOR_LDFLAGS)

spotify_tray_LDADD =  \
	$(GTK_LIBS) $(APPINDICATOR_LIBS)


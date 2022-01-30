dnl Process this file with autoconf to produce a configure script.
AC_INIT(spotify-tray,1.3.2,,)
AM_INIT_AUTOMAKE([foreign])
AC_PREREQ(2.69)
AC_CONFIG_HEADERS(config.h)
AC_GNU_SOURCE
CFLAGS="$CFLAGS"
AC_SUBST(CFLAGS)
AC_PROG_CC
AC_HEADER_STDC

PKG_CHECK_MODULES(GTK, gtk+-3.0,,)
PKG_CHECK_MODULES(X11, x11,,)

AC_SUBST([BUILD_DATE], $(LC_ALL=C date +"%a %b %d %Y"))

AC_OUTPUT([
Makefile
src/Makefile
spotify-tray.spec
spotify-tray.desktop
])

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(zarfy, [0.1.0], supergring0@yahoo.com)

AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AC_CONFIG_SRCDIR([src/draw.c])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

PKG_CHECK_MODULES(ZARFY, [gtk+-2.0, gdk-pixbuf-xlib-2.0, libglade-2.0, xrandr])
AC_SUBST(ZARFY_CFLAGS)
AC_SUBST(ZARFY_LIBS)

AC_PATH_X
AC_HEADER_STDC
CPPFLAGS="`pkg-config --cflags gtk+-2.0 libglade-2.0` $CPPFLAGS"
AC_CHECK_HEADERS(gtk/gtk.h, [], AC_MSG_ERROR([missing libgtk header files.]))
AC_CHECK_HEADERS(gdk-pixbuf-xlib/gdk-pixbuf-xlib.h, [], AC_MSG_ERROR([libgdk-pixbuf header files.]))
AC_CHECK_HEADERS(glade/glade.h, [], AC_MSG_ERROR([missing libglade header files.]))
AC_CHECK_HEADERS(X11/Xlib.h, [], AC_MSG_ERROR([missing Xlib header files.]))
#AC_CHECK_HEADERS(X11/extensions/Xrandr.h, [], AC_MSG_ERROR([missing libxrandr header files.]))
AC_CHECK_HEADERS(X11/extensions/Xrender.h, [], AC_MSG_ERROR([missing libxrender header files.]))

AC_OUTPUT([Makefile
src/Makefile
])


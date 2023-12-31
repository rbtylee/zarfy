dnl Process this file with autoconf to produce a configure script.

# get rid of that stupid cache mechanism
rm -f config.cache

AC_PREREQ([2.61])

# Original Code: https://sourceforge.net/projects/zarfy/
# James S. Allingham <supergring0@yahoo.com>
AC_INIT([zarfy],[0.1.1],[ylee@bodhilinux.com],[zarfy],[https://github.com/rbtylee/zarfy])

AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS(config.h)

AC_CONFIG_SRCDIR([src/draw.c])

AC_PROG_CC
AC_PROG_INSTALL

AC_PROG_LN_S
dnl ---------------
dnl math
dnl ---------------
AC_C___ATTRIBUTE__
AC_CHECK_FUNCS([floor])
AC_SEARCH_LIBS([floor], [m], [],
        [AC_MSG_ERROR([Math library not found])])
PKG_CHECK_MODULES(ZARFY, [gtk+-2.0, gdk-pixbuf-xlib-2.0, libglade-2.0, xrandr, x11])
AC_SUBST(ZARFY_CFLAGS)
AC_SUBST(ZARFY_LIBS)

AC_PATH_X

AX_APPEND_FLAG([-Wno-deprecated-declarations], [CFLAGS])
CPPFLAGS="`pkg-config --cflags gtk+-2.0 libglade-2.0` $CPPFLAGS"

AC_CHECK_HEADERS(gtk/gtk.h, [], AC_MSG_ERROR([missing libgtk header files.]))
AC_CHECK_HEADERS(gdk-pixbuf-xlib/gdk-pixbuf-xlib.h, [], AC_MSG_ERROR([libgdk-pixbuf header files.]))
AC_CHECK_HEADERS(glade/glade.h, [], AC_MSG_ERROR([missing libglade header files.]))
AC_CHECK_HEADERS(X11/Xlib.h, [], AC_MSG_ERROR([missing Xlib header files.]))
AC_CHECK_HEADERS(X11/extensions/Xrandr.h, [], AC_MSG_ERROR([missing libxrandr header files.]))
AC_CHECK_HEADERS(X11/extensions/Xrender.h, [], AC_MSG_ERROR([missing libxrender header files.]))
AC_CHECK_FUNCS(getline)

AC_CONFIG_FILES([Makefile
src/Makefile
man/Makefile
data/Makefile
])
AC_OUTPUT

# report
txt_strip() {
  echo "[$]@" | sed -e 's/^[[ \t]]*\([[^ \t]]*\)[[ \t]]*$/\1/g'
}

echo
echo "Summary:"
echo
cat << SUMMARY_EOF
 * project.........: $PACKAGE $VERSION
 * prefix..........: $(txt_strip $prefix)
 * CFLAGS..........: $(txt_strip $CFLAGS)
SUMMARY_EOF

MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
	-DDATA_DIR=\""$(DESTDIR)$(pkgdatadir)"\" \
	@ZARFY_CFLAGS@

bin_PROGRAMS = zarfy
zarfy_SOURCES =	callbacks.c \
				draw.c \
				xrr.c \
				zarfy.c \
				config.c \
				sw_mode.c \
				zarfy.h \
				zarfy_private.h
zarfy_LDFLAGS = -rdynamic
zarfy_LDADD = @ZARFY_LIBS@

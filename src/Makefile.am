INCLUDES = \
	-DDATA_DIR=\""$(pkgdatadir)"\" \
	@ZARFY_CFLAGS@

bin_PROGRAMS = zarfy
zarfy_SOURCES =	callbacks.c \
				draw.c \
				xrr.c \
				zarfy.c \
				config.c \
				sw_mode.c \
				zarfy.h

zarfy_LDFLAGS = -rdynamic
zarfy_LDADD = @ZARFY_LIBS@


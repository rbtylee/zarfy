
SUBDIRS = src man

MAINTAINERCLEANFILES = \
ABOUT-NLS* \
Makefile.in \
aclocal.m4 \
compile \
config.guess \
config.h.in \
config.h.in~ \
config.rpath \
config.sub \
configure \
depcomp \
install-sh \
ltmain.sh \
missing

EXTRA_DIST = \
	data/zarfy.glade \
	data/monitor.png \
	data/monitor_s.png \
	data/monitor_d.png \
	data/dvi.png \
	data/lcd.png \
	data/vga.png \
	data/tv.png \
	data/zarfy.png 

install-data-local:
	@$(NORMAL_INSTALL)
	  $(mkinstalldirs) $(pkgdatadir); \
	  for fname in data/*; do \
	    if test -f $$fname; then \
	      $(INSTALL_DATA) $$fname $(pkgdatadir); \
	    fi \
	  done;

uninstall-local:
	@$(NORMAL_UNINSTALL)
	  for fname in $(pkgdatadir)/*; do \
	    if test -f $$fname; then \
	      rm $$fname; \
	    fi \
	  done; \
	rmdir $(pkgdatadir);

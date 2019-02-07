
SUBDIRS = src
EXTRA_DIST = \
	data/zarfy.glade \
	data/monitor.png \
	data/monitor_s.png \
	data/monitor_d.png \
	data/dvi.png \
	data/lcd.png \
	data/vga.png \
	data/tv.png \
	data/zarfy.png \
	man/zarfy.1.gz

install-data-local:
	@$(NORMAL_INSTALL)
	  $(mkinstalldirs) $(pkgdatadir); \
	  for fname in data/*; do \
	    if test -f $$fname; then \
	      $(INSTALL_DATA) $$fname $(pkgdatadir); \
	    fi \
	  done;
	  $(mkinstalldirs) $(mandir)/man1; \
	  $(INSTALL_DATA) man/zarfy.1.gz $(mandir)/man1

uninstall-local:
	@$(NORMAL_UNINSTALL)
	  for fname in $(pkgdatadir)/*; do \
	    if test -f $$fname; then \
	      rm $$fname; \
	    fi \
	  done; \
	rmdir $(pkgdatadir); \
	rm $(mandir)/man1/zarfy.*



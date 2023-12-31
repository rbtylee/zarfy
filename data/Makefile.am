MAINTAINERCLEANFILES = Makefile.in
zarfydir = $(datadir)/zarfy
zarfy_DATA =  \
	zarfy.glade \
	monitor.png \
	monitor_s.png \
	monitor_d.png \
	dvi.png \
	lcd.png \
	vga.png \
	tv.png 

# Custom uninstall hook to remove the directory
uninstall-hook:
	-rmdir $(DESTDIR)$(zarfydir)

# Ensure the uninstall-hook is executed during uninstallation
uninstall-local: uninstall-hook

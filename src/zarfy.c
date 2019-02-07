/*
* Copyright 2008 James S. Allingham
*
* This file is part of zarfy.
*
*    zarfy is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    zarfy is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with zarfy.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "zarfy.h"

void
bail (char *format, ...)
{
	va_list vl;
	va_start (vl, format);
	vfprintf (stderr, format, vl);
	va_end (vl);

	if ( gtk_started ) gtk_main_quit();
	exit (1);
}

void *
getmem(size_t count, size_t size)
{
	void *ptr;
	if (  (ptr=calloc(count,size)) ) return ptr;
	bail("Out of memory\n");
}

void
init_globals()
{
	disp = NULL;
	switch_mode = FALSE;
	display_name = NULL;
	gtk_started = FALSE;
	ignore_combo_change = FALSE;
	selected.monitor = NULL;
	selected.idx = -1;
	cycle = -1;
	onflg = FALSE;
	home = getenv("HOME");
}

/* called once at startup */
void
get_glade_ptrs()
{
	int i, map_width;
	double aspect;
	char lbl[100];

	/* main window */
	window1 = (GtkWindow *)glade_xml_get_widget(xml, "window1");
	gtk_window_set_icon_from_file(window1, ZARFY_ICON, NULL);
	
	/* main map area */
 	map_da = glade_xml_get_widget(xml,"map_da");
	aspect = (double)maxwidth / (double)maxheight;
	map_width = (double)MAP_HEIGHT * aspect;
	gtk_widget_set_size_request(map_da, map_width, MAP_HEIGHT);
	
	/* monitor thumbs */
	for ( i=0; i<MAXOUTPUT; i++ ) {
		sprintf(lbl,"label%1d", i+1);
		labels[i] = (GtkLabel *)glade_xml_get_widget(xml, lbl);
		sprintf(lbl, "output%1d", i+1);
		mntrs[i] = glade_xml_get_widget(xml, lbl);
		
		if ( i>=scres->noutput ) {
			gtk_widget_destroy((GtkWidget *)labels[i]);
			gtk_widget_destroy(mntrs[i]);
		}
	}
	
	/* rotate/reflect buttons */
	for ( i=0; i<6; i++ ) {
		sprintf( lbl, "rbtn%1d", i+1 );
		rot[i] = (GtkRadioButton *) glade_xml_get_widget(xml,lbl);
	}

	/* legend */
	for ( i=0; i<MAXOUTPUT; i++ ) {
		sprintf( lbl, "legend%1d", i+1 );
		legend[i] = (GtkLabel *) glade_xml_get_widget(xml,lbl);
		sprintf( lbl, "legend_da%1d", i+1 );
		legend_da[i] = glade_xml_get_widget(xml,lbl);
	}
	
	modebox = (GtkComboBox *) glade_xml_get_widget(xml,"modebox");
	rbtns = (GtkTable *) glade_xml_get_widget(xml,"rbtns");
	offbtn = (GtkCheckButton *) glade_xml_get_widget(xml,"offbtn");
	clonebox = (GtkComboBox *) glade_xml_get_widget(xml,"clonebox");
	snapto = (GtkCheckButton *) glade_xml_get_widget(xml,"snapto");
	max_label = (GtkLabel *) glade_xml_get_widget(xml,"max_label");
	cur_label = (GtkLabel *) glade_xml_get_widget(xml,"cur_label");
	alloff_warning = (GtkDialog *)glade_xml_get_widget(xml,"alloff_warning");
 	about_dialog = (GtkDialog *) glade_xml_get_widget(xml,"about_dialog");
 	about_lbl = (GtkLabel *) glade_xml_get_widget(xml,"about_lbl");
 	credits_dialog = (GtkDialog *) glade_xml_get_widget(xml,"credits_dialog");
 	credits_lbl = (GtkLabel *) glade_xml_get_widget(xml,"credits_lbl");
 	license_dialog = (GtkDialog *) glade_xml_get_widget(xml,"license_dialog");
	
	gtk_widget_show_all((GtkWidget *)window1);
	
	for ( i=0; i<scres->noutput; i++ )
		gdk_window_clear(mntrs[i]->window);
}

/* called on startup/apply/refresh */
void
init_display()
{
	int i, j;
	
	for ( i=0; i<scres->noutput;  i++ ) {
		gtk_label_set_text(labels[i], outputs[i]->name);
		if ( outputs[i]->connection == RR_Connected ) {
			draw_monitor(i, MON_PIXMAP);
			gdk_window_set_cursor (mntrs[i]->window, gdkhand);
		}
		else {
			draw_monitor(i, MON_DESEL_PIXMAP);
			gdk_window_set_cursor (mntrs[i]->window, gdkptr);
		}
	}

	for ( i=0, j=0; i<scres->noutput; i++ ) {
		clear_window (legend_da[i], legend_da_pms[i]);
		if ( outputs[i]->connection == RR_Connected ) {
			gtk_label_set_text(legend[j++],outputs[i]->name);
		}
	}

	gtk_widget_set_sensitive ((GtkWidget *)modebox, FALSE);
	gtk_widget_set_sensitive ((GtkWidget *)rbtns, FALSE);
	gtk_widget_set_sensitive ((GtkWidget *)offbtn, FALSE);
	gtk_widget_set_sensitive ((GtkWidget *)clonebox, FALSE);
	gtk_widget_set_sensitive ( map_da, FALSE);

	gdk_window_set_cursor (map_da->window, gdkptr);

	set_screen_size();
	draw_map();
	draw_legend();
	
	for ( i=0; i<scres->noutput; i++ ) {
		if ( outputs[i]->connection == RR_Connected && outputs[i]->crtc != None )
			select_output(labels[i]);
	}

	cycle++;
}

/* called once at startup */
void
init_graphics()
{
	int i;
	char buf[100];

	if ( !(xml = glade_xml_new(GLADE_XML_ZARFY, NULL, NULL)) )
		bail("Error opening glade xml file %s\n",buf);

	glade_xml_signal_autoconnect(xml);
	get_glade_ptrs();

	draw_gc = gdk_gc_new (map_da->window);
	map_pm = gdk_pixmap_new (map_da->window, map_da->allocation.width,
									map_da->allocation.height, -1);

	for ( i=0; i<scres->noutput; i++) 
			mntr_pms[i] = gdk_pixmap_new(mntrs[i]->window,
											mntrs[i]->allocation.width,
											mntrs[i]->allocation.height, -1);

	for ( i=0; i<scres->noutput; i++ )
		legend_da_pms[i] = gdk_pixmap_new(legend_da[i]->window,
											legend_da[i]->allocation.width,
											legend_da[i]->allocation.height, -1);

	gdkptr = gdk_cursor_new(GDK_LEFT_PTR);
	gdkhand = gdk_cursor_new(GDK_HAND2);

	sprintf( buf, "ZARFY\n\nVersion %s", VERSION );
	gtk_label_set_text ( about_lbl, buf );

	sprintf( buf, "Author %s\n%s", PACKAGE_AUTHOR, PACKAGE_BUGREPORT );
	gtk_label_set_text (credits_lbl, buf);

	for ( i=0; i<scres->noutput; i++ )
		if ( outputs[i]->connection == RR_Connected )
			gtk_widget_set_tooltip_text ( (GtkWidget *)mntrs[i],
						"Click to select");

	gtk_widget_set_tooltip_text ( (GtkWidget *)map_da,
						"Select an output\nthen click here to position");
}

void
xsetup()
{
	int major, minor;

	if ( disp != NULL ) XCloseDisplay(disp);

	if ( (disp = XOpenDisplay(display_name)) == NULL ) 
		bail("Can't open display %s\n", display_name);
	
	if (!XRRQueryVersion (disp, &major, &minor))
		bail("RandR extension missing\n");
	if (major < 1 || (major == 1 && minor < 2)) 
		bail("RandR version before 1.2\n");

	if ( (screen = DefaultScreen(disp)) >= ScreenCount (disp) )
		bail("Invalid screen number %d (display has %d)\n",
					screen, ScreenCount (disp) );

	rootwin = RootWindow (disp, screen);
	gdk_pixbuf_xlib_init (disp, screen); /* needed for screenshot */
}

void
usage()
{
	printf("Usage: zarfy [-l | -s] [-d display]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int i, loadnexit=FALSE;
	
	init_globals();
	
	for ( i=1; i<argc; i++) {
		if ( !strcmp(argv[i], "-l") ) {
			if ( switch_mode ) usage();
			loadnexit = TRUE;
		}
		else if ( !strcmp(argv[i], "-s") ) {
			if ( loadnexit ) usage();
			switch_mode = TRUE;
		}
		else if ( !strcmp(argv[i], "-d") ) {
			if ( ++i < argc && argv[i][0] != '-' )
				display_name = argv[i];
			else
				usage();
		}
		else
			usage();
	}

	xrr_init();

	if ( loadnexit ) return 0;

	gtk_init(&argc, &argv);

	if ( switch_mode )
		sw_setup();
	
	else {
		init_graphics();
		init_display();
	}

	gtk_started = TRUE;
	gtk_main();
	return 0;
}

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

/* Quick Switch Mode routines */

#include "zarfy.h"

#define				SW_WIDTH		80
#define				BORDER_WIDTH	5
#define				CELL_WIDTH		(SW_WIDTH + BORDER_WIDTH)
#define				LINE_WIDTH		2
#define				KEY_TIMER		2.0
#define				FLASH_TIMER		0.15
#define				FLASH_COUNT		15

int					sw_flash;

GdkColor			fgcol = RED;
GdkColor 			bgcol = LTGREY;

GtkWidget			*sw_win;
GtkWidget			*sw_vbox;
GtkWidget			*sw_da;
GdkPixmap			*sw_pm;
GtkWidget			*sw_label;
int					sw_current;
int					sw_max_hgt;
int					doubleclick;
int					ncomb;

struct	{
	int				nout;
	int				idx[2];
} sw_combos[10];

void
sw_exit()
{
	int i, j;

/* turn off any de-selected outputs */ 
	for ( i=0; i<scres->noutput; i++) {
		j = 0;
		if ( outputs[i]->connection == RR_Connected && outputs[i]->crtc != None ) {
			for ( j=0; j<sw_combos[sw_current].nout; j++ )
				if ( i == sw_combos[sw_current].idx[j] ) break;
		}
		if ( j == sw_combos[sw_current].nout )
			turn_off_output(outputs[i]);
	}

/* turn on any selected outputs */ 
	for ( i=0; i<sw_combos[sw_current].nout; i++ ) {
		j = sw_combos[sw_current].idx[i];
		if ( outputs[j]->crtc == None ) {
			if ( ( turn_on_output(outputs[j])) ) {
				if ( conf[j].crtc != None )
					set_config(outputs[j]);
			}
		}
	}

	apply();
	do_scripts();
	write_config();
	gtk_main_quit();
}

gint
expose_sw()
{	
	gdk_draw_drawable(sw_da->window, draw_gc, sw_pm,
				0, 0, 0, 0, sw_da->allocation.width, sw_da->allocation.height);
	return FALSE;
}

gint
button_press_event (GtkWidget *w, GdkEventButton *event, void *d)
{
	if (event->button !=1 )return FALSE; /* only left button events */
	doubleclick = event->type == GDK_2BUTTON_PRESS; 
	return FALSE;
}

gint
button_release_event (GtkWidget *w, GdkEventButton *event, void *d)
{
	if (event->button !=1 ) return FALSE; /* only left button events */
	
	if ( doubleclick )
		sw_exit();
	
	else {
		sw_deselect();
		sw_current = min(event->x/CELL_WIDTH, ncomb-1);
		sw_select();
	}
	return FALSE;
}

gint
key_press_event (GtkWidget *w, GdkEventKey *event, void *d)
{
	if ( event->keyval == GDK_Left || event->keyval == GDK_KP_Left ) {
		sw_deselect();
		if ( --sw_current < 0 ) sw_current = ncomb-1;
		sw_select();
	}
	else if ( event->keyval == GDK_Right || event->keyval == GDK_KP_Right ) {
		sw_deselect();
		if ( ++sw_current >= ncomb ) sw_current = 0;
		sw_select();
	}
	else if ( event->keyval == GDK_Return || event->keyval == GDK_KP_Enter ) 
		sw_exit();
	
	return FALSE;
}

void
sw_timer(double secs, int which)
{
	static struct itimerval timer;

	timer.it_interval.tv_sec=0;
	timer.it_interval.tv_usec=0;
	timer.it_value.tv_sec = floor(secs);
	timer.it_value.tv_usec = (secs - floor(secs)) * 1e6;
	setitimer(ITIMER_REAL, &timer, NULL);
	sw_flash = which;
}

void
sw_border(GdkColor color)
{
	gdk_gc_set_rgb_fg_color ( draw_gc, &color );
	gdk_draw_rectangle(sw_pm, draw_gc, FALSE,
					   sw_current*CELL_WIDTH +1, 1, /* x,y */
					   CELL_WIDTH,					/* width */
					   sw_max_hgt + BORDER_WIDTH);	/* height */
}

void
sw_alarm(int sig)
{
	static int flash_count;
	
	if ( !sw_flash )
		flash_count = FLASH_COUNT;
	else { 
		if ( --flash_count == 0 )
			sw_exit();
		if ( flash_count&1 )
			sw_border(bgcol);
		else 
			sw_border(fgcol);
		
		expose_sw();
	}
	sw_timer(FLASH_TIMER, TRUE);
}

void
sw_select()
{
	char buf[100];
	int i, j;

	sw_border(fgcol);

	i = sw_combos[sw_current].idx[0];
	j = sw_combos[sw_current].idx[1];

	strcpy(buf, outputs[i]->name);
	if ( sw_combos[sw_current].nout == 2 ) 
		sprintf(&buf[strlen(buf)], "+%s", outputs[j]->name);

	gtk_label_set_text((GtkLabel *)sw_label, buf);
	
	sw_timer(KEY_TIMER, FALSE);
}

void
sw_deselect()
{
	sw_border(bgcol);
}

char *
generic_name( char *name )
{
	static char buf[100];
	int i, j, found=0;

#define NTYPE 7
	char *prop_names[4][NTYPE]={
		/* intel    ati       radeon    nv     mga    nouveau   nvidia*/
		{ "LVDS",  "LVDS",    "PANEL", "LVDS", "???", "???" ,   "???"},
		{ "VGA",   "VGA",     "VGA",   "VGA",  "VGA", "Analog", "CRT"},
		{ "TMDS",  "DVI",     "DVI",   "DVI",  "DVI", "Digital","DFP"},
		{ "TV",    "S-video", "TV",    "???",  "???", "???",    "TV"}};

	char *generic_names[]=
		{ "lcd", "vga", "dvi", "tv", "monitor" };

	for ( i=0; i<4 ; i++ ) {
		for ( j=0; j<NTYPE; j++ ) {
			if ( strcasestr(name, prop_names[i][j] )) {
				found = TRUE;
				break;
			}
		}
		if ( found ) break;
	}
	
	sprintf(buf, "%s/%s.png", DATA_DIR, generic_names[i]);
	return buf;
}

void sw_setup()
{
	int i, j, destx;
	GdkPixbuf *large[MAXOUTPUT], *small[MAXOUTPUT];

/* load & scale the monitor icons */

	sw_max_hgt = ncomb = 0;
	for ( i=0; i<scres->noutput; i++ ) {
		if ( outputs[i]->connection == RR_Connected ) {
			ncomb++;
			large[i] = gdk_pixbuf_new_from_file_at_scale(
								generic_name(outputs[i]->name),
								SW_WIDTH, -1, TRUE, NULL);

			sw_max_hgt = max(sw_max_hgt, gdk_pixbuf_get_height(large[i]));

			small[i] = gdk_pixbuf_scale_simple( large[i],
					gdk_pixbuf_get_width(large[i])*7/10,
					gdk_pixbuf_get_height(large[i])*7/10,
					 GDK_INTERP_BILINEAR );

			for ( j=i+1; j<scres->noutput; j++ )
				if ( outputs[j]->connection == RR_Connected ) ncomb++;
		}
	}

	sw_widgit_init();

	/* draw each possible combination in a cell */
	ncomb = destx = 0;
	for ( i=0; i<scres->noutput; i++ ) {

		if ( outputs[i]->connection != RR_Connected ) continue;
		if ( outputs[i]->crtc != None ) sw_current = ncomb;

		sw_combos[ncomb].nout = 1;
		sw_combos[ncomb].idx[0]= i;

		gdk_draw_pixbuf ( sw_pm, draw_gc, large[i],
						0, 0, destx + BORDER_WIDTH-1,
						sw_max_hgt - gdk_pixbuf_get_height(large[i]),
	              		gdk_pixbuf_get_width ( large[i] ),
						gdk_pixbuf_get_height (large[i]),
	              		GDK_RGB_DITHER_NORMAL, 0, 0 );

		destx += CELL_WIDTH;
		ncomb++;
		
		for ( j=i+1; j<scres->noutput; j++ ) {
			if ( outputs[j]->connection != RR_Connected ) continue;
			if ( outputs[i]->crtc != None && outputs[j]->crtc != None )
				sw_current = ncomb;

			sw_combos[ncomb].nout = 2;
			sw_combos[ncomb].idx[0] = i;
			sw_combos[ncomb].idx[1] = j;;

			gdk_draw_pixbuf(sw_pm, draw_gc, small[i], 0, 0, /* srcx, srcy */
							destx+BORDER_WIDTH-1, BORDER_WIDTH, /* destx, desty */
							gdk_pixbuf_get_width(small[i]),	 /* width */
							gdk_pixbuf_get_height(small[i]), /* height */
							GDK_RGB_DITHER_NORMAL, 0,0);
			
			gdk_draw_pixbuf(sw_pm, draw_gc, small[j], 0, 0,
							destx + SW_WIDTH - gdk_pixbuf_get_width(small[j])-1,
							sw_max_hgt - gdk_pixbuf_get_height(small[j]),
							gdk_pixbuf_get_width(small[j]),
							gdk_pixbuf_get_height(small[j]),
							GDK_RGB_DITHER_NORMAL, 0,0);

			destx += CELL_WIDTH;
			ncomb++;
		}
	}

	sw_select();

	for (i=0; i<scres->noutput; i++) {
		if ( outputs[i]->connection == RR_Connected ) {
			g_object_unref(large[i]);
			g_object_unref(small[i]);
		}
	}
}

void
sw_widgit_init()
{
	sw_win = (GtkWidget *)gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_position( (GtkWindow *)sw_win, GTK_WIN_POS_CENTER_ALWAYS );
	gtk_window_set_decorated( (GtkWindow *)sw_win, FALSE);
	gtk_window_set_keep_above((GtkWindow *)sw_win, TRUE);
	
	sw_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (sw_win), sw_vbox);
	
	sw_da = gtk_drawing_area_new();
	gtk_widget_set_app_paintable( sw_da, TRUE );
	gtk_widget_set_size_request (sw_da,
								CELL_WIDTH*ncomb + BORDER_WIDTH,
								sw_max_hgt + BORDER_WIDTH*2 );
	
	sw_label = gtk_label_new (NULL);
	gtk_widget_set_size_request (sw_label,
								CELL_WIDTH*ncomb + BORDER_WIDTH, 30 );
	gtk_label_set_justify((GtkLabel *)sw_label,GTK_JUSTIFY_CENTER);
	
	gtk_box_pack_start (GTK_BOX (sw_vbox), sw_da, TRUE, TRUE, 0);
	gtk_box_pack_end (GTK_BOX (sw_vbox), sw_label, TRUE, TRUE, 0);
	
	g_signal_connect(GTK_OBJECT(sw_win), "key_press_event", GTK_SIGNAL_FUNC(key_press_event), NULL);
	g_signal_connect(GTK_OBJECT(sw_win), "button_press_event", GTK_SIGNAL_FUNC(button_press_event), NULL);
	g_signal_connect(GTK_OBJECT(sw_win), "button_release_event", GTK_SIGNAL_FUNC(button_release_event), NULL);
	g_signal_connect(GTK_OBJECT(sw_da), "expose_event", GTK_SIGNAL_FUNC(expose_sw), NULL);

	gtk_widget_set_events((GtkWidget *)sw_win,
			GDK_EXPOSURE_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK );
	
	signal (SIGALRM, sw_alarm);

	gtk_widget_show_all(sw_win);
	gtk_window_present((GtkWindow*)sw_win);	/* try for the input focus (seems to be needed for metacity) */

	draw_gc = gdk_gc_new (sw_da->window);
	gdk_gc_set_line_attributes (draw_gc, LINE_WIDTH, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);

	sw_pm = gdk_pixmap_new (sw_da->window, sw_da->allocation.width,	sw_da->allocation.height, -1);
	gdk_gc_set_rgb_fg_color ( draw_gc, &bgcol );
	gdk_draw_rectangle ( sw_pm, draw_gc, TRUE, 0, 0, sw_da->allocation.width, sw_da->allocation.height );

}

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

GdkColor colors[MAXOUTPUT] = 
		{ BLUE, RED, GREEN, ORANGE, YELLOW, AQUA, PINK, LIME, LTBLUE, BLACK };

void
clear_window(GtkWidget *widget, GdkPixmap *pm)
{
	gdk_window_clear(widget->window);
	gdk_draw_drawable(pm, draw_gc, widget->window,	0, 0, 0, 0,
						widget->allocation.width, widget->allocation.height);
}

void
draw_legend()
{
	int i, j=0;
	char buf[100];

	for (i=0; i<scres->noutput; i++) {
		if ( outputs[i]->connection == RR_Connected ) {

			clear_window(legend_da[j], legend_da_pms[j]);
			gdk_gc_set_rgb_fg_color ( draw_gc, &colors[i] );

			if ( selected.monitor && (outputs[i] == selected.output) )
				gdk_gc_set_line_attributes(draw_gc, 3,
								GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
			else
				gdk_gc_set_line_attributes(draw_gc, 1,
								GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);

			gdk_draw_line(legend_da_pms[j], draw_gc, 0, legend_da[j]->allocation.height/2,
									legend_da[j]->allocation.width-1,
									legend_da[j]->allocation.height/2);
			j++;
		}
	}
	
	sprintf(buf, "Max. Screen\n%dx%d", maxwidth, maxheight);
	gtk_label_set_text(max_label, buf);

	sprintf(buf,"Current Screen\n%dx%d", screen_width, screen_height);
	gtk_label_set_text(cur_label, buf);

	expose_legend();
}

void
draw_monitor(int idx, char *filename)
{
	XRRModeInfo *mi;
	GdkPixbuf *pb;
	double aspect;
	int w;
	
	if ( outputs[idx]->connection == RR_Connected )
		mi = preferred_mode(outputs[idx]);
	else
		mi = scres->modes;

	aspect = (double)mi->width/(double)mi->height; 
	w = (double)(IMAGE_HEIGHT-HEIGHT_ADJUST) * aspect + WIDTH_ADJUST;

	thumb[idx].width = (double)w/(double)IMAGE_WIDTH * (double)SCREEN_WIDTH;

	pb = gdk_pixbuf_new_from_file_at_scale(filename,
	                                       w, IMAGE_HEIGHT, FALSE, NULL);
	clear_window(mntrs[idx], mntr_pms[idx]);

/* thumbnail x offset into monitor image */
	thumb[idx].offset = (mntrs[idx]->allocation.width - gdk_pixbuf_get_width(pb))/2;
	
	gdk_draw_pixbuf(mntr_pms[idx], draw_gc, pb,
	                  0, 0, thumb[idx].offset, 0,
	                  gdk_pixbuf_get_width(pb), gdk_pixbuf_get_height(pb),
	                  GDK_RGB_DITHER_NORMAL, 0, 0 );
	g_object_unref(pb);
}

void
draw_crtc_window(XRRCrtcInfo *ci, int xorg, double scale)
{
	gdk_draw_rectangle(map_pm, draw_gc, FALSE,
	                     xorg+ci->x*scale, ci->y*scale,
	                     ci->width*scale, ci->height*scale);
}

void
draw_thumb(int idx, XRRCrtcInfo *ci, GdkPixbuf *screenshot, double scale)
{
#define FRAME_X 5
#define FRAME_Y 5

	GdkPixbuf *ss;
	GdkPixbuf *thm[] = {NULL, NULL, NULL, NULL, NULL};
	int i, r = 0;
	
	if ( !screenshot ) return;

	int x =  (double)ci->x * scale;
	int y =  (double)ci->y * scale;
	int w =  (double)ci->width * scale;
	int h =  (double)ci->height * scale;

	ss = gdk_pixbuf_new_subpixbuf(screenshot,x, y, w, h);		/* unref'ed pixbuf */
												
	thm[0] = ss;

	if ( ci->rotation & RR_Reflect_X )
		thm[r+1] = gdk_pixbuf_flip(thm[r++], TRUE);

	if ( ci->rotation & RR_Reflect_Y )
		thm[r+1] = gdk_pixbuf_flip(thm[r++], FALSE);

	switch ( ci->rotation & 0xf )
	{
		case RR_Rotate_90:
			thm[r+1] = gdk_pixbuf_rotate_simple(thm[r++],
							GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
			break;
		case RR_Rotate_180:
			thm[r+1] = gdk_pixbuf_rotate_simple(thm[r++],
							GDK_PIXBUF_ROTATE_UPSIDEDOWN);
			break;
		case RR_Rotate_270:
			thm[r+1] = gdk_pixbuf_rotate_simple(thm[r++],
							GDK_PIXBUF_ROTATE_CLOCKWISE);
	}

	thm[r+1] = gdk_pixbuf_scale_simple(thm[r++], thumb[idx].width, IMAGE_HEIGHT-HEIGHT_ADJUST,
							GDK_INTERP_BILINEAR);

	gdk_pixbuf_render_to_drawable(thm[r], mntr_pms[idx], draw_gc, 0, 0,
								FRAME_X+thumb[idx].offset, FRAME_Y,
								gdk_pixbuf_get_width(thm[r]),
								gdk_pixbuf_get_height(thm[r]),
								GDK_RGB_DITHER_NORMAL, 0, 0);

	for ( i=0;i<=r;i++ ) g_object_unref(thm[i]);
}

void
draw_map()
{
	GdkPixbuf *screenshot, *ss_scaled;
	GdkColor white = WHITE;
	double scale= (double)(map_da->allocation.height) / (double)maxheight;
	int  i;

	clear_window(map_da, map_pm);

	/*  max screen area */
	gdk_gc_set_rgb_fg_color(draw_gc, &white);
	gdk_draw_rectangle(map_pm, draw_gc, TRUE, 0, 0, maxwidth*scale, maxheight*scale);

	/* actual screen area: draw scaled screenshot*/
	screenshot = gdk_pixbuf_xlib_get_from_drawable(NULL, rootwin, 0, NULL,
	             0, 0, 0, 0,
	             DisplayWidth(disp, screen),
	             DisplayHeight(disp, screen) );

	ss_scaled = gdk_pixbuf_scale_simple(screenshot,
						screen_width*scale, screen_height*scale,
						GDK_INTERP_BILINEAR);
	g_object_unref(screenshot);

	gdk_pixbuf_render_to_drawable(ss_scaled, map_pm, draw_gc,
									0, 0, 0, 0,
									screen_width*scale, screen_height*scale,
									GDK_RGB_DITHER_NORMAL, 0, 0);

	/* draw connected outputs */
	gdk_gc_set_line_attributes(draw_gc, 1, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);

	for ( i=0; i<scres->noutput; i++ ) {
		XRRCrtcInfo *ci;

		if ( outputs[i]->connection != RR_Connected || !outputs[i]->crtc ) continue;
		if ( selected.monitor && outputs[i] == selected.output ) continue;

		ci = get_crtc(outputs[i]->crtc);
		gdk_gc_set_rgb_fg_color(draw_gc, &colors[i]);
		draw_crtc_window(ci, 0, scale);
		draw_thumb(i, ci, ss_scaled, scale);
	}

/* draw selected output last (on top) */
	gdk_gc_set_line_attributes(draw_gc, 3, GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_BEVEL);

	if ( selected.monitor && selected.output->crtc ) {

		gdk_gc_set_rgb_fg_color(draw_gc, &colors[selected.idx]);
		draw_crtc_window(selected.crtc, 0, scale);
		draw_thumb(selected.idx, selected.crtc, ss_scaled, scale);
	}
	g_object_unref(ss_scaled);

	expose_thumbs();
	expose_map();
}


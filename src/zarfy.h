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

#define _GNU_SOURCE
#include "../config.h"

#include <gtk/gtk.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#define PACKAGE_AUTHOR "J.S. Allingham"

#define MON_PIXMAP 			DATA_DIR "/monitor.png"
#define MON_SEL_PIXMAP 		DATA_DIR "/monitor_s.png"
#define MON_DESEL_PIXMAP 	DATA_DIR "/monitor_d.png"
#define GLADE_XML_ZARFY 	DATA_DIR "/zarfy.glade"
#define ZARFY_ICON			DATA_DIR "/zarfy.png"

#define MAXCRTC	4
#define MAXOUTPUT 10
#define IMAGE_WIDTH 70
#define IMAGE_HEIGHT 70
#define HEIGHT_ADJUST 22
#define SCREEN_WIDTH 62
#define WIDTH_ADJUST 7
#define MAP_HEIGHT 300

#define ROTATE_MASK (RR_Rotate_0 | RR_Rotate_90 | RR_Rotate_180 | RR_Rotate_270)
#define REFLECT_MASK (RR_Reflect_X | RR_Reflect_Y)

#define output_id(o) (scres->outputs[output_idx(o)])
#define crtc_id(c) (scres->crtcs[crtc_idx(c)])

#define BLUE {0,0,0,0xffff}
#define GREEN {0,0,0xffff,0}
#define RED {0,0xffff,0,0}
#define BLACK {0,0,0,0}
#define YELLOW {0,0xffff,0xffff,0}
#define AQUA {0,0x66ff,0xa8ff,0xb9ff}
#define PINK {0,0xffff,0,0xf3ff}
#define ORANGE {0,0xffff,0xbaff,0x22ff}
#define LTBLUE {0,0x06ff,0xfdff,0xf4ff}
#define LIME {0,0xa1ff,0xf8ff,0x0aff}
#define LTGREY {0,0xd3ff,0xd3ff,0xddff}
#define WHITE {0,0xffff,0xffff,0xffff}

Display					*disp;
Window					rootwin;
int						screen;
int						minwidth, maxwidth, minheight, maxheight;
int						screen_width, screen_height;
int						cycle;
char					*display_name;
char					*home;
Bool					switch_mode;
Bool					onflg;
Bool					gtk_started;
Bool					ignore_combo_change;

/* output thumbs */
struct {
	int					width;
	int					offset;
} thumb[MAXOUTPUT];

/* currently selected output */
struct {
	int					idx;
	GtkWidget			*monitor;
	GdkPixmap			*pm;
	XRROutputInfo		*output;
	XRRCrtcInfo			*crtc;
} selected;

/* configuration */
struct {
	char				name[10];
	RRCrtc				crtc;
	Connection			connection;
	RRMode				mode;
	int					width;
	int					height;
	int					x;
	int					y;
	Rotation			rot;
} conf[MAXOUTPUT];
#define NCONF 9

XRRScreenResources   	*scres;
XRRScreenResources 		*s_scres;
XRRCrtcInfo				*crtcs[MAXCRTC];
XRRCrtcInfo				*s_crtcs[MAXCRTC];
XRROutputInfo			*outputs[MAXOUTPUT];

GtkWindow				*window1;
GtkWidget				*mntrs[MAXOUTPUT];
GdkPixmap				*mntr_pms[MAXOUTPUT];
GtkLabel				*labels[MAXOUTPUT];
GtkComboBox				*modebox;
GtkTable				*rbtns;
GtkRadioButton			*rot[6];
GtkCheckButton			*offbtn;
GtkComboBox				*clonebox;
GtkWidget				*map_da;
GdkPixmap				*map_pm;
GdkGC					*draw_gc;
GdkCursor				*gdkhand;
GdkCursor				*gdkptr;
GtkCheckButton			*snapto;
GtkDialog				*alloff_warning;
GtkLabel				*legend[MAXOUTPUT];
GtkWidget				*legend_da[MAXOUTPUT];
GdkPixmap				*legend_da_pms[MAXOUTPUT];
GtkLabel				*max_label, *cur_label;
GtkDialog				*about_dialog;
GtkLabel				*about_lbl;
GtkDialog				*credits_dialog;
GtkLabel				*credits_lbl;
GtkDialog				*license_dialog;
GladeXML				*xml;

/* zarfy.c */
void 				bail(char *, ...);
void 				*getmem	(size_t, size_t);
void 				get_glade_ptrs	(void);
void 				init_display(void);
void 				init_globals(void);
void				xsetup(void);

/* xrr.c */
void				apply(void);
void				apply_crtc(XRRCrtcInfo *);
void				apply_screen_size(void);
void 				cleanup	(void);
int					crtc_idx(XRRCrtcInfo *);
Bool				crtc_changed(XRRCrtcInfo *);
void				disable_crtc(RRCrtc);
int					get_clone(XRROutputInfo *);
XRRCrtcInfo 		*get_crtc(RRCrtc);
XRRCrtcInfo			*get_crtc_for_output(XRROutputInfo *);
XRROutputInfo 		*get_output	(char *);
RROutput			get_output_id(char *);
void 				get_xrr_info(void);
float				horiz_refresh (XRRModeInfo *);
int					lookup(XID *, int, XID);
XRRModeInfo 		*mode_info(RRMode);
XRROutputInfo		*output_by_id(RROutput);
int					output_idx(XRROutputInfo *);
void				panic(Status);
XRRModeInfo			*preferred_mode	(XRROutputInfo *);
Bool	 			turn_on_output	(XRROutputInfo */*, Bool*/);
Bool 				turn_off_output	(XRROutputInfo *);
void				xrr_init(void);
Bool				zap_output(XRRCrtcInfo *, RROutput);
void				recover(void);

/* draw.c */
void				clear_window(GtkWidget *, GdkPixmap *);
void				draw_crtc_window(XRRCrtcInfo *, int, double);
void				draw_legend(void);
void 				draw_map(void);
void				draw_monitor(int, char *);
void				draw_thumb(int, XRRCrtcInfo *, GdkPixbuf *, double);

/* callbacks.c */
Bool				checkalloff(void);
void				check_bounds(XRRCrtcInfo *);
void 				deselect_output(void);
gint				expose_event (void);
gint				expose_legend(void);
gint				expose_map(void);
gint				expose_thumbs(void);
gint				offbtn_toggled(void);
void				reselect_output(void);
void				select_output (GtkLabel *);
void				set_hw_for_rot(XRRCrtcInfo *);
void				set_screen_size(void);
void				setup_selected_widgets(void);

/* config.c */
void				do_scripts(void);
void				exec_script(XRRCrtcInfo *, char *);
void				read_config(void);
int					set_config(XRROutputInfo *);
void				write_config(void);

/* sw_mode.c */
void				sw_deselect(void);
void				sw_select(void);
void				sw_setup(void);
void				sw_widgit_init(void);

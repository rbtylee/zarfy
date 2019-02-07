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

/* expose the crtc map */
gint
expose_map ()
{

	gdk_draw_drawable(map_da->window, draw_gc, map_pm,
				0, 0, 0, 0, map_da->allocation.width, map_da->allocation.height);

	return FALSE;
}

/* expose the monitor thumbs */
gint
expose_thumbs()
{
	int i;
	for ( i=0; i<scres->noutput; i++)
		gdk_draw_drawable(mntrs[i]->window, draw_gc, mntr_pms[i],
				0, 0, 0, 0, mntrs[i]->allocation.width, mntrs[i]->allocation.height);

	return FALSE;
}

gint
expose_legend()
{
	int i;
	
	for (i=0; i<scres->noutput; i++)
		gdk_draw_drawable(legend_da[i]->window, draw_gc, legend_da_pms[i],
				0, 0, 0, 0, legend_da[i]->allocation.width, legend_da[i]->allocation.height);
	return FALSE;
}

/* main window About button clicked */
gint
about_btn() 
{
	int button = 0;

	while ( button != 2 )  /*  2 = Close button */
		button = gtk_dialog_run(about_dialog);

	gtk_widget_hide((GtkWidget *)about_dialog);
	return FALSE;
}

/* About dialog Credits button */
gint
do_credits_dialog()
{
	gtk_dialog_run( credits_dialog );
	gtk_widget_hide((GtkWidget *)credits_dialog);
	return FALSE;
}

/* About dialog License button  */
gint
do_license_dialog()
{
	gtk_dialog_run( license_dialog );
	gtk_widget_hide((GtkWidget *)license_dialog);
	return FALSE;
}

/* warning if all crtcs are being turned off */
Bool
checkalloff()
{
	int i;

	for (i=0; i<scres->ncrtc; i++)
		if (crtcs[i]->mode) break;

	if ( i < scres->ncrtc ) return FALSE;

	i = gtk_dialog_run(alloff_warning);
	gtk_widget_hide((GtkWidget *)alloff_warning);

	if ( i == 0 ) return TRUE; /* Cancel button */
	return FALSE;					/* OK button */
}

/* don't let a crtc be moved outside the max sreen dimensions */
void
check_bounds(XRRCrtcInfo *ci)
{
	ci->x = max(ci->x, 0);

	if ( (ci->x + ci->width) > maxwidth )
		ci->x = maxwidth - ci->width;

	ci->y = max(ci->y, 0);

	if ( (ci->y + ci->height) > maxheight )
		ci->y = maxheight - ci->height;
}

/* crtc map area clicked */
gint
map_click()
{
	int x, y;
	double scale= ( double )maxheight / ( double )map_da->allocation.height;

	if ( selected.crtc == NULL ) return FALSE;

/* once seen, get rid of tooltip so as not to annoy */
//	gtk_widget_set_has_tooltip ( map_da, FALSE );

	gdk_window_get_pointer(map_da->window, &x, &y, NULL);

	if ( (double)x * scale > maxwidth
		|| (double)y *scale > maxheight ) return FALSE;

	if ( !gtk_toggle_button_get_active ((GtkToggleButton *)snapto)) {
/* snap-to button off: free placement */		
		selected.crtc->x = (double)x * scale;
		selected.crtc->y = (double)y * scale;
		check_bounds(selected.crtc);
	}

	else { /* find target for snap-to */
		int i, j, dmin, side=0;
		int n = -1;

		x = (double)x * scale;
		y = (double)y * scale;
		dmin = min(x, y);

/* for all active crtcs, find side closest to the click */

		for (i=0; i<scres->ncrtc; i++) {
			if ( crtcs[i]!=selected.crtc && crtcs[i]->mode && crtcs[i]->noutput ) {
				for (j=0; j<4; j++ ) {
					int dist=0;
					switch (j) {
						case 0:
							if ( y>=crtcs[i]->y && y<crtcs[i]->y+crtcs[i]->height )
								dist = abs(x-crtcs[i]->x);
							else
								dist = maxwidth;
							break;
						case 1:
							if ( y>=crtcs[i]->y && y<crtcs[i]->y+crtcs[i]->height )
								dist = abs(x-(crtcs[i]->x+crtcs[i]->width));
							else
								dist = maxwidth;
							break;
						case 2:
							if ( x>= crtcs[i]->x && x< crtcs[i]->x+crtcs[i]->width )
								dist = abs(y-crtcs[i]->y);
							else
								dist = maxheight;
							break;
						case 3:
							if ( x>= crtcs[i]->x && x< crtcs[i]->x+crtcs[i]->width )
								dist = abs(y-(crtcs[i]->y+crtcs[i]->height));
							else
								dist = maxheight;
					}
					if ( (dmin = min(dmin, dist ) ) == dist ) {
						n = i;
						side = j;
					}
				}
			}
		}
		if ( n == -1 )			 /* none found, snap to origin */
			selected.crtc->x = selected.crtc->y = 0;
		else {
			int nx=0, ny=0;
			Bool inside = ( x>=crtcs[n]->x && x<=crtcs[n]->x+crtcs[n]->width
						&& y>=crtcs[n]->y && y<=crtcs[n]->y+crtcs[n]->height );

			if ( inside ) {			/* same-as */
				nx = crtcs[n]->x;
				ny = crtcs[n]->y;
			}

			else {
				switch (side) {
				case 0:				/* right-of */
					nx = crtcs[n]->x-selected.crtc->width;
					ny = crtcs[n]->y;
					break;
				case 1:				/* left-of */
					nx = crtcs[n]->x+crtcs[n]->width;
					ny = crtcs[n]->y;
					break;
				case 2:				/* above */
					ny = crtcs[n]->y-selected.crtc->height;
					nx = crtcs[n]->x;
					break;
				case 3:				/* below */
					ny = crtcs[n]->y+crtcs[n]->height;
					nx = crtcs[n]->x;
				}
			}
			if ( nx >=0 && nx+selected.crtc->width <= maxwidth
					&& ny>=0 && ny+selected.crtc->height <= maxheight ) {
				selected.crtc->x = nx;
				selected.crtc->y = ny;
			}
		}
	}
	set_screen_size();
	draw_map();
	draw_legend();
	return FALSE;
}

void
set_hw_for_rot(XRRCrtcInfo *ci)
{
	XRRModeInfo *mi = mode_info(ci->mode);

	if ( ((ci->rotation & 0xf) == RR_Rotate_90 )
			|| ((ci->rotation & 0xf) == RR_Rotate_270) ) {
		ci->width = mi->height;
		ci->height = mi->width;
	}
	else {
		ci->height = mi->height;
		ci->width = mi->width;
	}
	check_bounds(ci);
}

/* one of the rotation/reflection buttons clicked */
gint
rotate(GtkToggleButton *button)
{
	char *lbl=(char *) gtk_button_get_label ((GtkButton *)button);

	if ( gtk_toggle_button_get_active(button) ) {

		Rotation rot = selected.crtc->rotation & ROTATE_MASK;
		Rotation ref = selected.crtc->rotation & REFLECT_MASK;

		if ( !strcmp(lbl,"Normal") ) rot = RR_Rotate_0;
		else if ( !strcmp(lbl,"Right") ) rot = RR_Rotate_270;
		else if ( !strcmp(lbl,"180") ) rot = RR_Rotate_180;
		else if ( !strcmp(lbl,"Left") ) rot = RR_Rotate_90;
		else if ( !strcmp(lbl,"Reflect X") ) ref |= RR_Reflect_X;
		else if ( !strcmp(lbl,"Reflect Y") ) ref |= RR_Reflect_Y;

		selected.crtc->rotation = rot|ref;
	}
	else {
		if ( !strcmp(lbl,"Reflect X") ) { 
			selected.crtc->rotation &= ~RR_Reflect_X;
		}
		else if ( !strcmp(lbl,"Reflect Y") ) {
			selected.crtc->rotation &= ~RR_Reflect_Y;
		}
	}
	set_hw_for_rot(selected.crtc);
	set_screen_size();
	draw_map();
	draw_legend();
	
	return FALSE;
}

/* mode box selection made */
gint
mode_change()
{
	if ( !ignore_combo_change ) {

		int i = gtk_combo_box_get_active( modebox );

		selected.crtc->mode = selected.output->modes[i];
		set_hw_for_rot(selected.crtc);
		set_screen_size();
		draw_map();
		draw_legend();
	}
	return FALSE;
}

/* clone combobox selection made */
gint
clone_change()
{
	Bool offbtn_changed = FALSE;

	if ( !ignore_combo_change ) {
		int i = gtk_combo_box_get_active( clonebox );
		Bool on = !gtk_toggle_button_get_active((GtkToggleButton *)offbtn);

		if (i==0) { 			/* Clone off */
			if ( on ) { /* output was turned on */
				gtk_toggle_button_set_active((GtkToggleButton *)offbtn, TRUE);
				offbtn_toggled();
				offbtn_changed = TRUE;
			}
		}
		else {					/* Clone on */
			XRRCrtcInfo *ci;
			XRROutputInfo *clone = output_by_id(selected.output->clones[i-1]);

			if ( on ) turn_off_output(selected.output);
			selected.output->crtc = clone->crtc;
			ci = get_crtc(clone->crtc);
			ci->outputs[ci->noutput++] = output_id(selected.output);

			if ( !on ) {
				gtk_toggle_button_set_active((GtkToggleButton *)offbtn, FALSE);
				onflg = TRUE;
				offbtn_toggled();
				offbtn_changed = TRUE;
			}
		}
	}

	if ( !offbtn_changed) {
		set_screen_size();
		draw_map();
		draw_legend();
	}
	return FALSE;
}

/* off button clicked (or called from clone_changed) */
gint
offbtn_toggled(void)
{
	static struct { 
		RROutput	oid;
		int			cycle;
		int			x,y;
		int			width, height;
		RRMode		mode;
		Rotation	rot;
	} cache;

	Bool changed;

	if ( gtk_toggle_button_get_active((GtkToggleButton *)offbtn) ) { /* on -> off */

/* save info in case of re-activation */
		cache.cycle = cycle;
		cache.oid = output_id(selected.output);
		cache.x = selected.crtc->x;
		cache.y = selected.crtc->y;
		cache.width = selected.crtc->width;
		cache.height = selected.crtc->height;
		cache.mode = selected.crtc->mode;
		cache.rot = selected.crtc->rotation;

		if ( (changed = turn_off_output(selected.output)) ) {
			draw_monitor(selected.idx, MON_SEL_PIXMAP);
			selected.crtc = NULL;
			
			ignore_combo_change = TRUE;
			gtk_combo_box_set_active (modebox, 0);
			ignore_combo_change = FALSE;
		}
		else /* turn button back to ON */ 
			gtk_toggle_button_set_active((GtkToggleButton *)offbtn, FALSE);
	}
	else { /* off -> on */
		if ( (changed = onflg) ) onflg = FALSE;
		if ( changed || (changed = turn_on_output(selected.output)) ) {

			selected.crtc = get_crtc(selected.output->crtc);

			if ( cache.cycle == cycle && cache.oid == output_id(selected.output) ) {
				selected.crtc->x = cache.x;
				selected.crtc->y = cache.y;
				selected.crtc->width = cache.width;
				selected.crtc->height = cache.height;
				selected.crtc->mode = cache.mode;
				selected.crtc->rotation = cache.rot;
			}
		}
		else 
			gtk_toggle_button_set_active((GtkToggleButton *)offbtn, TRUE);
	}
	if ( changed ) {
		setup_selected_widgets();
		set_screen_size();
		draw_map();
		draw_legend();
	}
	return FALSE;
}

gint
apply_button (void)
{
	if ( checkalloff() )return FALSE;

	deselect_output();
	apply();
	do_scripts();
	write_config();
	get_xrr_info();
	init_display();
	reselect_output();
	return FALSE;
}

gint
ok_button (void)
{
	if ( checkalloff() ) return FALSE;

	apply();
	do_scripts();
	write_config();
	gtk_main_quit();
}

/* refresh in case a monitor has been connected/disconnected since program start */
gint
refresh_button (GtkLabel *label)
{
	deselect_output();
	get_xrr_info();
	init_display();
	reselect_output();
	return FALSE;
}

void
deselect_output()
{
	int i;

	if (selected.monitor) {
		draw_monitor(selected.idx, MON_PIXMAP);

		ignore_combo_change = TRUE;

		for ( i=selected.output->nmode-1; i>=0; i-- )
			gtk_combo_box_remove_text ( modebox, i );

		for ( i=selected.output->nclone; i>=0; i-- )
			gtk_combo_box_remove_text ( clonebox, i );

		ignore_combo_change = FALSE;
		selected.monitor = NULL;
	}

	gdk_window_set_cursor (map_da->window, gdkptr);
}

void
reselect_output()
{
	if ( selected.idx != -1 )
		select_output( labels[selected.idx] );
}

/* monitor thumb icon clicked */
void
select_output (GtkLabel *lbl)
{
	char *ltxt, buf[100];
	int idx, i;
	XRROutputInfo *oi;
	XRRModeInfo *prefmode;

	for ( idx=0; idx<scres->noutput; idx++ )
		if ( lbl == labels[idx] ) break;

	if (selected.monitor == mntrs[idx]) return; /* already selected */

	ltxt = (char *)gtk_label_get_text(lbl);
	oi=get_output(ltxt);
	if (oi->connection == RR_Disconnected) return; /* not a candidate for selection */

	deselect_output(); 								/* deselect previous */

	selected.idx = idx;
	selected.monitor = mntrs[idx];
	selected.pm = mntr_pms[idx];
	selected.output = oi;
	selected.crtc = get_crtc(selected.output->crtc);

/* OK, user has seen the tooltip, get rid of the annoying thing */
//	gtk_widget_set_has_tooltip ( mntrs[idx], FALSE );

/* Give a hint how to change position */
	if ( gtk_widget_get_has_tooltip ( map_da ) ) {
		sprintf( buf, "Click where you\nwant to move %s to", selected.output->name ); 
		gtk_widget_set_tooltip_text ( map_da, buf );
	}

	prefmode = preferred_mode(selected.output);
	ignore_combo_change = TRUE; /* combo box calls generate an unwanted event */

	for ( i=0 ; i<selected.output->nmode; i++ ) {
		XRRModeInfo	*mode = mode_info(selected.output->modes[i]);

		sprintf(buf ,"%s%6.1fHz %s" , mode->name, horiz_refresh(mode) ,
					(prefmode->id == mode->id ? "*" :"" ) );
		gtk_combo_box_append_text (modebox, buf);
		if ( selected.crtc && (mode->id == selected.crtc->mode) ) 
			gtk_combo_box_set_active (modebox, i);
	}

	sprintf(buf, "Clone off");
	gtk_combo_box_append_text (clonebox, buf);
	gtk_combo_box_set_active (clonebox, 0);

	if ( selected.output->nclone ) {

		for ( i=0; i<selected.output->nclone; i++ ) {
			XRROutputInfo *clone = output_by_id(selected.output->clones[i]);

			if ( clone->crtc ) {
				XRRCrtcInfo *ci = get_crtc(clone->crtc);

				if ( lookup(selected.output->modes,
						selected.output->nmode, ci->mode) != -1 )  {
					sprintf(buf, "Clone to %s", clone->name );
					gtk_combo_box_append_text (clonebox, buf);
				}
			}
		}
		gtk_combo_box_set_active (clonebox, get_clone(selected.output));
	}
	ignore_combo_change = FALSE;

	draw_monitor(selected.idx, MON_SEL_PIXMAP);
	setup_selected_widgets();
	draw_map();
	draw_legend();
}

void
setup_selected_widgets()
{
	char *ltxt;
	int i;

	for ( i=0; i<6; i++) {
		ltxt = (char *)gtk_button_get_label( (GtkButton *)rot[i] );

		if ( selected.crtc ) {
			if ( (!strcmp(ltxt, "Normal") && (selected.crtc->rotations & RR_Rotate_0))
				|| (!strcmp(ltxt, "Left") && (selected.crtc->rotations & RR_Rotate_90))
				|| (!strcmp(ltxt, "180") && (selected.crtc->rotations & RR_Rotate_180))
				|| (!strcmp(ltxt, "Right") && (selected.crtc->rotations & RR_Rotate_270))  
				|| (!strcmp(ltxt, "Reflect X") && (selected.crtc->rotations & RR_Reflect_X))
				|| (!strcmp(ltxt, "Reflect Y") && (selected.crtc->rotations & RR_Reflect_Y)) )
			{
				gtk_widget_set_sensitive ( (GtkWidget *)rot[i], TRUE );

				if ( (!strcmp(ltxt, "Normal") && (selected.crtc->rotation & RR_Rotate_0))
					|| (!strcmp(ltxt, "Left") && (selected.crtc->rotation & RR_Rotate_90))
					|| (!strcmp(ltxt, "180") && (selected.crtc->rotation & RR_Rotate_180))
					|| (!strcmp(ltxt, "Right") && (selected.crtc->rotation & RR_Rotate_270))  
					|| (!strcmp(ltxt, "Reflect X") && (selected.crtc->rotation & RR_Reflect_X))
					|| (!strcmp(ltxt, "Reflect Y") && (selected.crtc->rotation & RR_Reflect_Y)) )

						gtk_toggle_button_set_active ( (GtkToggleButton *)rot[i], TRUE);
				continue;
			}
		}
		gtk_toggle_button_set_active ( (GtkToggleButton *)rot[i], FALSE);
		gtk_widget_set_sensitive ( (GtkWidget *)rot[i], FALSE );
	}

	if ( selected.crtc ) {
		gtk_toggle_button_set_active((GtkToggleButton *)offbtn, FALSE);
		gtk_widget_set_sensitive ( (GtkWidget *)rbtns, TRUE );
		gtk_widget_set_sensitive ( map_da, TRUE );
		gdk_window_set_cursor (map_da->window, gdkhand);
		gtk_widget_set_sensitive ( (GtkWidget *)offbtn, TRUE );

/* to keep things simple, no mode changes allowed while cloned */
		if ( selected.crtc->noutput >1 ) 
			gtk_widget_set_sensitive((GtkWidget *)modebox, FALSE);
		else
			gtk_widget_set_sensitive((GtkWidget *)modebox, TRUE);
	}

	else {
		gtk_toggle_button_set_active((GtkToggleButton *)offbtn, TRUE);
		gtk_widget_set_sensitive ( (GtkWidget *)rbtns, FALSE );
		gtk_widget_set_sensitive ( map_da, FALSE );
		gdk_window_set_cursor (map_da->window, gdkptr );
		gtk_widget_set_sensitive((GtkWidget *)modebox, FALSE);

		if ( get_crtc_for_output(selected.output) == None )
			gtk_widget_set_sensitive ( (GtkWidget *)offbtn, FALSE );
		else
			gtk_widget_set_sensitive ( (GtkWidget *)offbtn, TRUE );
	}

	if ( selected.output->nclone ) 
		gtk_widget_set_sensitive((GtkWidget *)clonebox, TRUE);
	else 
		gtk_widget_set_sensitive((GtkWidget *)clonebox, FALSE);
}

void
set_screen_size()
{
	int i;

	screen_width = minwidth;
	screen_height = minheight;

	for ( i=0; i<scres->ncrtc; i++ ) {
		if ( crtcs[i]->mode )  {
			screen_width = max(screen_width, crtcs[i]->x + crtcs[i]->width );
			screen_height = max(screen_height, crtcs[i]->y + crtcs[i]->height );
		}
	}
}


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

XRRModeInfo *
mode_info(RRMode mode)
{
	int i;

	for ( i=0; i<scres->nmode; i++ )
		if ( scres->modes[i].id == mode ) return &scres->modes[i];

	bail("getmodeinfo: mode 0x%x not found\n",mode);
}

XRROutputInfo *
get_output(char *oname)
{
	int i;
	for ( i=0; i<scres->noutput; i++ )
		if ( !(strcmp(outputs[i]->name, oname)) ) return outputs[i];

	bail("get_output: output %s not found\n",oname);
}

XRROutputInfo *
output_by_id(RROutput oid)
{
	int i;

	for (i=0; i<scres->noutput; i++)
		if ( scres->outputs[i] == oid ) return outputs[i];

	bail("output_by_id: output 0x%x not found\n");
}

int
output_idx(XRROutputInfo *oi)
{
	int i;
	for ( i=0; i<scres->noutput; i++ )
		if ( outputs[i] == oi ) return i;

	bail("output_idx: output 0x%x\n",oi);
}

XRRCrtcInfo *
get_crtc(RRCrtc c)
{
	int i;

	if (c == 0) return NULL;

	for ( i=0; i<scres->ncrtc; i++ )
		if ( scres->crtcs[i] == c ) 	return crtcs[i];

	bail("get_crtc: crtc 0x%x not found\n", c);
}

int
crtc_idx(XRRCrtcInfo *ci)
{
	int i;

	for ( i=0; i<scres->ncrtc; i++ )
		if ( crtcs[i] == ci ) return i;

	bail("crtc_idx: crtc 0x%x not found\n",ci);

}

/* get preferred mode for an output
   If more than one defined, return the one with hihgest res
   If none defined, return the top of the mode list for the output
   NB. Some monitors report modes which they do not, in fact, support
	In that case a working mode must be selected manually by the user. */

XRRModeInfo *
preferred_mode(XRROutputInfo *oi)
{
	int i;
	int maxw=0;
	XRRModeInfo *best=NULL;

	for ( i=0; i< oi->npreferred; i++) {
		XRRModeInfo *mi = mode_info(oi->modes[i]);
		if ( mi->width > maxw ) {
			maxw=mi->width;
			best=mi;
		}
	}
	return best ? best : mode_info(oi->modes[0]);
}

float
horiz_refresh (XRRModeInfo *mi)
{
	float hz = 0.0;

	if (mi->hTotal && mi->vTotal)
		hz = (float) mi->dotClock/((float)mi->hTotal * (float)mi->vTotal);
	return hz;
}

int
get_clone(XRROutputInfo *oi)
{
	int i;

	for ( i=0; i<scres->noutput; i++ ) {
		if ( outputs[i] == oi ) continue;
		if ( outputs[i]->crtc == None ) continue;
		if ( outputs[i]->crtc == oi->crtc )
			return lookup(oi->clones, oi->nclone, output_id(outputs[i]))+1;
	}
	return 0;
}

/* find an available crtc for the given output */
XRRCrtcInfo *
get_crtc_for_output(XRROutputInfo *oi)
{
	int i;

	for (i=0; i<oi->ncrtc; i++) {
		XRRCrtcInfo *ci = get_crtc(oi->crtcs[i]);

		if ( !ci->noutput 
				&& lookup(ci->possible, ci->npossible, output_id(oi))!=-1)
					 return ci;
	}
	return NULL;
}

/* Remove an output from a crtc's list of outputs */ 
Bool
zap_output(XRRCrtcInfo *ci, RROutput oid)
{
	int i, j;

	for ( i=0; i < ci->noutput; i++ ) 
		if (ci->outputs[i] == oid) break;

	if ( i == ci->noutput) return FALSE;

	for ( j=i; j<ci->noutput-1; j++)
		ci->outputs[j] = ci->outputs[j+1];

	if (--ci->noutput == 0 ) {
		ci->outputs[0] = None;
		ci->mode = None;
	}
	return TRUE;
}

/* activate a connected output with default config values*/
Bool
turn_on_output(XRROutputInfo *oi)
{
	XRRModeInfo *mi;
	XRRCrtcInfo *ci = get_crtc_for_output(oi);
 
	if ( !ci ) {
		fprintf(stderr,
				"Failed to get crtc for %s\nTry turning off one of the other outputs first\n",
				oi->name);
		return FALSE;
	}
	oi->crtc = crtc_id(ci);
	ci->outputs[ci->noutput++] = output_id(oi);

	if ( !ci->mode ) {
		mi = preferred_mode(oi);
		ci->mode = mi->id;
		ci->width = mi->width;
		ci->height = mi->height;
		ci->x = ci->y = 0;
		ci->rotation = RR_Rotate_0;
	}
	return TRUE;
}

Bool
turn_off_output(XRROutputInfo *oi)
{
	XRRCrtcInfo *ci = get_crtc(oi->crtc);

	if ( !ci ) return FALSE;
	
	if ( zap_output( ci, output_id(oi) ) ) {
		oi->crtc = None;
		return TRUE;
	}
	return FALSE; 
}

/* get the current hardware setup */
void
get_xrr_info()
{
	int i;
	static int first=TRUE;

	if ( first ) first = FALSE;
	else cleanup();

	xsetup();

	XRRGetScreenSizeRange (disp, rootwin, &minwidth, &minheight,
					 &maxwidth, &maxheight);
 
	if ( !(s_scres = XRRGetScreenResources (disp, rootwin)) )
		bail ("Failed to get screen resources.");

	scres = getmem(1, sizeof(XRRScreenResources));
	memcpy(scres, s_scres, sizeof(XRRScreenResources));

	if (scres->ncrtc > MAXCRTC )
		bail("More crtcs than %d\n",MAXCRTC);
	if (scres->noutput > MAXOUTPUT )
		bail("More outputs than %d\n",MAXOUTPUT);

	for ( i=0; i<scres->ncrtc; i++ ) {

		if ( !(s_crtcs[i] = XRRGetCrtcInfo (disp, scres, scres->crtcs[i])) )
			bail("Failed to get info for crtc 0x%x\n",scres->crtcs[i]);

		crtcs[i] = getmem( 1, sizeof(XRRCrtcInfo) );
		memcpy( crtcs[i], s_crtcs[i], sizeof(XRRCrtcInfo) );
		crtcs[i]->outputs = getmem(MAXOUTPUT,sizeof(RROutput) );
		
		memcpy( crtcs[i]->outputs, s_crtcs[i]->outputs,
			   s_crtcs[i]->noutput*sizeof(RROutput) );
	}
	for ( i=0; i<scres->noutput; i++ ) {

		if ( !(outputs[i]=XRRGetOutputInfo(disp, scres, scres->outputs[i])) )
			bail("Failed to get info for output 0x%x\n",scres->outputs[i]);
	}
}

void
cleanup()
{
	int i;

	for (i=0; i<scres->ncrtc; i++) {
		free(crtcs[i]->outputs);
		free(crtcs[i]);
		XRRFreeCrtcInfo(s_crtcs[i]);
	}

	for ( i=0; i<scres->noutput; i++ )
			XRRFreeOutputInfo(outputs[i]);

	XRRFreeScreenResources (s_scres);
	free(scres);
}

/* set startup state - called once*/
void
xrr_init()
{
	int i;

	get_xrr_info();
	read_config();

/* turn off outputs which are disconnected but turned on */
	for (i=0; i<scres->noutput; i++ )
		if ( ( outputs[i]->connection == RR_Disconnected ) && outputs[i]->crtc )
			turn_off_output(outputs[i]);

	if ( !switch_mode ) {

		/* try to restore previous config */
		for (i=0; i<scres->noutput; i++ ) {
			if ( outputs[i]->connection == RR_Connected ) {
				if ( conf[i].crtc != None ) { 				/* do we have a config? */
					if ( outputs[i]->crtc == None) { 		/* monitor not already turned on ?*/
						if ( !turn_on_output(outputs[i]) ) continue;
					}
					set_config(outputs[i]);
				}
			}
		}
 
/* turn on outputs which are connected but turned off  */
		for (i=0; i<scres->noutput; i++ ) 
			if ( outputs[i]->connection == RR_Connected && !outputs[i]->crtc )
				turn_on_output(outputs[i]);
	}

	set_screen_size();
	apply();
	get_xrr_info();
	set_screen_size();
}

void
apply()
{
	int i;
	int change, crtc_change[MAXCRTC];
	
	change =  (screen_width != DisplayWidth(disp,screen) )
			|| (screen_height != DisplayHeight(disp,screen) );
	
	for ( i=0; i<scres->ncrtc; i++) {
		crtc_change[i] = crtc_changed(crtcs[i]);
		change |= crtc_change[i];
	}
	if ( !change ) return;

/* disable any active crtc outside new screen size (else apply fails) */
	for  ( i=0; i<scres->ncrtc; i++ ) {
		if (s_crtcs[i]->mode) {
			if ( (s_crtcs[i]->x + s_crtcs[i]->width) > screen_width
					|| (s_crtcs[i]->y + s_crtcs[i]->height) > screen_height )
				disable_crtc(scres->crtcs[i]);
		}
	}

	XGrabServer(disp);
	apply_screen_size();

	for (i=0; i<scres->ncrtc; i++) {
		if ( crtc_change[i] ) 
 			apply_crtc(crtcs[i]);
	}

	XUngrabServer(disp);
	XSync(disp, FALSE);
}

void
apply_screen_size()
{
	if ( screen_width != DisplayWidth(disp,screen)
			|| screen_height != DisplayHeight(disp,screen) )

		XRRSetScreenSize (disp, rootwin, screen_width, screen_height,
		      				DisplayWidthMM(disp,screen), DisplayHeightMM(disp,screen));
}

void
apply_crtc(XRRCrtcInfo *ci)
{
	Status s;

	if ( (s = XRRSetCrtcConfig (disp, scres, crtc_id(ci), CurrentTime,
									ci->x, ci->y, ci->mode, ci->rotation,
									ci->outputs, ci->noutput) ) != RRSetConfigSuccess )
		panic(s);
}

void
disable_crtc(RRCrtc cid)
{
	Status s;

	if ( (s = XRRSetCrtcConfig (disp, scres, cid, CurrentTime,
									0, 0, None, RR_Rotate_0,NULL,0)) != RRSetConfigSuccess )
		panic(s);
}

/* check if a crtc has changed since last get_xrr_info() */
Bool
crtc_changed(XRRCrtcInfo *ci)
{
	int i=crtc_idx(ci);
	int j;

	if ( ci->x != s_crtcs[i]->x
			|| ci->y != s_crtcs[i]->y
			|| ci->width != s_crtcs[i]->width
			|| ci->height != s_crtcs[i]->height
			|| ci->mode != s_crtcs[i]->mode
			|| ci->rotation != s_crtcs[i]->rotation
			|| ci->noutput != s_crtcs[i]->noutput )
		return TRUE;

	for ( j=0; j<ci->noutput; j++ )
		if ( ci->outputs[j] != s_crtcs[i]->outputs[j] ) return TRUE;

	return FALSE;	
}

void
panic (Status s)
{
	fprintf(stderr,"panic: CRTC Config failed with status ");
	switch (s) {
		case RRSetConfigSuccess:
			fprintf(stderr,"RRSetConfigSuccess\n");
			break;
		case RRSetConfigInvalidTime:
			fprintf(stderr,"RRSetConfigInvalidTime\n");
			break;
		case RRSetConfigInvalidConfigTime:
			fprintf(stderr,"RRSetConfigInvalidConfigTime\n");
			break;
		case RRSetConfigFailed:
			fprintf(stderr,"RRSetConfigFailed\n");
			break;
		default:
			fprintf(stderr,"Unknown Error\n");
	}
	fprintf(stderr, "Attempting restore of last working config...\n");
	recover();
	exit(1);
}

void
recover()
{
	int i;
	memcpy(scres, s_scres, sizeof(XRRScreenResources));

	for (i=0; i<scres->ncrtc; i++)
		memcpy(crtcs[i], s_crtcs[i], sizeof(XRRCrtcInfo));

	set_screen_size();
	apply();
}

int
lookup(XID *list, int  nobj, XID target)
{
	int i;
	for ( i=0; i<nobj; i++ )
		if ( list[i] == target ) return i;
	return -1;
}



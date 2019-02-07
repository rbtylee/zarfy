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
read_config()
{
	FILE *fp;
	int  i, status=0;
	size_t n;
	char *line = getmem( (n = (strlen(home) + strlen(disp->display_name) + 30)), 1 );

	sprintf( line, "%s/.zarfy/%s/outputs.conf", home, disp->display_name ); 

	if  ( (fp=fopen(line, "r")) != NULL ) {
		for (i=0; i<scres->noutput; i++) {
			if ( getline(&line, &n, fp) == -1 ) break;
			if ( sscanf(line, "%s%x%x%x%d%d%d%d%x", conf[i].name, &conf[i].crtc,
				 &conf[i].connection, &conf[i].mode, &conf[i].width, &conf[i].height,
	 			&conf[i].x, &conf[i].y, &conf[i].rot) != NCONF ) break;
			if ( strcmp(conf[i].name, outputs[i]->name) != 0 ) break;
			if ( conf[i].mode != None && outputs[i]->connection == RR_Connected) 
				if (lookup(outputs[i]->modes, outputs[i]->nmode, conf[i].mode) <0 ) break;
		}
		if ( i == scres->noutput ) status=1;
		fclose(fp);
	}

/* on any error, discard all config info */
	if ( !status ) {
		for ( i=0; i<scres->noutput; i++ ) {
			strcpy(conf[i].name, outputs[i]->name);
			conf[i].crtc = outputs[i]->crtc;
			conf[i].connection = outputs[i]->connection;
			conf[i].mode = conf[i].width = conf[i].height = 0;
			conf[i].x = conf[i].y = conf[i].rot = 0;
		}
	}
	free(line);
}

void
write_config(void)
{
	FILE *fp;
	int i;
	char *buf = getmem( strlen(home) + strlen(disp->display_name) + 30, 1 );

	sprintf( buf, "%s/.zarfy", home); 

/* make the config directory if necessary */
	for ( i=0; i<2; i++ ) {
		if ( i == 0 ) sprintf( buf, "%s/.zarfy", home); 
		else sprintf( &buf[strlen(buf)], "/%s", disp->display_name);
 
		if ( access(buf, X_OK ) == -1 ) {
			if (errno == ENOENT) {
				if ( mkdir(buf, S_IRWXU) == -1 ) {
					perror(buf);
					free(buf);
					return;
				}
			}
			else {
				perror(buf);
				free(buf);
				return;
			}
		}
	}

	strcat(buf, "/outputs.conf");
	
	if  ( (fp=fopen(buf,"w")) == NULL )	{
		perror(buf);
		free(buf);
		return;
	}

	for ( i=0; i<scres->noutput; i++ ) {
		if ( outputs[i]->connection == RR_Connected && outputs[i]->crtc != None ) {
			XRRCrtcInfo *ci = get_crtc(outputs[i]->crtc);
			conf[i].crtc = outputs[i]->crtc;
			conf[i].connection = outputs[i]->connection;
			conf[i].mode = ci->mode;
			conf[i].width = ci->width;
			conf[i].height = ci->height;
			conf[i].x = ci->x;
			conf[i].y = ci->y;
			conf[i].rot = ci->rotation;
		}
		fprintf(fp,"%s %x %x %x %d %d %d %d %x\n",
				conf[i].name, conf[i].crtc, conf[i].connection, conf[i].mode,
				conf[i].width, conf[i].height, conf[i].x, conf[i].y, conf[i].rot);  
	}

	fclose(fp);
	free(buf);
}

int
set_config(XRROutputInfo *oi)
{
	int status = FALSE, i=output_idx(oi);
	XRRCrtcInfo *ci = get_crtc( oi->crtc );

	if ( lookup(oi->modes, oi->nmode, conf[i].mode) >=0 ) {
		XRRModeInfo *mi = mode_info( conf[i].mode );
		
		if ( conf[i].x >= 0 && conf[i].y >=0
			&& conf[i].width == mi->width && conf[i].height == mi->height
			&& ( conf[i].x+mi->width <= maxwidth )
			&& ( conf[i].y+mi->height <= maxheight ) ) {

				ci->mode = conf[i].mode;
				ci->width = mi->width;
				ci->height = mi->height;
				ci->x = conf[i].x;
				ci->y = conf[i].y;
				ci->rotation = conf[i].rot;
				set_hw_for_rot( ci );
				status = TRUE;
		}
	}
	return status;
}

void
do_scripts()
{
	int i;

	for ( i=0; i<scres->ncrtc; i++ ) {
		
		if ( s_crtcs[i]->rotation == crtcs[i]->rotation ) continue;

		Rotation rot = crtcs[i]->rotation & ROTATE_MASK;
		Rotation ref = crtcs[i]->rotation & REFLECT_MASK;
		Rotation oldrot = s_crtcs[i]->rotation & ROTATE_MASK;
		Rotation oldref = s_crtcs[i]->rotation & REFLECT_MASK;
		
		if ( rot != oldrot ) {
			if ( rot == RR_Rotate_0 )
				exec_script(crtcs[i], "rotate_none");
			else if ( rot == RR_Rotate_90 )
				exec_script(crtcs[i], "rotate_left");
			else if ( rot == RR_Rotate_180 )
				exec_script(crtcs[i], "rotate_180");
			else if ( rot == RR_Rotate_270 )
				exec_script(crtcs[i], "rotate_right");
		}
		
		if ( (ref & RR_Reflect_X) != (oldref & RR_Reflect_X) ) {
			if ( ref & RR_Reflect_X )
				exec_script(crtcs[i], "reflect_x");
			else
				exec_script(crtcs[i], "unreflect_x");
		}
		
		if ( (ref & RR_Reflect_Y) != (oldref & RR_Reflect_Y) ) {
			if ( ref & RR_Reflect_Y )
				exec_script(crtcs[i], "reflect_y");
			else
				exec_script(crtcs[i], "unreflect_y");
		}
	}
}

void
exec_script(XRRCrtcInfo *ci, char * arg)
{
	int j, s, status;
	char *buf = getmem( strlen(home) + strlen(disp->display_name) + 50, 1);
	s = sprintf(buf, "%s/.zarfy/%s/", home, disp->display_name);
	
	for (j=0; j<ci->noutput; j++) {
		XRROutputInfo *oi = output_by_id(ci->outputs[j]);
		sprintf(&buf[s], "%s_RR.sh", oi->name);

		if ( access(buf, X_OK ) != -1 ) {	/* file exists & is executable */
			sprintf(&buf[strlen(buf)], " %s", arg);
			if ( (status = system(buf)) != 0 )  		/* do it */
				fprintf(stderr, "%s returned status: %d\n", buf, status);
		}
		else if ( errno != ENOENT ) /* ENOENT -> no script to execute */
			perror(buf);
	}
	free(buf);
}

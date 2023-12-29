/*
* Copyright 2023 Robert Wiley
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

GtkWidget				*legend_da[MAXOUTPUT];
GdkPixmap				*legend_da_pms[MAXOUTPUT];
Display					*disp;
Window					rootwin;
int						screen;
int						cycle;
Bool					switch_mode;
Bool					onflg;
Bool					gtk_started;
Bool					ignore_combo_change;
char					*display_name;
char					*home;
GtkWindow				*window1;
GtkLabel				*labels[MAXOUTPUT];
GtkComboBox				*modebox;
GtkTable				*rbtns;
GtkRadioButton			*rotbtn[6];
GtkCheckButton			*offbtn;
GtkComboBox				*clonebox;
GtkWidget				*map_da;
GdkPixmap				*map_pm;
GtkLabel				*max_label, *cur_label;
GtkDialog				*about_dialog;
GtkLabel				*about_lbl;
GtkDialog				*credits_dialog;
GtkLabel				*credits_lbl;
GtkDialog				*license_dialog;
GladeXML				*xml;
GtkWidget				*mntrs[MAXOUTPUT];
GdkPixmap				*mntr_pms[MAXOUTPUT];
GdkCursor				*gdkhand;
GdkCursor				*gdkptr;
GtkCheckButton			*snapto;
GtkDialog				*alloff_warning;
GtkLabel				*legend[MAXOUTPUT];

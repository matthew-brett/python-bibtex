/*
 This file is part of pybliographer
 
 Copyright (C) 1998-1999 Frederic GOBRY
 Email : gobry@idiap.ch
 	   
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2 
 of the License, or (at your option) any later version.
   
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. 
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 $Id: stringutils.c,v 1.1.2.1 2003/09/01 19:54:46 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bibtex.h"

static GList * strings = NULL;

gchar *
bibtex_tmp_string (gchar * string) {
    strings = g_list_append (strings, string);
    return string;
}

void 
bibtex_tmp_string_free (void) {
    GList * tmp = strings;
    
    while (tmp) {
	g_free (tmp->data);
	tmp = tmp->next;
    }

    g_list_free (strings);
    
    strings = NULL;
}


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
 
 $Id: accents.c,v 1.1.2.2 2003/09/02 14:35:33 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <string.h>

#include "bibtex.h"

typedef struct  {
    gchar c;
    gchar m;
}
CharMapping;

typedef struct  {
    gchar * c;
    gchar * m;
}
StringMapping;

CharMapping acute [] = {
    {'A', 'Á'},
    {'E', 'É'},
    {'I', 'Í'},
    {'O', 'Ó'},
    {'U', 'Ú'},
    {'Y', 'Ý'},
    {'a', 'á'},
    {'e', 'é'},
    {'i', 'í'},
    {'o', 'ó'},
    {'u', 'ú'},
    {'y', 'ý'},
    {0, 0}
};

CharMapping grave [] = {
    {'A', 'À'},
    {'E', 'È'},
    {'I', 'Ì'},
    {'O', 'Ò'},
    {'U', 'Ù'},
    {'a', 'à'},
    {'e', 'è'},
    {'i', 'ì'},
    {'o', 'ò'},
    {'u', 'ù'},
    {0, 0}
};

CharMapping hat [] = {
    {'A', 'Â'},
    {'E', 'Ê'},
    {'I', 'Î'},
    {'O', 'Ô'},
    {'U', 'Û'},
    {'a', 'â'},
    {'e', 'ê'},
    {'i', 'î'},
    {'o', 'ô'},
    {'u', 'û'},
    {0, 0}
};

CharMapping trema [] = {
    {'A', 'Ä'},
    {'E', 'Ë'},
    {'I', 'Ï'},
    {'O', 'Ö'},
    {'U', 'Ü'},
    {'a', 'ä'},
    {'e', 'ë'},
    {'i', 'ï'},
    {'o', 'ö'},
    {'u', 'ü'},
    {'y', 'ÿ'},
    {0, 0}
};

CharMapping cedilla [] = {
    {'C', 'Ç'},
    {'c', 'ç'},
    {0, 0}
};

CharMapping tilda [] = {
    {'A', 'Ã'},
    {'O', 'Õ'},
    {'a', 'ã'},
    {'o', 'õ'},
    {0, 0}
};

StringMapping commands [] = {
    {"backslash", "\\"},
    {"S",  "§"},
    {"ss", "ß"},
    {"DH", "Ð"},
    {"dh", "ð"},
    {"AE", "Æ"},
    {"ae", "æ"},
    {"O",  "Ø"},
    {"o",  "ø"},
    {"TH", "Þ"},
    {"th", "þ"},
    {"aa", "å"},
    {"AA", "Å"},
    {"guillemotleft",    "«"},
    {"guillemotright",   "»"},
    {"flqq",             "«"},
    {"frqq",             "»"},
    {"guilsingleft",     "<"},
    {"guilsingright",    ">"},
    {"textquestiondown", "¿"},
    {"textexclamdown",   "¡"},
    {"copyright",        "©"},
    {"pound",            "£"},
    {"neg",              "¬"},
    {"-",                "­"},
    {"cdotp",            "·"},
    {",",                "¸"},
    {NULL, NULL}
};

static gchar *
initialize_table (CharMapping * map, char empty) {
    gchar * table;

    g_return_val_if_fail (map != NULL, NULL);

    table = g_new0 (gchar, 256);

    while (map->c != '\0') {
	table [(int) map->c] = map->m;
	map ++;
    }

    table [0] = empty;

    return table;
}

static GHashTable *
initialize_mapping (StringMapping * map) {
    GHashTable * dico;
    
    dico = g_hash_table_new (g_str_hash, g_str_equal);
    
    while (map->c != NULL) {
	g_hash_table_insert (dico, map->c, map->m);
	map ++;
    }

    return dico;
}

static gchar *
eat_as_string (GList ** flow,
	       gint qtt,
	       gboolean * loss) {

    BibtexStruct * tmp_s;
    gchar * tmp, * text = g_strdup ("");

    g_return_val_if_fail (qtt > 0, text);

    if (flow == NULL) {
	return text;
    }

    while (qtt > 0 && (* flow)) {
	tmp = text;
	tmp_s = (BibtexStruct *) ((* flow)->data);
	* flow = (* flow)->next;

	if (tmp_s->type == BIBTEX_STRUCT_SPACE) continue;

	qtt --;

	text = g_strconcat (text, 
			    bibtex_struct_as_string (tmp_s, BIBTEX_OTHER,
						     NULL, loss),
			    NULL);
	g_free (tmp);
    }

    return text;
}

gchar * 
bibtex_accent_string (BibtexStruct * s, 
		      GList ** flow,
		      gboolean * loss) {
    
    static gchar * acute_table = NULL;
    static gchar * grave_table = NULL;
    static gchar * hat_table = NULL;
    static gchar * trema_table = NULL;
    static gchar * cedilla_table = NULL;
    static gchar * tilda_table = NULL;

    static GHashTable * commands_table = NULL;

    gchar * text, * tmp, accent;

    g_return_val_if_fail (s != NULL, NULL);
    g_return_val_if_fail (s->type == BIBTEX_STRUCT_COMMAND, NULL);

    if (acute_table == NULL) {
	/* Initialize accent table if necessary */

	acute_table    = initialize_table   (acute,   '´');
	grave_table    = initialize_table   (grave,   '\0');
	hat_table      = initialize_table   (hat,     '\0');
	trema_table    = initialize_table   (trema,   '¨');
	cedilla_table  = initialize_table   (cedilla, '\0');
	tilda_table    = initialize_table   (tilda,   '\0');

	commands_table = initialize_mapping (commands);
    }

    /* traiter les codes de 1 de long */
    if (strlen (s->value.com) == 1) {
	accent = s->value.com [0];

	if (accent == 'i') {
	    return g_strdup ("i");
	}

	/* Is it a known accent ? */
	if (accent == '\'' ||
	    accent == '^'  ||
	    accent == '`'  ||
	    accent == '"'  ||
	    accent == '~'  ||
	    accent == 'c') {
	    
	    text = eat_as_string (flow, 1, loss);

	    tmp  = NULL;

	    switch (accent) {
	    case '\'':
		tmp = acute_table;
		break;
	    case '`':
		tmp = grave_table;
		break;
	    case '^':
		tmp = hat_table;
		break;
	    case '"':
		tmp = trema_table;
		break;
	    case 'c':
		tmp = cedilla_table;
		break;
	    case '~':
		tmp = tilda_table;
		break;
		
	    default:
		g_assert_not_reached ();
		break;
	    }
	    
	    /* We know how to convert */
	    if (tmp [(int) text [0]] != 0) {
		if (text [0]) {
		    text [0] = tmp [(int) text [0]];
		}
		else {
		    tmp = g_strdup_printf ("%c", tmp [(int)text [0]]);
		    g_free (text);
		    text = tmp;
		}
	    }
	    else {
		if (loss) * loss = TRUE;
	    }

	    return text;
	}
	else {
	    /* return the single symbol */
	    if (! isalnum (s->value.com [0])) {
		return g_strdup (s->value.com);
	    }
	}
    }

    /* if not found, use dictionnary to eventually map */
    text = g_hash_table_lookup (commands_table, s->value.com);

    if (text) {
      return g_strdup (text);
    }

    if (loss) * loss = TRUE;
    bibtex_warning ("unable to convert `\\%s'", s->value.com);

    return g_strdup (s->value.com);
}


void
bibtex_capitalize (gchar * text,
		   gboolean is_noun,
		   gboolean at_start) {
    gboolean begin_of_sentence;
    gchar * current;

    g_return_if_fail (text != NULL);

    /* Put everything lowercase */
    if (is_noun) {
	g_strdown (text);
    }

    current = text;
    begin_of_sentence = at_start;

    /* Parse the whole text */
    while (* current) {
	switch (* current) {
	    
	case ' ':
	    /* Skip whitespace */
	    break;

	case '-':
	    /* Composed names */
	    if (is_noun) {
		begin_of_sentence = TRUE;
	    }
	    break;

	case '.':
	    /* New sentence */
	    begin_of_sentence = TRUE;
	    break;

	default:
	    if (isalpha (* current) && begin_of_sentence) {
		* current = toupper (* current);
		begin_of_sentence = FALSE;
	    }
	    break;
	}
	
	current ++;
    }
}

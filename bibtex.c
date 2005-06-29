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
 
 $Id: bibtex.c,v 1.1.2.2 2003/09/02 14:35:33 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <string.h>

#include "bibtex.h"

void bibtex_message_handler (const gchar *log_domain G_GNUC_UNUSED,
			     GLogLevelFlags log_level,
			     const gchar *message,
			     gpointer user_data G_GNUC_UNUSED)
{
    gchar * name = g_get_prgname ();

    if (name) {
	fprintf (stderr, "%s: ", name);
    }

    switch (log_level) {

    case BIB_LEVEL_ERROR:
	fprintf (stderr, "error: %s\n", message);
	break;
    case BIB_LEVEL_WARNING:
	fprintf (stderr, "warning: %s\n", message);
	break;
    case BIB_LEVEL_MESSAGE:
	printf ("%s\n", message);
	break;
	
    default:
	fprintf (stderr, "<unknown level %d>: %s\n", log_level, message);
	break;
    }
}

void 
bibtex_set_default_handler (void) {
    g_log_set_handler (G_LOG_DOMAIN, BIB_LEVEL_ERROR,   
		       bibtex_message_handler, NULL);
    g_log_set_handler (G_LOG_DOMAIN, BIB_LEVEL_WARNING, 
		       bibtex_message_handler, NULL);
    g_log_set_handler (G_LOG_DOMAIN, BIB_LEVEL_MESSAGE, 
		       bibtex_message_handler, NULL);
}


static void 
add_to_dico (gpointer key, gpointer value, gpointer user) {
    gchar * val = (gchar *) key;
    BibtexField * field;
    BibtexStruct * structure;

    if ((structure = g_hash_table_lookup ((GHashTable *) user, val)) == NULL) {
	val = g_strdup ((char *) key);
    }
    else {
	bibtex_struct_destroy (structure, TRUE);
    }

    field = (BibtexField *) value;
    g_strdown (val);

    g_hash_table_insert ((GHashTable *) user, val, field->structure);
}


BibtexEntry * 
bibtex_source_next_entry (BibtexSource * file,
			  gboolean filter) {
    BibtexEntry * ent;

    int offset;

    g_return_val_if_fail (file != NULL, NULL);

    if (file->eof) return NULL;

    offset = file->offset;
    file->error = FALSE;

    do {
	ent = bibtex_analyzer_parse (file);

	if (ent) {
	    /* Incrementer les numeros de ligne */
	    file->line += ent->length;
	    
	    ent->offset = offset;
	    ent->length = file->offset - offset;
	    
	    /* Rajouter les definitions au dictionnaire, si necessaire */
	    if (ent->type) {
		if (strcasecmp (ent->type, "string") == 0) {

		    g_hash_table_foreach (ent->table, add_to_dico, 
					  file->table);
		    
		    if (filter) {
			/* Return nothing, we store it as string database */
			bibtex_entry_destroy (ent, FALSE);
			ent = NULL;
		    }
		}
		else {
		    do {
			if (strcasecmp (ent->type, "comment") == 0) {
			    bibtex_entry_destroy (ent, TRUE);
			    ent = NULL;
			    
			    break;
			}

			if (strcasecmp (ent->type, "preamble") == 0) {
			    if (filter) {
				bibtex_warning ("%s:%d: skipping preamble",
						file->name, file->line);
				
				bibtex_entry_destroy (ent, TRUE);
				ent = NULL;
			    }
			    else {
				ent->textual_preamble =
				    bibtex_struct_as_bibtex (ent->preamble);
			    }
			    break;
			}

			/* normal case; convert preamble into entry name */
			if (ent->preamble) {
			    switch (ent->preamble->type) {
			    case BIBTEX_STRUCT_REF:
				/* alphanumeric identifiers */
				ent->name = 
				    g_strdup (ent->preamble->value.ref);
				break;
			    case BIBTEX_STRUCT_TEXT:
				/* numeric identifiers */
				ent->name = 
				    g_strdup (ent->preamble->value.text);
				break;

			    default:
				if (file->strict) {
				    bibtex_error ("%s:%d: entry has a weird name", 
						  file->name, file->line);
				    bibtex_entry_destroy (ent, TRUE);
				    file->error = TRUE;
				    
				    return NULL;
				}
				else {
				    bibtex_warning ("%s:%d: entry has a weird name", 
						    file->name, file->line);
				    bibtex_struct_destroy (ent->preamble, TRUE);
				    ent->preamble = NULL;
				    ent->name = NULL;
				}
				break;
			    }
			}
			else {
			    if (file->strict) {
				bibtex_error ("%s:%d: entry has no identifier", 
					      file->name,
					      file->line);
				
				bibtex_entry_destroy (ent, TRUE);
				file->error = TRUE;
				
				return NULL;
			    }
			    else {
				bibtex_warning ("%s:%d: entry has no identifier", 
						file->name,
						file->line);
			    }
			}
		    } while (0);
		}
	    }
	}
	else {
	    /* No ent, there has been a problem */
	    return NULL;
	}
    } 
    while (!ent);

    return ent;
}



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
 
 $Id: field.c,v 1.1.2.2 2003/09/02 14:35:33 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "bibtex.h"


static GMemChunk * field_chunk = NULL;

BibtexField *
bibtex_field_new (BibtexFieldType type) {
    BibtexField * field;

    if (field_chunk == NULL) {
	field_chunk = g_mem_chunk_new ("BibtexField",
				       sizeof (BibtexField),
				       sizeof (BibtexField) * 16,
				       G_ALLOC_AND_FREE);
    }

    field = g_chunk_new (BibtexField, field_chunk);
    
    field->structure = NULL;
    field->type = type;
    field->text = NULL;
    field->converted = FALSE;
    field->loss = FALSE;

    switch (field->type) {
    case BIBTEX_AUTHOR:
	/* Empty list */
	field->field.author = NULL;
	break;

    case BIBTEX_DATE:
	field->field.date.year = 0;
	field->field.date.month = 0;
	field->field.date.day = 0;
	break;

    case BIBTEX_OTHER:
    case BIBTEX_TITLE:
	break;

    default:
	g_warning ("unknown field type `%d'", field->type);
	bibtex_field_destroy (field, TRUE);

	return NULL;
    }

    return field;
}

void
bibtex_field_destroy (BibtexField * field,
		      gboolean value) {

    g_return_if_fail (field != NULL);
    
    if (value && field->structure) {
	bibtex_struct_destroy (field->structure, TRUE);
    }

    if (field->text) {
	g_free (field->text);
    }

    switch (field->type) {
	
    case BIBTEX_AUTHOR:
	if (field->field.author) {
	    bibtex_author_group_destroy (field->field.author);
	}
	break;

    default:
      break;
    }

    g_chunk_free (field, field_chunk);
}


BibtexField *
bibtex_struct_as_field (BibtexStruct * s,
			BibtexFieldType type) {

    BibtexField * field;

    g_return_val_if_fail (s != NULL, NULL);

    field = bibtex_field_new (type);
    
    field->structure  = s;

    return field;
}

BibtexField *
bibtex_field_parse (BibtexField * field,
		    GHashTable * dico) {

    g_return_val_if_fail (field != NULL, NULL);

    if (field->converted) {
	/* Convert just once */
	return field;
    }

    field->converted = TRUE;

    field->text = bibtex_struct_as_string (field->structure,
					   field->type, dico, 
					   & field->loss);

    switch (field->type) {
    case BIBTEX_AUTHOR:
	field->field.author = bibtex_author_parse (field->structure, dico);
	break;

    case BIBTEX_DATE:
	field->field.date.year  = atoi (field->text);
	field->field.date.month = 0;
	field->field.date.day   = 0;
	break;

    default:
      break;
    }

    return field;
}

BibtexField *
bibtex_string_as_field (gchar * string,
			BibtexFieldType type) {
    BibtexField * field = NULL;

    g_return_val_if_fail (string != NULL, NULL);

    field = bibtex_field_new (BIBTEX_OTHER);
    field->converted = TRUE;
    field->text = g_strdup (string);

    switch (type) {
    case BIBTEX_AUTHOR:
	break;

    case BIBTEX_DATE:
	break;

    case BIBTEX_TITLE:
	break;

    case BIBTEX_OTHER:
	break;

    default:
	bibtex_field_destroy (field, TRUE);
	g_warning ("unknown type `%d' for string `%s'", 
		   type, string);
	return NULL;
    }

    return field;
}

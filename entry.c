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
 
 $Id: entry.c,v 1.1.2.1 2003/09/01 19:54:46 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bibtex.h"

static GMemChunk * entry_chunk = NULL;

static void
free_data_field (gpointer key, 
		 gpointer value, 
		 gpointer user) {

    g_free (key);
    if (user) {
	bibtex_field_destroy ((BibtexField *) value, TRUE);
    }
}


BibtexEntry *
bibtex_entry_new (void) {
    BibtexEntry * entry;

    if (entry_chunk == NULL) {
	entry_chunk = g_mem_chunk_new ("BibtexEntry",
				       sizeof (BibtexEntry),
				       sizeof (BibtexEntry) * 16,
				       G_ALLOC_AND_FREE);
    }

    entry = g_chunk_new (BibtexEntry, entry_chunk);
    
    entry->length = 0;

    entry->name = entry->type = NULL;
    entry->preamble = NULL;
    
    entry->table = g_hash_table_new (g_str_hash, g_str_equal);

    return entry;
}
void 
bibtex_entry_destroy (BibtexEntry * entry,
		      gboolean content) {

    g_return_if_fail (entry != NULL);

    if (entry->type)
	g_free (entry->type);

    if (entry->name)
	g_free (entry->name);

    if (entry->preamble) 
	bibtex_struct_destroy (entry->preamble, TRUE);

    g_hash_table_foreach (entry->table, free_data_field, GINT_TO_POINTER(content));
    g_hash_table_destroy (entry->table);

    g_chunk_free (entry, entry_chunk);
}

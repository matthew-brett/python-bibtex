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
 
 $Id: testparser.c,v 1.1.2.1 2003/09/01 19:54:46 fredgo Exp $
*/

#include "bibtex.h"

char * program_name = "testparser";

void print_all (gpointer key,
		gpointer data,
		gpointer user)
{
    BibtexField * field = (BibtexField *) data;
    gchar * tmp;
    int i;
    BibtexAuthor * auth;
    GHashTable * dico = (GHashTable *) user;

    bibtex_field_parse (field, dico);

    printf ("\tfield `%s': [%d]\t", (gchar *) key, field->loss);

    switch (field->type) {
    case BIBTEX_AUTHOR:
	printf ("%s", field->text);

	for (i = 0; i < field->field.author->len; i++) {
	    
	    auth = (BibtexAuthor *) & g_array_index (field->field.author,
						     BibtexAuthor, i);

	    printf ("\n\t  %s", auth->last);
	    if (auth->lineage) {
		printf (" (%s)", auth->lineage);
	    }
	    if (auth->first) {
		printf (", %s", auth->first);
	    }
	}
	printf ("\n");
	break;

    default:
	printf ("%s\n", field->text);
    }
}

main (int argc, char * argv [])
{
    BibtexEntry * entry;
    BibtexSource * file;
    char * tmp;
    BibtexField * field;

    bibtex_set_default_handler ();

    if (0) {
      field = bibtex_field_new (BIBTEX_TITLE);
      field->text = g_strdup ("Essai \\ é GRONF oui FIN");
      field->converted = TRUE;
      
      field = bibtex_reverse_field (field, 1, 1);
      tmp = bibtex_struct_as_bibtex (field->structure);
      
      g_message ("converted: %s", tmp);
      g_free (tmp);
    }

    file = bibtex_source_new ();

    if (! bibtex_source_file (file, argv [1])) {
	printf ("cant open file\n");
	exit (0);
    }

    file->debug  = argc - 2;
    file->strict = 0;

    while (entry = bibtex_source_next_entry (file, TRUE)) {

	if (entry->type && entry->name)
	    printf ("(%s) [%s] %d\n",entry->type, entry->name,
		    bibtex_source_get_offset (file));

	g_hash_table_foreach (entry->table, print_all, file->table);
	bibtex_entry_destroy (entry, TRUE);

	printf ("\n");
    }

    if (file->eof) 
	printf ("at end of file\n");
    else {
	printf ("at error\n");
    }

    bibtex_source_string (file , "essai", "@test{coucou, author = {moi}}");

    while (entry = bibtex_source_next_entry (file, TRUE)) {

	if (entry->type && entry->name)
	    printf ("(%s) [%s] %d\n",entry->type, entry->name,
		    bibtex_source_get_offset (file));

	g_hash_table_foreach (entry->table, print_all, file->table);
	bibtex_entry_destroy (entry, TRUE);

	printf ("\n");
    }

    if (file->eof) 
	printf ("at end of file\n");
    else {
	printf ("at error\n");
    }

    bibtex_source_destroy (file, TRUE);


    g_mem_profile ();
}

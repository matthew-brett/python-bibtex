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
 
 $Id: author.c,v 1.1.2.2 2003/09/02 14:35:33 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <ctype.h>

#include "bibtex.h"

/* --------------------------------------------------
   Simple allocation / desallocation mechanism
   -------------------------------------------------- */

BibtexAuthorGroup *
bibtex_author_group_new (void) {
    return g_array_new (FALSE,
			FALSE,
			sizeof (BibtexAuthor));
}

void
bibtex_author_group_destroy (BibtexAuthorGroup * group) {
    int i;
    BibtexAuthor * field;

    g_return_if_fail (group != NULL);

    for (i = 0; i < group->len; i++) {
	field = & g_array_index (group, BibtexAuthor, i);
	
	if (field->last) {
	    g_free (field->last);
	}
	
	if (field->first) {
	    g_free (field->first);
	}
	
	if (field->lineage) {
	    g_free (field->lineage);
	}

	if (field->honorific) {
	    g_free (field->honorific);
	}
    }

    g_array_free (group, TRUE);
}

BibtexAuthor * 
bibtex_author_new (void) {
    BibtexAuthor * field;
    
    field = g_new (BibtexAuthor, 1);

    field->last  = NULL;
    field->first = NULL;
    field->lineage = NULL;
    field->honorific = NULL;

    return field;
}

void
bibtex_author_destroy (BibtexAuthor * field) {

    g_return_if_fail (field != NULL);

    if (field->last) {
	g_free (field->last);
    }
    
    if (field->first) {
	g_free (field->first);
    }
    
    if (field->lineage) {
	g_free (field->lineage);
    }

    if (field->honorific) {
	g_free (field->honorific);
    }
    
    g_free (field);
}

/* -------------------------------------------------- 
   Here comes the ugly code that decyphers and capitalizes an author
   group... still needs some work !
   -------------------------------------------------- */

typedef struct {
    gchar * text;
    guint level;
} BTGroup;

static GMemChunk * chunk = NULL;

static BTGroup * 
btgroup_new (gchar * text,
	     guint level) {
    BTGroup * group;

    if (chunk == NULL) {
	chunk = g_mem_chunk_new ("BTGroup",
				 sizeof (BTGroup),
				 sizeof (BTGroup) * 16,
				 G_ALLOC_AND_FREE);
    }

    group = g_chunk_new (BTGroup, chunk);
    group->text = text;
    group->level = level;

    return group;
}

static void
btgroup_destroy (BTGroup * group) {

    g_chunk_free (group, chunk);
}

/* this function adds the comma separated blocks to the token list */

static GList *
split_spaces (GList * tokens,
	      gchar * data,
	      guint level) {
    gchar * text, * courant, sep;
    gboolean one = TRUE;
    
    text = data;
    courant = data;

    while ((text = strchr (text, ',')) != NULL) {
	one = FALSE;

	sep  = * text;
	* text = '\0';

	if (strlen (courant) > 0) {
	    tokens = g_list_append (tokens, 
				    btgroup_new (g_strdup (courant), level));
	}

	tokens = g_list_append (tokens, btgroup_new (g_strdup (","), level));

	* text = sep;
	
	text ++;
	courant = text;
    }

    if (strlen (courant) > 0) {
	tokens = g_list_append (tokens, 
				btgroup_new (g_strdup (courant), level));
    }

    return tokens;
}


/*
  This function takes a BibtexStruct and splits it into pieces (separate words,
  ponctuation, sub-groups...)
*/

static GList *
tokenify (GList * tokens, 
	  BibtexStruct * s,
	  guint level,
	  GHashTable * dico) {

    GList * tmp;
    gchar * text, * courant;
    BibtexStruct * tmp_s;

    /* Aux niveaux plus élevés, on considère les données d'un bloc */
    if (level > 1) {
	text = bibtex_struct_as_string (s, BIBTEX_OTHER, dico, NULL);
	tokens = g_list_append (tokens, btgroup_new (text, level));

	return tokens;
    }

    switch (s->type) {

    case BIBTEX_STRUCT_LIST:
	tmp = s->value.list;

	while (tmp != NULL) {
	    tmp_s = (BibtexStruct *) tmp->data;
	    tmp   = tmp->next;

	    /* Deal with eventual commands */
	    switch (tmp_s->type) {
	    case BIBTEX_STRUCT_COMMAND:

		courant = bibtex_accent_string (tmp_s, & tmp, NULL);
		tokens  = split_spaces (tokens, courant, level);
		g_free (courant);
		break;
		    
	    default:
		tokens = tokenify (tokens, tmp_s, level, dico);
		break;
	    }
	}

	break;
	    
    case BIBTEX_STRUCT_TEXT:
	tokens = split_spaces (tokens, s->value.text, level);
	break;

    case BIBTEX_STRUCT_SPACE:
	tokens = g_list_append (tokens, btgroup_new (g_strdup (" "), level));
	break;

    case BIBTEX_STRUCT_REF:
	tmp_s = (BibtexStruct *) g_hash_table_lookup (dico, s->value.ref);

	if (tmp_s) {
	    tokens = tokenify (tokens, tmp_s, level, dico);
	}
	break;
	    
    case BIBTEX_STRUCT_SUB:
	tokens = tokenify (tokens, s->value.sub->content, level + 1, dico);
	break;
	
    case BIBTEX_STRUCT_COMMAND:
	/* Normally, commands have been considered in STRUCT_LIST */
	return NULL;
	break;
	    
    default:
	g_assert_not_reached ();
	break;
    }

    return tokens;
}


static void
text_free (gpointer data,
	   gpointer user) {
    BTGroup * group;

    group = (BTGroup *) data;
    g_free (group->text);

    btgroup_destroy (group);
}


void
extract_author (BibtexAuthorGroup * authors,
		GList * aut_elem) {

#define SECTION_LENGTH 4

    GList * tmp;
    gchar * text;
    BibtexAuthor * author;
    gint i;
    gint sections, comas;
    GPtrArray * section [SECTION_LENGTH], * array;
    BTGroup * group;

    gint lastname_section;

    /* Room for the new author */
    g_array_set_size (authors, authors->len + 1);
    author = & g_array_index (authors, BibtexAuthor, authors->len - 1);

    author->first     = NULL;
    author->last      = NULL;
    author->honorific = NULL;
    author->lineage   = NULL;

    for (i = 0; i < SECTION_LENGTH; i ++) {
	section [i]  = g_ptr_array_new ();
    }
    
    /* count the , */
    tmp   = aut_elem;
    comas = 0;

    while (tmp) {
	group = (BTGroup *) tmp->data;
	text = group->text;

	tmp = tmp->next;

	/* Check for , syntax */
	if (strcmp (",", text) == 0) {
	  comas ++;
	}
    }

/*      g_message ("%d comas", comas); */

    /* Parse the list into several groups */

    tmp      = aut_elem;
    array    = section [0];
    sections = 0;

    lastname_section = -1;

    while (tmp) {
	group = (BTGroup *) tmp->data;
	text = group->text;

	tmp = tmp->next;

	/* Check for , syntax */
	if (strcmp (",", text) == 0) {

	    /* skip eventual empty sections */
	    if (array->len) {
		sections ++;

		/* only consider the 3 first sections */
		if (sections < SECTION_LENGTH)
		    array = section [sections];
	    }

	    lastname_section = -1;
	    continue;
	}

	/* If we have the particule in lowercase */
	if (group->level == 1  &&
	    comas == 0         &&
	    islower (text [0]) && 
	    sections > 0       &&
	    lastname_section == -1) {

	    if (array->len) {
		sections ++;
		
		/* only consider the 3 first sections */
		if (sections < SECTION_LENGTH)
		    array = section [sections];
	    }

	    lastname_section = sections;

	    g_strdown (text);
	    g_ptr_array_add (array, text);
	    continue;
	}

	/* default, add new text */
	g_ptr_array_add (array, text);
    }

    /* 
       Compact the sections to skip the empty ones. Intermediate sections are
       already compressed. we only have to check the last one.
     */

    if (array->len == 0) {
	sections --;
	comas    --;
    }

    if (sections < comas) {
	comas = sections;
    }

    if (sections < 0) {
	/* no definition here, skip this author */

	bibtex_warning ("empty author definition");

	for (i = 0; i < SECTION_LENGTH; i ++) {
	    g_ptr_array_free (section [i], TRUE);
	}

	g_array_set_size (authors, authors->len - 1);
	return;
    }

    switch (comas) {
    case 1:
	/* Format: <Last Name>, <First Name> */

	g_ptr_array_add (section [0], NULL);
	g_ptr_array_add (section [1], NULL);

	author->last  = g_strjoinv (" ", (gchar **) section [0]->pdata);

	if (section [1]->len > 1) {
	    author->first = g_strjoinv (" ", (gchar **) section [1]->pdata);
	}
	break;

    case 0:
	/* Format: <prenom...> [von|de|du|del] <nom> */

	/* If there is no lowercase word indicating the last name */
	if (lastname_section == -1) {
	    /* simply take the last word of the first sequence */
	    g_ptr_array_add(section[1],  
			    g_ptr_array_index(section[0], 
					      section[0]->len-1));
	    g_ptr_array_index(section[0], section[0]->len-1) = NULL;

	    lastname_section = 1;
	}
	else {
	    g_ptr_array_add(section[0], NULL);
	}
	
	g_ptr_array_add(section[1], NULL);

	if (section[0]->len > 1) {
	    author->first = g_strjoinv(" ", (gchar**)section[0]->pdata);
	}

	author->last  = g_strjoinv(" ", (gchar**)section[lastname_section]->pdata);
	break;

    case 2:
	/* Format: <lastname>, <lineage>, <firstname> */
	g_ptr_array_add (section [0], NULL);
	g_ptr_array_add (section [1], NULL);
	g_ptr_array_add (section [2], NULL);

	author->last    = g_strjoinv (" ", (gchar **) section [0]->pdata);
	author->lineage = g_strjoinv (" ", (gchar **) section [1]->pdata);
	author->first   = g_strjoinv (" ", (gchar **) section [2]->pdata);
	break;

    default:
	/* too many comas... */
	bibtex_warning ("too many comas in author definition");

	g_ptr_array_add (section [0], NULL);
	g_ptr_array_add (section [1], NULL);

	author->last  = g_strjoinv (" ", (gchar **) section [0]->pdata);

	if (section [1]->len > 1) {
	    author->first = g_strjoinv (" ", (gchar **) section [1]->pdata);
	}
	break;
    }

    for (i = 0; i < SECTION_LENGTH; i ++) {
	g_ptr_array_free (section [i], TRUE);
    }
}

BibtexAuthorGroup *
bibtex_author_parse (BibtexStruct * s,
		     GHashTable * dico) {

    GList * list = NULL, * toremove;

    BibtexAuthorGroup * authors;
    GList * tokens, * aut_elem;
    gchar * text;
    gboolean skip;
    GList * target;
    BTGroup * group, * tmp_g;
    gboolean compact, first_pass;


    g_return_val_if_fail (s != NULL, NULL);

    authors = bibtex_author_group_new ();

    /* --------------------------------------------------
       Split into elementary tokens 
       -------------------------------------------------- */

    tokens = tokenify (NULL, s, 0, dico);
    list = tokens;

    while (list) {
	group = (BTGroup *) list->data;

	list = list->next;
    }

    /* --------------------------------------------------
       Compact the strings as much as possible 
       -------------------------------------------------- */
    compact    = FALSE;
    first_pass = TRUE;

    while (! compact) {
	compact = TRUE;
	list = tokens;
	toremove = NULL;
	skip = TRUE;
	target = NULL;
	
	/* For every token... */
	while (list) {
	    group = (BTGroup *) list->data;
	    text = group->text;
	    
	    /* Skip and remove spaces */
	    if (strcmp (" ", text) == 0) {
		skip = TRUE;
		list = list->next;
		continue;
	    }

	    /* Skip commas */
	    if (strcmp (",", text) == 0) {
		skip = TRUE;
		list = list->next;
		continue;
	    }
	    
	    /* Merge non separated entries */
	    if (! skip) {
		compact = FALSE;
		toremove = g_list_append (toremove, group);
		
		g_assert (target != NULL);
		
		/* Update data */
		tmp_g = (BTGroup *) target->data;
		target->data = btgroup_new (g_strconcat (tmp_g->text, 
							 text, NULL),
					    group->level);
		btgroup_destroy (tmp_g);

		list = list->next;
	    }
	    else {
		/* Get next entry */
		skip = FALSE;
		target = list;
		list = list->next;
	    }
	}
	
	/* If there has been no compacting, remove spaces */
	if (compact) {
	    list = tokens;
	    
	    /* For every token... */
	    while (list) {
		group = (BTGroup *) list->data;
		text = group->text;
		
		if (strcmp (" ", text) == 0) {
		    toremove = g_list_append (toremove, group);
		}
		list = list->next;
	    }
	}

	/* Remove items no more used */
	list = toremove;
	while (list) {
	    /* Remove current from list */
	    tokens = g_list_remove (tokens, list->data);
	    
	    text_free (list->data, NULL);
	    list = list->next;
	}
	g_list_free (toremove);

	/* ...and eventually start again */
	first_pass = FALSE;
    }

    /* --------------------------------------------------
       Heuristic to extract authors... 
       -------------------------------------------------- */

    aut_elem = NULL;	
    list = tokens;

    while (list) {
	group = (BTGroup *) list->data;
	text = group->text;

	list = list->next;

	if (g_strcasecmp (text, "and") == 0) {
	    if (aut_elem == NULL) {
		bibtex_warning ("double `and' in author field");
	    }
	    else {
		extract_author (authors, aut_elem);
		
		g_list_free (aut_elem);
		aut_elem = NULL;
	    }
	}
	else {
	    aut_elem = g_list_append (aut_elem, group);
	}
    }

    /* Extract last author */
    if (aut_elem == NULL) {
	bibtex_warning ("`and' at end of author field");
    }
    else {
	extract_author (authors, aut_elem);
	g_list_free (aut_elem);
    }

    g_list_foreach (tokens, text_free, NULL);
    g_list_free (tokens);

    return authors;
}

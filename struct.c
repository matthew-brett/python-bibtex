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
 
 $Id: struct.c,v 1.1.2.2 2003/09/02 14:35:33 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <strings.h>
#include <ctype.h>

#include "bibtex.h"

static GMemChunk * struct_chunk = NULL;

BibtexStruct *
bibtex_struct_new (BibtexStructType type) {
    BibtexStruct * s;
    
    if (struct_chunk == NULL) {
	struct_chunk = g_mem_chunk_new ("BibtexStruct",
					sizeof (BibtexStruct),
					sizeof (BibtexStruct) * 16,
					G_ALLOC_AND_FREE);
    }

    s = g_chunk_new (BibtexStruct, struct_chunk);

    s->type = type;

    switch (type) {
    case BIBTEX_STRUCT_LIST:
	s->value.list = NULL;
	break;
    case BIBTEX_STRUCT_TEXT:
	s->value.text = NULL;
	break;
    case BIBTEX_STRUCT_REF:
	s->value.ref = NULL;
	break;
    case BIBTEX_STRUCT_COMMAND:
	s->value.com = NULL;
	break;
    case BIBTEX_STRUCT_SUB:
	s->value.sub = g_new (BibtexStructSub, 1);
	s->value.sub->content = NULL;
	s->value.sub->encloser = BIBTEX_ENCLOSER_BRACE;
	break;
    case BIBTEX_STRUCT_SPACE:
	s->value.unbreakable = FALSE;
	break;

    default:
	g_assert_not_reached ();
    }

    return s;
}

void
bibtex_struct_destroy (BibtexStruct * s,
		       gboolean remove_content) {
    GList * current;

    g_return_if_fail (s != NULL);

    switch (s->type) {
    case BIBTEX_STRUCT_TEXT:
	if (remove_content)
	    g_free (s->value.text);
	break;

    case BIBTEX_STRUCT_REF:
	if (remove_content)
	    g_free (s->value.ref);
	break;

    case BIBTEX_STRUCT_COMMAND:
	if (remove_content)
	    g_free (s->value.com);
	break;

    case BIBTEX_STRUCT_LIST:
	if (remove_content) {
	    current = s->value.list;
	    while (current) {
		bibtex_struct_destroy ((BibtexStruct *) current->data,
				       remove_content);
		current = current->next;
	    }
	}

	g_list_free (s->value.list);
	break;

    case BIBTEX_STRUCT_SUB:
	if (remove_content)
	    bibtex_struct_destroy ((BibtexStruct *) s->value.sub->content,
				   remove_content);
	g_free (s->value.sub);
	break;

    case BIBTEX_STRUCT_SPACE:
      break;
      
    default:
	g_assert_not_reached ();
	break;
    }

    g_chunk_free (s, struct_chunk);
}

/* -------------------------------------------------- */

void
bibtex_struct_display (BibtexStruct * source) {
    GList * list;

    g_return_if_fail (source != NULL);

    switch (source->type) {
	
    case BIBTEX_STRUCT_TEXT:
	printf ("Text(%s)", source->value.text);
	break;
	
    case BIBTEX_STRUCT_REF:
	printf ("Ref(%s)", source->value.ref);
	break;

    case BIBTEX_STRUCT_COMMAND:
	printf ("Command(%s)", source->value.com);
	break;

    case BIBTEX_STRUCT_SUB:
	printf ("Sub(");
	bibtex_struct_display (source->value.sub->content);
	printf (")");
	break;

    case BIBTEX_STRUCT_LIST:
	printf ("List(");
	list = source->value.list;
	while (list) {
	    bibtex_struct_display ((BibtexStruct *) list->data);
	    list = list->next;
	}
	printf (")\n");
	break;

    case BIBTEX_STRUCT_SPACE:
	printf ("Space()");
	break;

    default:
	printf ("Argggg(%d)", source->type);
    }

}

BibtexStruct *
bibtex_struct_copy (BibtexStruct * source) {
    BibtexStruct * target;
    GList * list;

    g_return_val_if_fail (source != NULL, NULL);

    target = bibtex_struct_new (source->type);
    switch (source->type) {

    case BIBTEX_STRUCT_TEXT:
	target->value.text = g_strdup (source->value.text);
	break;

    case BIBTEX_STRUCT_REF:
	target->value.ref = g_strdup (source->value.ref);
	break;

    case BIBTEX_STRUCT_COMMAND:
	target->value.com = g_strdup (source->value.com);
	break;

    case BIBTEX_STRUCT_SUB:
	target->value.sub->encloser = source->value.sub->encloser;
	target->value.sub->content = 
	    bibtex_struct_copy (source->value.sub->content);
	break;

    case BIBTEX_STRUCT_LIST:
	list = source->value.list;
	while (list) {
	    target->value.list = 
		g_list_append (target->value.list,
			       bibtex_struct_copy ((BibtexStruct *)
						   list->data));
	    list = list->next;
	}
	break;

    case BIBTEX_STRUCT_SPACE:
	target->value.unbreakable = source->value.unbreakable;
	break;

    default:
	g_assert_not_reached ();
	break;
    }

    return target;
}

BibtexStruct *
bibtex_struct_append (BibtexStruct * s1, 
		      BibtexStruct * s2) {
    BibtexStruct * ret;
    gchar * tmp;

    if (s1 == NULL && s2 == NULL) return NULL;

    if (s1 == NULL) return s2;
    if (s2 == NULL) return s1;

    if (s1->type == BIBTEX_STRUCT_TEXT &&
	s2->type == BIBTEX_STRUCT_TEXT) {

	/* Fusion de deux bouts de texte */

	tmp = s1->value.text;
	s1->value.text = g_strconcat (s1->value.text,
				      s2->value.text,
				      NULL);
	g_free (tmp);
	bibtex_struct_destroy (s2, TRUE);
	return (s1);
    }

    if (s1->type == BIBTEX_STRUCT_LIST &&
	s2->type == BIBTEX_STRUCT_LIST) {

	/* Fusion de deux listes simples */

	s1->value.list = g_list_concat (s1->value.list, 
					s2->value.list);
	bibtex_struct_destroy (s2, FALSE);
	return (s1);
    }

    /* One or the other is a list... */
    if (s1->type == BIBTEX_STRUCT_LIST) {
	/* append the second to the first... */
	s1->value.list = g_list_append (s1->value.list, s2);
	return (s1);
    }

    if (s2->type == BIBTEX_STRUCT_LIST) {
	/* append the second to the first... */
	s2->value.list = g_list_prepend (s2->value.list, s1);
	return (s2);
    }

    /* Dans le cas general, creer une nouvelle liste contenant seulement les
       deux elements passes en entree */
    
    ret = bibtex_struct_new (BIBTEX_STRUCT_LIST);
    ret->value.list = g_list_append (ret->value.list, s1);
    ret->value.list = g_list_append (ret->value.list, s2);

    return ret;
}


static gchar *
bibtex_real_string (BibtexStruct * s,
		    BibtexFieldType type,
		    GHashTable * dico,
		    gboolean as_bibtex,
		    gint level,
		    gboolean * loss,
		    gboolean at_beginning,
		    gboolean strip_first_layer,
		    gboolean as_latex) {

    gchar * text = NULL, * tmp;
    GList * list;
    BibtexStruct * tmp_s;
    gboolean first;
    GString * string;

    g_return_val_if_fail (s != NULL, NULL);

    switch (s->type) {

    case BIBTEX_STRUCT_SPACE:
	if (as_bibtex) {
	    if (s->value.unbreakable) {
		text = g_strdup ("~");
	    } 
	    else {
		text = g_strdup (" ");
	    }
	}
	else {
	    /* unbreakable spaces are lost */
	    if (s->value.unbreakable) {
		if (loss) * loss = TRUE;
	    }
	    text = g_strdup (" ");
	}
	break;

    case BIBTEX_STRUCT_COMMAND:
	if (as_bibtex) {
	    text = g_strconcat ("\\", s->value.com, NULL);
	}
	else {
	    text = bibtex_accent_string (s, NULL, loss);
	}
	break;
	
    case BIBTEX_STRUCT_TEXT:
	text = g_strdup (s->value.text);

	if ((! as_bibtex || as_latex) && 
	    level == 1 && 
	    type == BIBTEX_TITLE) {
	    if (at_beginning) {
		text [0] = toupper (text [0]);
		g_strdown (text + 1);
	    }
	    else {
		g_strdown (text);
	    }
	}
	break;

    case BIBTEX_STRUCT_REF:
	g_strdown (s->value.ref);

	if (as_bibtex && ! as_latex) {
	    text = g_strdup (s->value.ref);
	}
	else {
	    if (loss) * loss = TRUE;

	    if (dico) {
		tmp_s = (BibtexStruct *) 
		    g_hash_table_lookup (dico, s->value.ref);
		
		if (tmp_s) {
		    text = bibtex_real_string (tmp_s, type, 
					       dico, 
					       as_bibtex,
					       level, loss, at_beginning,
					       strip_first_layer, as_latex);
		}
		else {
		    bibtex_warning ("reference `%s' undefined", s->value.text);
		    text = g_strdup ("<undefined>");
		}
	    }
	    else {
		text = g_strdup ("<undefined>");
	    }
	}
	break;

    case BIBTEX_STRUCT_LIST:
	string = g_string_new ("");

	list = s->value.list;
	first = TRUE;

	while (list != NULL) {
	    tmp_s = (BibtexStruct *) list->data;

	    list = list->next;

	    if (! as_bibtex &&
		tmp_s->type == BIBTEX_STRUCT_COMMAND) {

		/* Passer a la fonction le flot suivant */
		tmp = bibtex_accent_string (tmp_s, & list, loss);
		g_string_append (string, tmp);
		g_free (tmp);
	    }
	    else {
		if (level == 0 && as_bibtex && ! first && ! as_latex) {
		    g_string_append (string, " # ");
		}
		tmp = bibtex_real_string (tmp_s, type, dico, as_bibtex, level,
					  loss, at_beginning && first, 
					  strip_first_layer, as_latex);

		g_string_append (string, tmp);
		g_free (tmp);
	    }

	    first = FALSE;
	}

	text = string->str;
	g_string_free (string, FALSE);
	break;
	
    case BIBTEX_STRUCT_SUB:
	if (as_bibtex) {
	    tmp = bibtex_real_string (s->value.sub->content,
				      type, dico, 
				      as_bibtex,
				      /* climb one level if we are in pure
					 bibtex, or if we are in latex and {} */
				      level + 1,
				      loss, at_beginning, FALSE, as_latex);

	    if (strip_first_layer) {
		text = tmp;
	    }
	    else {
		/* As bibtex, add the encloser */
		switch (s->value.sub->encloser) {
		    
		case BIBTEX_ENCLOSER_BRACE:
		    text = g_strdup_printf ("{%s}", tmp);
		    break;
		    
		case BIBTEX_ENCLOSER_QUOTE:
		    text = g_strdup_printf ("\"%s\"", tmp);
		    break;
		
		default:
		    g_assert_not_reached ();
		    break;
		}
		
		g_free (tmp);
	    }
	}
	else {
	    /* As simple text, add nothing. */
	    text = bibtex_real_string (s->value.sub->content,
				       type, dico,
				       as_bibtex,
				       level + 1,
				       loss, at_beginning, FALSE, as_latex);
	}
	break;

    default:
	g_assert_not_reached ();
	break;
    }


    return text;
}

BibtexStruct * 
bibtex_struct_flatten (BibtexStruct * s) {
    GList * list, * newlist, * tmp;
    BibtexStruct * tmp_s;
    gboolean stable;

    g_return_val_if_fail (s != NULL, NULL);

    switch (s->type) {
    case BIBTEX_STRUCT_LIST:
	/* Aplanir une liste */
	stable = FALSE;

	while (! stable) {
	    stable = TRUE;
	    list = s->value.list;
	    
	    newlist = NULL;
	    while (list) {
		tmp_s = (BibtexStruct *) list->data;
		
		/* Si on trouve une liste dans la liste... */
		if (tmp_s->type == BIBTEX_STRUCT_LIST) {
		    /* Il faudra repasser un coup apres... */
		    stable = FALSE;
		    tmp = tmp_s->value.list;

		    /* Incorporer tous les elements de la deuxieme liste */
		    while (tmp) {
			newlist = g_list_append (newlist, tmp->data);
			tmp = tmp->next;
		    }
		    /* ...et detruire l'ancien element */
		    bibtex_struct_destroy (tmp_s, FALSE);
		}
		else {
		    newlist = g_list_append (newlist, 
					     bibtex_struct_flatten (tmp_s));
		}
		
		list = list->next;
	    }
	    g_list_free (s->value.list);
	    s->value.list = newlist;
	}

	break;

    case BIBTEX_STRUCT_SUB:
	s->value.sub->content = 
	    bibtex_struct_flatten (s->value.sub->content);
	break;

    default:
	break;
    }

    return s;
}

gchar * 
bibtex_struct_as_string (BibtexStruct * s,
			 BibtexFieldType type,
			 GHashTable * dico,
			 gboolean * loss) {
    g_return_val_if_fail (s != NULL, NULL);

    return bibtex_real_string (s, type, dico, FALSE, 0, loss, TRUE, 
			       FALSE, FALSE);
}

gchar * 
bibtex_struct_as_bibtex (BibtexStruct * s) {
    g_return_val_if_fail (s != NULL, NULL);

    return bibtex_real_string (s, BIBTEX_OTHER, NULL, TRUE, 0, NULL, TRUE,
			       FALSE, FALSE);
}

gchar * 
bibtex_struct_as_latex (BibtexStruct * s,
			BibtexFieldType type,
			GHashTable * dico) {
    g_return_val_if_fail (s != NULL, NULL);

    return bibtex_real_string (s, type, dico, TRUE, 0, NULL, TRUE,
			       TRUE, TRUE);
}

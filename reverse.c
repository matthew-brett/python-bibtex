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
 
 $Id: reverse.c,v 1.1.2.3 2003/09/10 07:49:58 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <regex.h>

#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#else /* ! HAVE_STDBOOL_H */

/* stdbool.h for GNU.  */

/* The type `bool' must promote to `int' or `unsigned int'.  The constants
   `true' and `false' must have the value 0 and 1 respectively.  */
typedef enum
  {
    false = 0,
    true = 1
  } bool;

/* The names `true' and `false' must also be made available as macros.  */
#define false	false
#define true	true

/* Signal that all the definitions are present.  */
#define __bool_true_false_are_defined	1

#endif /* HAVE_STDBOOL_H */

#include <stdio.h>

#include <recode.h>

#include "bibtex.h"


static BibtexStruct *
text_to_struct (gchar * string) {
    BibtexEntry * entry;
    BibtexStruct * s;
    static BibtexSource * source = NULL;
 
    if (source == NULL) source = bibtex_source_new ();
 
    /* parse as a string */
    if (! bibtex_source_string (source, "internal string", string)) {
        g_error ("can't create string");
    }
 
    entry = bibtex_source_next_entry (source, FALSE);
 
    if (entry == NULL) {
        bibtex_error ("can't parse entry `%s'", string);
        return NULL;
    }
 
    s = bibtex_struct_copy (entry->preamble);
     
    bibtex_entry_destroy (entry, TRUE);
 
    return s;
}

static gboolean
author_needs_quotes (gchar * string) {
  static gboolean initialized = FALSE;
  static regex_t and_re;

  if (! initialized) {
    initialized = regcomp (& and_re, "[^[:alnum:]]and[^[:alnum:]]", REG_ICASE |
                           REG_EXTENDED) == 0;
    g_assert (initialized);
  }
  return
    (strpbrk (string, ",") != NULL) ||
    (regexec (& and_re, string, 0,NULL, 0)  == 0);
}

BibtexField * 
bibtex_reverse_field (BibtexField * field,
		      gboolean use_braces,
		      gboolean do_quote) {
    BibtexStruct * s = NULL;
    gchar * string, * tmp;
    gboolean is_upper, has_space, is_command;
    gint i;
    BibtexAuthor * author;

    static GString *      st      = NULL;
    static RECODE_OUTER   outer   = NULL;
    static RECODE_REQUEST request = NULL;

    g_return_val_if_fail (field != NULL, NULL);

    if (st == NULL) st = g_string_sized_new (16);
	
    if (outer == NULL) {
	outer = recode_new_outer (false);
	g_assert (outer != NULL);
    }

    if (request == NULL) {
	request = recode_new_request (outer);
	g_assert (request != NULL);
	if (! recode_scan_request (request, "latin1..latex")) {
	    g_error ("can't create recoder");
	}
    }

    if (field->structure) {
	bibtex_struct_destroy (field->structure, TRUE);
	field->structure = NULL;
    }

    field->loss = FALSE;

    switch (field->type) {
    case BIBTEX_OTHER:
	g_return_val_if_fail (field->text != NULL, NULL);

	g_string_truncate (st, 0);

	if (! use_braces) {
	    if (strchr (field->text, '"')) {
	        use_braces = TRUE;
	    }
	}

	if (use_braces) {
	    g_string_append (st, "@preamble{{");
	}
	else {
	    g_string_append (st, "@preamble{\"");
	}

	if (do_quote) {
	  tmp = recode_string (request, field->text);
	  g_string_append (st, tmp);
	  g_free (tmp);
	}
	else {
	  g_string_append (st, field->text);
	}

	if (use_braces) {
	    g_string_append (st, "}}");
	}
	else {
	    g_string_append (st, "\"}");
	}

	s = text_to_struct (st->str);
	break;

    case BIBTEX_TITLE:
	g_return_val_if_fail (field->text != NULL, NULL);

	g_string_truncate (st, 0);
	
	if (! use_braces) {
	    if (strchr (field->text, '"')) {
	        use_braces = TRUE;
	    }
	}

	tmp = recode_string (request, field->text);

	if (use_braces) {
	    g_string_append (st, "@preamble{{");
	}
	else {
	    g_string_append (st, "@preamble{\"");
	}

	/* Put the first lower case between {} */
	string = tmp;
	if (* tmp >= 'a' && * tmp <= 'z') {
	    /* Put the beginning in lower cases */
	    g_string_append_c (st, '{');
	    g_string_append_c (st, * tmp);
	    g_string_append_c (st, '}');
	}
	else {
	    /* The first character is done */
	    g_string_append_c (st, * tmp);
	}
	
	tmp ++;

	/* check for upper cases afterward */
	is_upper   = false;
	is_command = false;

	while (* tmp) {
	    /* start a latex command */
	    if (* tmp == '\\') {

		/* eventually closes the bracket */
		if (is_upper) {
		    is_upper = false;
		    g_string_append_c (st, '}');
		}

		is_command = true;
		g_string_append_c (st, * tmp);
		tmp ++;

		continue;
	    }
	    if (is_command) {
		if (! ((* tmp >= 'a' && * tmp <= 'z') ||
		       (* tmp >= 'A' && * tmp <= 'Z'))) {
		    is_command = false;
		}
		g_string_append_c (st, * tmp);
		tmp ++;

		continue;
	    }

	    if (* tmp >= 'A' && * tmp <= 'Z') {
		if (! is_upper) {
		    g_string_append_c (st, '{');
		    is_upper = true;
		}
		g_string_append_c (st, * tmp);
	    }
	    else {
		if (is_upper) {
		    g_string_append_c (st, '}');
		    is_upper = false;
		}

		g_string_append_c (st, * tmp);
	    }
	    tmp ++;
	}

	/* eventually close the brackets */
	if (is_upper) {
	    g_string_append_c (st, '}');
	    is_upper = false;
	}
	g_free (string);

	if (use_braces) {
	    g_string_append (st, "}}");
	}
	else {
	    g_string_append (st, "\"}");
	}

	s = text_to_struct (st->str);
	break;

    case BIBTEX_AUTHOR:
	g_return_val_if_fail (field->field.author != NULL, NULL);

	g_string_truncate (st, 0);

	/* Create a simple preamble to parse */
	if (! use_braces) {
	    for (i = 0 ; i < field->field.author->len; i ++) {
		author = & g_array_index (field->field.author, BibtexAuthor, i);
		
		if (author->last && strchr (author->last, '"')) {
		    use_braces = TRUE;
		    break;
		}
		if (author->lineage && strchr (author->lineage, '"')) {
		    use_braces = TRUE;
		    break;
		}
		if (author->first && strchr (author->first, '"')) {
		    use_braces = TRUE;
		    break;
		}
	    }
	}
	
	if (use_braces) {
	    g_string_append (st, "@preamble{{");
	}
	else {
	    g_string_append (st, "@preamble{\"");
	}

	for (i = 0 ; i < field->field.author->len; i ++) {
	    author = & g_array_index (field->field.author, BibtexAuthor, i);

	    if (i != 0) {
		g_string_append (st, " and ");
	    }

	    if (author->last) {
	        /* quotes if there is no first name */
	        has_space = author_needs_quotes (author->last) ||
		  (author->first == NULL && 
		   strpbrk (author->last, " \t") != NULL);

		if (has_space) {
		    g_string_append_c (st, '{');
		}

		tmp = recode_string (request, author->last);
		g_string_append (st, tmp);
		g_free (tmp);

		if (has_space) {
		    g_string_append_c (st, '}');
		}
	    }

	    if (author->lineage) {
		g_string_append (st, ", ");

	        has_space = author_needs_quotes (author->lineage);

		if (has_space) {
		    g_string_append_c (st, '{');
		}

		tmp = recode_string (request, author->lineage);
		g_string_append (st, tmp);
		g_free (tmp);

		if (has_space) {
		    g_string_append_c (st, '}');
		}
	    }


	    if (author->first) {
		g_string_append (st, ", ");

	        has_space = author_needs_quotes (author->first);

		if (has_space) {
		    g_string_append_c (st, '{');
		}

		tmp = recode_string (request, author->first);
		g_string_append (st, tmp);
		g_free (tmp);

		if (has_space) {
		    g_string_append_c (st, '}');
		}
	    }
	}

	if (use_braces) {
	    g_string_append (st, "}}");
	}
	else {
	    g_string_append (st, "\"}");
	}

	s = text_to_struct (st->str);
	break;

    case BIBTEX_DATE:
	s = bibtex_struct_new (BIBTEX_STRUCT_TEXT);
	s->value.text = g_strdup_printf ("%d", field->field.date.year);
	break;

    default:
	g_assert_not_reached ();
    }

    field->structure = s;

    /* remove text field */
    if (field->text) {
	g_free (field->text);

	field->text = NULL;
	field->converted = FALSE;
    }

    return field;
}

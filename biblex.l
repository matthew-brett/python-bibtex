%{

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
 
 $Id: biblex.l,v 1.1.2.2 2003/09/02 14:35:33 fredgo Exp $
*/

/*  #include "parsername.h" */

#include <string.h>
#include "bibtex.h"
#include "bibparse.h"

static BibtexSource * current_source;

gboolean bibtex_parser_is_content;

extern void bibtex_next_line (void);

#define YY_USER_ACTION  current_source->offset += bibtex_parser_leng;

 
%}

DIGIT    [0-9]+
NAME     [^ \\\t{}\"@,=%#\n\r~]+
BODY 	 [^{}\\\" \n\t\r~]+

%option noyywrap
%x comment entry

%%
		if (YY_START == INITIAL) { BEGIN(comment); }

<comment>^[ \t]*@     	BEGIN(entry); return ('@'); /* Match begin of entry */

<comment>\n		bibtex_next_line (); /* Increment line number */
			
<comment>.


<comment,entry><<EOF>>  {  
    /* Indicate EOF */
    return (end_of_file); 
}

<entry>\\([a-zA-Z]+|[^a-zA-Z]) {
    /* Gestion du caractere \ */

    bibtex_parser_lval.text = g_strdup (bibtex_parser_text); 
    bibtex_tmp_string (bibtex_parser_lval.text);

    return (L_COMMAND); 
}

<entry>{BODY} {
    if (bibtex_parser_is_content) {
	/* Word in the text */
	bibtex_parser_lval.text = bibtex_tmp_string (g_strdup (bibtex_parser_text));

	return L_BODY;
    }

    /* Don't parse it if we are not inside a text field */
    current_source->offset -= bibtex_parser_leng;
    REJECT;
}


<entry>[ \t\n\r~]+ 	{
    /* Spaces handling */
    char * tmp = bibtex_parser_text;
    
    while (* tmp) {
	if (* tmp == '\n') bibtex_next_line ();
	tmp ++;
    }

    if (bibtex_parser_is_content) {
	/* Is it an unbreakable space ? */
	if (strcmp (bibtex_parser_text, "~") == 0) {
	    return L_UBSPACE;
	}
	return L_SPACE;
    }
}


<entry>{DIGIT}	 { 
    /* Lecture d'un nombre */

    bibtex_parser_lval.text = g_strdup (bibtex_parser_text); 
    bibtex_tmp_string (bibtex_parser_lval.text); 

    return (L_DIGIT); 
}


<entry>{NAME} { 
    /* Lecture d'un nom simple */

    bibtex_parser_lval.text = g_strdup (bibtex_parser_text); 
    bibtex_tmp_string (bibtex_parser_lval.text); 

    return (L_NAME); 
}

<entry>. 	{
    return bibtex_parser_text [0];
}
%%

/* Start the parser on the specified source */
void bibtex_parser_initialize (BibtexSource * source) {
    g_return_if_fail (source != NULL);
    
    /* Destroy old buffer */
    if (source->buffer) {
	bibtex_parser__delete_buffer ((YY_BUFFER_STATE) source->buffer);
    }

    switch (source->type) {
    case BIBTEX_SOURCE_FILE:
	source->buffer = (gpointer) 
	    bibtex_parser__create_buffer (source->source.file, 1024);
	break;
	
    case BIBTEX_SOURCE_STRING:
	source->buffer = (gpointer) 
	    bibtex_parser__scan_string (source->source.string);
	break;

    default:
	g_warning ("scanning nothing !");
	source->buffer = NULL;
    }
}

/* Continue parsing on the next entry */
void bibtex_parser_continue (BibtexSource * source) { 
    g_return_if_fail (source != NULL);
    
    current_source = source;
    
    bibtex_parser__switch_to_buffer ((YY_BUFFER_STATE) source->buffer);
    BEGIN (INITIAL); 
}

/* Parsing is over */
void bibtex_parser_finish (BibtexSource * source) {
    g_return_if_fail (source != NULL);
    
    if (source->buffer) {
	bibtex_parser__delete_buffer ((YY_BUFFER_STATE) source->buffer);
	source->buffer = NULL;
    }
}

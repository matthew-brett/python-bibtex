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
 
 $Id: bibparse.y,v 1.1.2.2 2003/09/02 14:35:33 fredgo Exp $
*/

/*  #include "parsername.h" */

#include <string.h>
#include "bibtex.h"

extern void bibtex_parser_initialize (BibtexSource *);
extern void bibtex_parser_continue (BibtexSource *);
extern void bibtex_parser_finish (BibtexSource *);

extern int bibtex_parser_lex (void);

int bibtex_parser_parse (void);

extern gboolean bibtex_parser_is_content;

extern int bibtex_parser_debug;

static BibtexEntry *	entry	= NULL;
static int		start_line, entry_start;
static BibtexSource *	current_source;
static gchar *	        error_string = NULL;
static gchar *	        warning_string = NULL;
static GString *        tmp_string = NULL;

static void 
nop (void) { 
    return ;
}

void 
bibtex_next_line (void) { 
    entry->length ++; 
}

void 
bibtex_analyzer_initialize (BibtexSource * source)  {
    bibtex_parser_initialize (source);
}

void 
bibtex_analyzer_finish (BibtexSource * source)  {
    g_return_if_fail (source != NULL);

    bibtex_parser_finish (source);
    
    current_source = NULL;
}
 
BibtexEntry * 
bibtex_analyzer_parse (BibtexSource * source) {
  int ret;
  gboolean is_comment;

  g_return_val_if_fail (source != NULL, NULL);

  if (! tmp_string) {
      tmp_string = g_string_new (NULL);
  }

  current_source = source;

  bibtex_parser_debug = source->debug;

  start_line  = source->line;
  entry_start = source->line + 1;

  entry = bibtex_entry_new ();

  bibtex_parser_continue (source);
  bibtex_parser_is_content = FALSE;

  ret = bibtex_parser_parse ();

  entry->start_line = entry_start;

  bibtex_tmp_string_free ();

  is_comment = (entry->type && (strcasecmp (entry->type, "comment") == 0));

  if (warning_string && ! is_comment) {
      bibtex_warning (warning_string);
  }
  
  if (ret != 0) {
      source->line += entry->length;
      
      if (error_string && ! is_comment) {
	  bibtex_error (error_string);
      }

      bibtex_entry_destroy (entry, TRUE);
      entry = NULL;
  }

  if (error_string) {
      g_free (error_string);
      error_string = NULL;
  }

  if (warning_string) {
      g_free (warning_string);
      warning_string = NULL;
  }

  return entry;
}

void 
bibtex_parser_error (char * s) {
    if (error_string) {
	g_free (error_string);
    }

    if (current_source) {
	error_string = g_strdup_printf ("%s:%d: %s", current_source->name,
					start_line + entry->length, s);
    }
    else {
	error_string = g_strdup_printf ("%d: %s", 
					start_line + entry->length, s);

    }
}

void 
bibtex_parser_warning (char * s) {
    if (current_source) {
	warning_string = g_strdup_printf ("%s:%d: %s", current_source->name,
					  start_line + entry->length, s);
    }
    else {
	warning_string = g_strdup_printf ("%d: %s", 
					  start_line + entry->length, s);

    }
}

static void 
bibtex_parser_start_error (char * s) {
    if (error_string) {
	g_free (error_string);
    }

    if (current_source) {
	error_string = g_strdup_printf ("%s:%d: %s", current_source->name,
					entry_start, s);
    }
    else {
	error_string = g_strdup_printf ("%d: %s", 
					entry_start, s);
    }
}

static void 
bibtex_parser_start_warning (char * s) {
    if (current_source) {
	warning_string = g_strdup_printf ("%s:%d: %s", current_source->name,
					  entry_start, s);
    }
    else {
	warning_string = g_strdup_printf ("%d: %s", 
					  entry_start, s);
    }
}

%}	

%union{
    gchar * text;
    BibtexStruct * body;
}

%token end_of_file
%token <text> L_NAME
%token <text> L_DIGIT
%token <text> L_COMMAND
%token <text> L_BODY
%token <text> L_SPACE
%token <text> L_UBSPACE

%type <entry> entry
%type <entry> values
%type <entry> value

%type <body> content
%type <body> simple_content
%type <body> content_brace
%type <body> content_quote
%type <body> text_quote
%type <body> text_brace
%type <body> text_part

%% 

/* Les deux types d'entrees */
/* ================================================== */
entry:	  '@' L_NAME '{' values '}' 
/* -------------------------------------------------- */
{
    entry->type = g_strdup ($2);
    g_strdown (entry->type);

    YYACCEPT; 
}
/* -------------------------------------------------- */
        | '@' L_NAME '(' values ')' 
/* -------------------------------------------------- */
{ 
    entry->type = g_strdup ($2);
    g_strdown (entry->type);

    YYACCEPT; 	
}
/* -------------------------------------------------- */
	| end_of_file		    
/* -------------------------------------------------- */
{ 
    current_source->eof = TRUE; 
    YYABORT; 
}
/* -------------------------------------------------- */
	| '@' L_NAME '(' error ')'
/* -------------------------------------------------- */
{
    if (strcasecmp ($2, "comment") == 0) {
	entry->type = g_strdup ($2);
	g_strdown (entry->type);

	yyclearin;
	YYACCEPT;
    }

    if (current_source->strict) {
	bibtex_parser_start_error ("perhaps a missing coma");
	YYABORT;
    }
    else {
	bibtex_parser_start_warning ("perhaps a missing coma.");

	entry->type = g_strdup ($2);
	g_strdown (entry->type);

	yyclearin;
	YYACCEPT;
    }
}
/* -------------------------------------------------- */
	| '@' L_NAME '{' error '}'
/* -------------------------------------------------- */
{
    if (strcasecmp ($2, "comment") == 0) {
	entry->type = g_strdup ($2);
	g_strdown (entry->type);

	yyclearin;
	YYACCEPT;
    }

    if (current_source->strict) {
	bibtex_parser_start_error ("perhaps a missing coma");
	YYABORT;
    }
    else {
	bibtex_parser_start_warning ("perhaps a missing coma");

	entry->type = g_strdup ($2);
	g_strdown (entry->type);

	yyclearin;
	YYACCEPT;
    }
}
/* -------------------------------------------------- */
	| '@' L_NAME '(' error end_of_file
/* -------------------------------------------------- */
{
    bibtex_parser_start_error ("end of file during processing");
    YYABORT;
}
/* -------------------------------------------------- */
	| '@' L_NAME '{' error end_of_file
/* -------------------------------------------------- */
{
    bibtex_parser_start_error ("end of file during processing");
    YYABORT;
}
/* -------------------------------------------------- */
	;



/* ================================================== */
/* La liste des valeurs */
/* -------------------------------------------------- */
values:	  value ',' values
/* -------------------------------------------------- */
{
    nop ();
}
/* -------------------------------------------------- */
	| value	','
/* -------------------------------------------------- */
{
    nop ();
}
/* -------------------------------------------------- */
	| value
/* -------------------------------------------------- */
{
    nop ();
}
	;




/* Une valeur */
/* ================================================== */
value:	  L_NAME '=' content 
/* -------------------------------------------------- */
{ 
    char * name;
    BibtexField * field;
    BibtexFieldType type = BIBTEX_OTHER;

    g_strdown ($1);
    field = g_hash_table_lookup (entry->table, $1);

    /* Get a new instance of a field name */
    if (field) {
	g_string_sprintf (tmp_string, "field `%s' is already defined", $1); 
	bibtex_parser_warning (tmp_string->str);

	bibtex_field_destroy (field, TRUE);
	name = $1;
    }
    else {
	name = g_strdup ($1);
    }

    /* Search its type */
    do {
	if (strcmp (name, "author") == 0) {
	    type = BIBTEX_AUTHOR;
	    break;
	}

	if (strcmp (name, "title") == 0) {
	    type = BIBTEX_TITLE;
	    break;
	}

	if (strcmp (name, "year") == 0) {
	    type = BIBTEX_DATE;
	    break;
	}
    } 
    while (0);

    /* Convert into the right field */
    field = bibtex_struct_as_field (bibtex_struct_flatten ($3),
				    type);

    g_hash_table_insert (entry->table, name, field);
}
/* -------------------------------------------------- */
	| content
/* -------------------------------------------------- */
{ 
    entry_start = start_line + entry->length;

    if (entry->preamble) {
	bibtex_parser_start_error ("entry already contains a preamble or has an unexpected comma in its key");
	YYABORT;
    }

    entry->preamble = $1;
}
/* -------------------------------------------------- */
	;



/* ================================================== */
content:    simple_content '#' content	
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_append ($1, $3);
} 
/* -------------------------------------------------- */
	  | simple_content	
/* -------------------------------------------------- */
{
    $$ = $1;
}
/* -------------------------------------------------- */
	  ;

/* 
   Definition du contenu d'un champ, encadre par des accolades
*/

/* ================================================== */
content_brace: '{' { bibtex_parser_is_content = TRUE; }
		text_brace '}'
/* -------------------------------------------------- */
{ 
    bibtex_parser_is_content = FALSE; 
    $$ = bibtex_struct_new (BIBTEX_STRUCT_SUB);

    $$->value.sub->encloser = BIBTEX_ENCLOSER_BRACE;
    $$->value.sub->content  = $3;
} 
;

/*
  Definition du contenu d'un champ encadre par des guillemets
*/
/* ================================================== */
content_quote: '"' { bibtex_parser_is_content = TRUE; }
		text_quote '"'
/* -------------------------------------------------- */
{ 
    bibtex_parser_is_content = FALSE; 
    $$ = bibtex_struct_new (BIBTEX_STRUCT_SUB);

    $$->value.sub->encloser = BIBTEX_ENCLOSER_QUOTE;
    $$->value.sub->content  = $3;
} 
;

/* ================================================== */
simple_content:   L_DIGIT 
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_new (BIBTEX_STRUCT_TEXT);
    $$->value.text = g_strdup ($1);
}
/* -------------------------------------------------- */
	       | L_NAME 
/* -------------------------------------------------- */
{
    $$ = bibtex_struct_new (BIBTEX_STRUCT_REF);
    $$->value.ref = g_strdup ($1);

    /* g_strdown ($$->value.ref); */
}
/* -------------------------------------------------- */
	       | content_brace
/* -------------------------------------------------- */
{ 
    $$ = $1;
}
/* -------------------------------------------------- */
	       | content_quote
/* -------------------------------------------------- */
{ 
    $$ = $1;
}
/* -------------------------------------------------- */
	       ;


/* ================================================== */
text_part: L_COMMAND 
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_new (BIBTEX_STRUCT_COMMAND);
    $$->value.com = g_strdup ($1 + 1);
}
/* -------------------------------------------------- */
	   | '{' text_brace '}'		
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_new (BIBTEX_STRUCT_SUB);
    $$->value.sub->encloser = BIBTEX_ENCLOSER_BRACE;
    $$->value.sub->content  = $2;
}
/* -------------------------------------------------- */
	       | L_SPACE
/* -------------------------------------------------- */
{
    $$ = bibtex_struct_new (BIBTEX_STRUCT_SPACE);
}
/* -------------------------------------------------- */
	       | L_UBSPACE
/* -------------------------------------------------- */
{
    $$ = bibtex_struct_new (BIBTEX_STRUCT_SPACE);
    $$->value.unbreakable = TRUE;
}
/* -------------------------------------------------- */
	   | L_BODY
/* -------------------------------------------------- */
{
    $$ = bibtex_struct_new (BIBTEX_STRUCT_TEXT);
    $$->value.text = g_strdup ($1);
}
/* -------------------------------------------------- */
	   ;


/* ================================================== */
text_brace:				
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_new (BIBTEX_STRUCT_TEXT);
    $$->value.text = g_strdup ("");
}
/* -------------------------------------------------- */
       | '"'  text_brace		
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_new (BIBTEX_STRUCT_TEXT);
    $$->value.text = g_strdup ("\"");

    $$ = bibtex_struct_append ($$, $2);
}
/* -------------------------------------------------- */
       | text_part text_brace 	
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_append ($1, $2);
}
/* -------------------------------------------------- */
       ;


/* ================================================== */
text_quote:				
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_new (BIBTEX_STRUCT_TEXT);
    $$->value.text = g_strdup ("");
}
/* -------------------------------------------------- */
       | text_part text_quote 	
/* -------------------------------------------------- */
{ 
    $$ = bibtex_struct_append ($1, $2);
}
/* -------------------------------------------------- */
;


%%     
	

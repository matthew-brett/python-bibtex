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
 
 $Id: bibtex.h,v 1.1.2.1 2003/09/01 19:54:46 fredgo Exp $
*/

#ifndef __bibtex_h__
#define __bibtex_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <glib.h>
#include "logging.h"
    
    /* 
       General structure for BibTeX content storing
    */
    typedef struct _BibtexStruct BibtexStruct;

    typedef enum {
	/* List holder */
	BIBTEX_STRUCT_LIST,
	/* Text body */
	BIBTEX_STRUCT_TEXT,
	/* Reference to alias */
	BIBTEX_STRUCT_REF,
	/* Sublevel, enclosed in braces or quotes */
	BIBTEX_STRUCT_SUB,
	/* \ like command */
	BIBTEX_STRUCT_COMMAND,
	/* space */
	BIBTEX_STRUCT_SPACE,
    }
    BibtexStructType;

    typedef enum {
	BIBTEX_ENCLOSER_BRACE,
	BIBTEX_ENCLOSER_QUOTE,
	BIBTEX_ENCLOSER_NONE
    }
    BibtexEncloserType;

    typedef struct {
	BibtexEncloserType encloser;
	BibtexStruct * content;
    }
    BibtexStructSub;

    struct _BibtexStruct {
	BibtexStructType type;

	union {
	    GList * list;
	    gchar * text;
	    gchar * ref;
	    gchar * com;
	    BibtexStructSub * sub;
	    gboolean unbreakable;
	} value;
    };

    /* 
       Available elementary field types 
    */

    typedef enum {
	BIBTEX_OTHER = 0,
	BIBTEX_AUTHOR,
	BIBTEX_TITLE,
	BIBTEX_DATE,
    } 
    BibtexFieldType;

    /* Single author */
    typedef struct {
	gchar * honorific;
	gchar * first;
	gchar * last;
	gchar * lineage;
    }
    BibtexAuthor;

    /* Group of authors */

    typedef GArray BibtexAuthorGroup;


    /* Date */
    typedef struct {
	gint16 year;
	gint16 month;
	gint16 day;
    }
    BibtexDateField;


    /* 
       General field declaration
    */

    typedef struct {
	gboolean converted;
	gboolean loss;

	BibtexFieldType    type;
	BibtexStruct *     structure;

	gchar * text;

	union {
	    BibtexAuthorGroup * author;
	    BibtexDateField     date;
	} field;
    }
    BibtexField;

    /*
      Full BibTeX entry
    */

    typedef struct {
	int length;
	int offset;

	int start_line;

	gchar * type;
	gchar * name;
	
	BibtexStruct * preamble;

	GHashTable * table;
    } 
    BibtexEntry;

    /*
      Full BibTeX database
    */

    typedef enum {
	BIBTEX_SOURCE_NONE,
	BIBTEX_SOURCE_FILE,
	BIBTEX_SOURCE_STRING
    }
    BibtexSourceType;

    typedef struct {
	gboolean eof, error;
	gboolean strict;

	int line;
	int offset;

	int debug;

	BibtexSourceType type;

	gchar * name;

	union {
	    FILE  * file;
	    gchar * string;
	} source;

	GHashTable * table;
	gpointer buffer;
    }
    BibtexSource;

    /* -------------------------------------------------- 
       High level interface
       -------------------------------------------------- */

    BibtexEntry * bibtex_entry_new      (void);

    void          bibtex_entry_destroy  (BibtexEntry * entry, 
					 gboolean content);


    /* Source manipulation */

    BibtexSource * bibtex_source_new (void);

    void           bibtex_source_destroy (BibtexSource * source, 
					  gboolean free_data);

    gboolean       bibtex_source_file (BibtexSource * source, gchar *
				       filename);

    gboolean       bibtex_source_string (BibtexSource * source, 
					 gchar * name,
					 gchar * string);

    /* Manipulate @string definitions in that source */
    BibtexStruct * bibtex_source_get_string (BibtexSource * source,
					     gchar * key);

    void           bibtex_source_set_string (BibtexSource * source,
					     gchar * key,
					     BibtexStruct * value);


    BibtexEntry *  bibtex_source_next_entry (BibtexSource * file, gboolean filter);

    void           bibtex_source_rewind (BibtexSource * file);

    gint           bibtex_source_get_offset (BibtexSource * file);
    
    void           bibtex_source_set_offset (BibtexSource * file, 
					     gint offset);

    /* Fields manipulation */

    BibtexField * bibtex_field_new     (BibtexFieldType type);
    void          bibtex_field_destroy (BibtexField * field, gboolean content);
    BibtexField * bibtex_field_parse   (BibtexField * field, GHashTable * dico);


    /* Authors manipulation */

    BibtexAuthor * bibtex_author_new     (void);
    void           bibtex_author_destroy (BibtexAuthor * author);

    BibtexAuthorGroup * bibtex_author_group_new     (void);
    void                bibtex_author_group_destroy (BibtexAuthorGroup * authors);
    BibtexAuthorGroup * bibtex_author_parse         (BibtexStruct * authors, 
						     GHashTable * dico);


    /* Structure allocation / manipulation */

    BibtexStruct * bibtex_struct_new     (BibtexStructType type);
    void           bibtex_struct_destroy (BibtexStruct * structure, 
					  gboolean content);

    BibtexStruct * bibtex_struct_copy    (BibtexStruct * source);
    void           bibtex_struct_display (BibtexStruct * source);
    BibtexStruct * bibtex_struct_flatten (BibtexStruct * source);
    BibtexStruct * bibtex_struct_append  (BibtexStruct *, BibtexStruct *);


    /* Structure conversions */

    gchar *       bibtex_struct_as_string (BibtexStruct * s, 
					   BibtexFieldType type, 
					   GHashTable * dico,
					   gboolean * loss);

    gchar *       bibtex_struct_as_bibtex (BibtexStruct * s);

    gchar *       bibtex_struct_as_latex  (BibtexStruct * s,
					   BibtexFieldType type,
					   GHashTable * dico);

    BibtexField * bibtex_struct_as_field  (BibtexStruct * s, 
					   BibtexFieldType type);


    /* recreate structure after modifications */

    BibtexField * bibtex_reverse_field (BibtexField * field,
					gboolean use_braces,
					gboolean do_quote);

    /* --------------------------------------------------
       Low level function
       -------------------------------------------------- */

    gchar * bibtex_accent_string (BibtexStruct * s, GList ** flow, gboolean * loss);
    void    bibtex_capitalize    (gchar * text, gboolean is_noun, gboolean at_start);

    /* Temporary strings */
    gchar * bibtex_tmp_string      (gchar *);
    void    bibtex_tmp_string_free (void);
    
    /* Parse next entry */
    BibtexEntry * bibtex_analyzer_parse (BibtexSource * file);

    /*  analyser on a file */
    void bibtex_analyzer_initialize (BibtexSource * file);
    void bibtex_analyzer_finish     (BibtexSource * file);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __DEBUGGING__
#include "debugging.h"
#endif /* __DEBUGGING__ */

#endif /* bibtex.h */

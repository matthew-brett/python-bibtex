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
 
 $Id: bibtexmodule.c,v 1.5.2.4 2003/07/31 13:18:55 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Python.h>
#include "bibtex.h"

char program_name [] = "bibtexmodule";

typedef struct {
  PyObject_HEAD
  BibtexSource *obj;
} PyBibtexSource_Object;

typedef struct {
  PyObject_HEAD
  BibtexField  *obj;
} PyBibtexField_Object;


/* Destructor of BibtexFile */
static void bibtex_py_close (PyBibtexSource_Object * self) {
    bibtex_source_destroy (self->obj, TRUE);
    PyMem_DEL (self);
}

/* Destructor of BibtexEntry */

static void destroy_field (PyBibtexField_Object * self)
{
    bibtex_field_destroy (self->obj, TRUE);

    PyMem_DEL (self);
}

static char PyBibtexSource_Type__doc__[] = "This is the type of a BibTeX source";
static char PyBibtexField_Type__doc__[]  = "This is the type of an internal BibTeX field";

static PyTypeObject PyBibtexSource_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                              /*ob_size*/
  "BibtexSource",                 /*tp_name*/
  sizeof(PyBibtexSource_Object),  /*tp_basicsize*/
  0,                              /*tp_itemsize*/
  (destructor)bibtex_py_close,    /*tp_dealloc*/
  (printfunc)0,                   /*tp_print*/
  (getattrfunc)0,                 /*tp_getattr*/
  (setattrfunc)0,                 /*tp_setattr*/
  (cmpfunc)0,                     /*tp_compare*/
  (reprfunc)0,                    /*tp_repr*/
  0,                              /*tp_as_number*/
  0,                              /*tp_as_sequence*/
  0,                              /*tp_as_mapping*/
  (hashfunc)0,                    /*tp_hash*/
  (ternaryfunc)0,                 /*tp_call*/
  (reprfunc)0,                    /*tp_str*/
  0L,0L,0L,0L,
  PyBibtexSource_Type__doc__
};

static PyTypeObject PyBibtexField_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                              /*ob_size*/
  "BibtexField",                  /*tp_name*/
  sizeof(PyBibtexField_Object) ,  /*tp_basicsize*/
  0,                              /*tp_itemsize*/
  (destructor)destroy_field,      /*tp_dealloc*/
  (printfunc)0,                   /*tp_print*/
  (getattrfunc)0,                 /*tp_getattr*/
  (setattrfunc)0,                 /*tp_setattr*/
  (cmpfunc)0,                     /*tp_compare*/
  (reprfunc)0,                    /*tp_repr*/
  0,                              /*tp_as_number*/
  0,                              /*tp_as_sequence*/
  0,                              /*tp_as_mapping*/
  (hashfunc)0,                    /*tp_hash*/
  (ternaryfunc)0,                 /*tp_call*/
  (reprfunc)0,                    /*tp_str*/
  0L,0L,0L,0L,
  PyBibtexField_Type__doc__
};



static void 
py_message_handler (const gchar *log_domain G_GNUC_UNUSED,
		    GLogLevelFlags log_level,
		    const gchar *message,
		    gpointer user_data G_GNUC_UNUSED)
{
    PyErr_SetString (PyExc_IOError, message);
}

static PyObject *
bib_open_file (PyObject * self, PyObject * args)
{
    char * name;
    BibtexSource * file;
    gint strictness;

    PyBibtexSource_Object * ret;

    if (! PyArg_ParseTuple(args, "si", & name, & strictness))
	return NULL;

    file = bibtex_source_new ();

    /* set the strictness */
    file->strict = strictness;

    if (! bibtex_source_file (file, name)) {
	bibtex_source_destroy (file, TRUE);
	return NULL;
    }

    /* Create a new object */
    ret = (PyBibtexSource_Object *) 
	PyObject_NEW (PyBibtexSource_Object, & PyBibtexSource_Type);
    ret->obj = file;

    return (PyObject *) ret;
}

static PyObject *
bib_open_string (PyObject * self, PyObject * args)
{
    char * name, * string;
    BibtexSource * file;
    gint strictness;

    PyBibtexSource_Object * ret;

    if (! PyArg_ParseTuple(args, "ssi", & name, & string, & strictness))
	return NULL;

    file = bibtex_source_new ();

    /* set the strictness */
    file->strict = strictness;

    if (! bibtex_source_string (file, name, string)) {
	bibtex_source_destroy (file, TRUE);
	return NULL;
    }

    /* Create a new object */
    ret = (PyBibtexSource_Object *) 
	PyObject_NEW (PyBibtexSource_Object, & PyBibtexSource_Type);
    ret->obj = file;

    return (PyObject *) ret;
}

static PyObject *
bib_expand (PyObject * self, PyObject * args) {
    PyObject * liste, * tmp, * auth [4];
    BibtexFieldType type;
    BibtexField * field;
    BibtexSource * file;
    PyBibtexSource_Object * file_obj;
    PyBibtexField_Object * field_obj;
    BibtexAuthor * author;

    int i, j;

    if (! PyArg_ParseTuple(args, "O!O!i:expand", 
			   &PyBibtexSource_Type, & file_obj, 
			   &PyBibtexField_Type, & field_obj, 
			   & type))
	return NULL;

    file  = file_obj->obj;
    field = field_obj->obj;

    if (! field->converted) {
	if (type != -1) {
	    field->type = type;
	}

	bibtex_field_parse (field, file->table);
    }

    switch (field->type) {
    case BIBTEX_TITLE:
    case BIBTEX_OTHER:
	tmp = Py_BuildValue ("iis", field->type, field->loss,
			     field->text);
	break;
    case BIBTEX_DATE:
	tmp = Py_BuildValue ("iisiii", 
			     field->type,
			     field->loss,
			     field->text,
			     field->field.date.year,
			     field->field.date.month,
			     field->field.date.day);
	break;

    case BIBTEX_AUTHOR:

	liste = PyList_New (field->field.author->len);

	for (i = 0; i < field->field.author->len; i++) {
	    author = & g_array_index (field->field.author, 
				      BibtexAuthor, i);
	    if (author->honorific) {
		auth [0] = PyString_FromString (author->honorific);
	    }
	    else {
		auth [0] = Py_None; 
		Py_INCREF (Py_None);
	    }

	    if (author->first) {
		auth [1] = PyString_FromString (author->first);
	    }
	    else {
		auth [1] = Py_None; 
		Py_INCREF (Py_None);
	    }

	    if (author->last) {
		auth [2] = PyString_FromString (author->last);
	    }
	    else {
		auth [2] = Py_None; 
		Py_INCREF (Py_None);
	    }

	    if (author->lineage) {
		auth [3] = PyString_FromString (author->lineage);
	    }
	    else {
		auth [3] = Py_None; 
		Py_INCREF (Py_None);
	    }

	    PyList_SetItem (liste, i,
			    Py_BuildValue ("OOOO", 
					   auth [0],
					   auth [1],
					   auth [2],
					   auth [3]));

	    for (j = 0; j < 4; j ++) {
		Py_DECREF (auth [j]);
	    }
	}
	tmp = Py_BuildValue ("iisO", 
			     field->type, 
			     field->loss, 
			     field->text, liste);
	Py_DECREF (liste);
	break;

    default:
	tmp = Py_None;
	Py_INCREF (Py_None);
    }

    return tmp;
}

static PyObject *
bib_get_native (PyObject * self, PyObject * args) {
    PyObject * tmp;
    BibtexField * field;
    PyBibtexField_Object * field_obj;
    gchar * text;

    if (! PyArg_ParseTuple(args, "O!:get_native", & PyBibtexField_Type, & field_obj))
	return NULL;

    field = field_obj->obj;

    if (field->structure == NULL) {
      Py_INCREF (Py_None);
      return Py_None;
    }

    text = bibtex_struct_as_bibtex (field->structure);
    tmp = Py_BuildValue("s", text); 
    g_free (text);

    return tmp;
}

static PyObject *
bib_copy_field (PyObject * self, PyObject * args) {
    BibtexField * field;
    PyBibtexField_Object * field_obj, * new_obj;

    if (! PyArg_ParseTuple(args, "O!:get_native", & PyBibtexField_Type, & field_obj))
	return NULL;

    field = field_obj->obj;

    new_obj = (PyBibtexField_Object *) PyObject_NEW (PyBibtexField_Object, & PyBibtexField_Type);

    new_obj->obj = bibtex_struct_as_field (bibtex_struct_copy (field->structure), field->type);

    return (PyObject *) new_obj;
}

static PyObject *
bib_get_latex (PyObject * self, PyObject * args) {
    PyObject * tmp;
    BibtexField * field;
    PyBibtexField_Object * field_obj;
    BibtexFieldType type;
    BibtexSource * file;
    PyBibtexSource_Object * file_obj;
    gchar * text;

    if (! PyArg_ParseTuple(args, "O!O!i:get_latex", 
			   &PyBibtexSource_Type, & file_obj, 
			   & PyBibtexField_Type, & field_obj,
			   & type
			   ))
	return NULL;

    field = field_obj->obj;
    file  = file_obj->obj;

    text = bibtex_struct_as_latex (field->structure,
				   type, file->table);
    tmp = Py_BuildValue("s", text); 
    g_free (text);

    return tmp;
}

static PyObject *
bib_set_native (PyObject * self, PyObject * args) {
    PyObject * tmp;
    BibtexField * field;
    static BibtexSource * source = NULL;
    BibtexEntry * entry;
    BibtexStruct * s;
    BibtexFieldType type;

    gchar * text, * to_parse;

    if (! PyArg_ParseTuple(args, "si:set_native", & text, &type))
	return NULL;

    /* Create new source */
    if (source == NULL) source = bibtex_source_new ();

    /* parse as a string */
    to_parse = g_strdup_printf ("@preamble{%s}", text);

    if (! bibtex_source_string (source, "internal string", to_parse)) {
	PyErr_SetString (PyExc_IOError, 
			 "can't create internal string for parsing");
	return NULL;
    }

    g_free (to_parse);

    entry = bibtex_source_next_entry (source, FALSE);

    if (entry == NULL) {
	return NULL;
    }

    s = bibtex_struct_copy (entry->preamble);
    bibtex_entry_destroy (entry, TRUE);

    field = bibtex_struct_as_field (s, type);

    tmp = (PyObject *) PyObject_NEW (PyBibtexField_Object, & PyBibtexField_Type);
    ((PyBibtexField_Object *) tmp)->obj = field;

    return tmp;
}


static void 
fill_dico (gpointer key, gpointer value, gpointer user)
{
    PyObject * dico = (PyObject *) user;
    PyObject * tmp1, * tmp2;

    tmp1 = PyString_FromString ((char *) key);

    tmp2 = (PyObject *) PyObject_NEW (PyBibtexField_Object, & PyBibtexField_Type);
    ((PyBibtexField_Object *) tmp2)->obj = value;

    PyDict_SetItem (dico, tmp1, tmp2);

    Py_DECREF (tmp1);
    Py_DECREF (tmp2);
}

static void 
fill_struct_dico (gpointer key, gpointer value, gpointer user)
{
    PyObject * dico = (PyObject *) user;
    PyObject * tmp1, * tmp2;

    tmp1 = PyString_FromString ((char *) key);

    tmp2 = (PyObject *) PyObject_NEW (PyBibtexField_Object, & PyBibtexField_Type);

    ((PyBibtexField_Object *) tmp2)->obj = bibtex_struct_as_field
	(bibtex_struct_copy ((BibtexStruct *) value), BIBTEX_OTHER);

    PyDict_SetItem (dico, tmp1, tmp2);

    Py_DECREF (tmp1);
    Py_DECREF (tmp2);
}

static PyObject *
bib_set_string (PyObject * self, PyObject * args)
{
    BibtexField * field;
    BibtexSource * source;
    PyBibtexSource_Object * source_obj;
    PyBibtexField_Object * field_obj;
    
    gchar * key;

    if (! PyArg_ParseTuple(args, "O!sO!:set_string", 
			   & PyBibtexSource_Type, 
			   & source_obj,
			   & key,
			   & PyBibtexField_Type,
			   & field_obj
			   ))
	return NULL;

    source = source_obj->obj;
    field  = field_obj->obj;

    /* set a copy of the struct as the field value */
    bibtex_source_set_string (source, key, 
			      bibtex_struct_copy (field->structure));

    Py_INCREF (Py_None);
    return Py_None;
}


static PyObject *
bib_next (PyObject * self, PyObject * args)
{
    BibtexEntry * ent;
    BibtexSource * file;
    PyBibtexSource_Object * file_obj;

    PyObject * dico, * tmp, * name;

    if (! PyArg_ParseTuple(args, "O!:next", & PyBibtexSource_Type, & file_obj))
	return NULL;

    file = file_obj->obj;

    ent = bibtex_source_next_entry (file, TRUE);

    if (ent == NULL) {
	if (file->eof) {
	    Py_INCREF(Py_None);
	    return Py_None;
	}

	return NULL;
    }

    /* Retour de la fonction */
    dico = PyDict_New (); 
    g_hash_table_foreach (ent->table, fill_dico, dico);

    if (ent->name) {
      name = PyString_FromString (ent->name);
    }
    else {
      name = Py_None;
    }

    tmp = Py_BuildValue ("OsiiO", name, ent->type, 
			 ent->offset, ent->start_line,
			 dico);
    Py_DECREF (dico);

    bibtex_entry_destroy (ent, FALSE);

    return tmp;
}


static PyObject *
bib_get_dict (PyObject * self, PyObject * args)
{
    BibtexSource * file;
    PyBibtexSource_Object * file_obj;

    PyObject * dico;

    if (! PyArg_ParseTuple(args, "O!:next", &PyBibtexSource_Type, & file_obj))
	return NULL;

    file = file_obj->obj;

    dico = PyDict_New (); 
    g_hash_table_foreach (file->table, fill_struct_dico, dico);

    return dico;
}


static PyObject *
bib_first (PyObject * self, PyObject * args)
{
    BibtexSource * file;
    PyBibtexSource_Object * file_obj;

    if (! PyArg_ParseTuple(args, "O!:first", &PyBibtexSource_Type, & file_obj))
	return NULL;

    file = file_obj->obj;

    bibtex_source_rewind (file);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
bib_reverse (PyObject * self, PyObject * args)
{
    BibtexField * field;
    PyObject * tuple, * authobj, * tmp;
    BibtexFieldType type;
    BibtexAuthor * auth;

    gint length, i, brace, quote;

    if (! PyArg_ParseTuple(args, "iiOi:reverse", & type, & brace, & tuple, &quote))
	return NULL;

    field = bibtex_field_new (type);

    if (field == NULL) {
	PyErr_SetString (PyExc_IOError, "can't create field");
	return NULL;
    }

    switch (field->type) {
    case BIBTEX_OTHER:
    case BIBTEX_TITLE:
	tmp = PyObject_Str (tuple);
	if (tmp == NULL) return NULL;

	field->text = g_strdup (PyString_AsString (tmp));
	Py_DECREF (tmp);
	break;

    case BIBTEX_DATE:
	tmp = PyObject_GetAttrString (tuple, "year");
	if (tmp == NULL) return NULL;

	if (tmp != Py_None)
	    field->field.date.year  = PyInt_AsLong (tmp);
	Py_DECREF (tmp);

	tmp = PyObject_GetAttrString (tuple, "month");
	if (tmp == NULL) return NULL;

	if (tmp != Py_None)
	    field->field.date.month = PyInt_AsLong (tmp);
	Py_DECREF (tmp);

	tmp = PyObject_GetAttrString (tuple, "day");
	if (tmp == NULL) return NULL;

	if (tmp != Py_None)
	    field->field.date.day   = PyInt_AsLong (tmp);
	Py_DECREF (tmp);
	break;

    case BIBTEX_AUTHOR:
	length = PySequence_Length (tuple);

	if (length < 0) return NULL;

	field->field.author = bibtex_author_group_new ();

	g_array_set_size (field->field.author, length);

	for (i = 0; i < length; i++) {
	    authobj = PySequence_GetItem (tuple, i);
	    auth    = & g_array_index (field->field.author, BibtexAuthor, i);
	    
	    tmp = PyObject_GetAttrString (authobj, "last");
	    if (tmp != Py_None) {
		auth->last = g_strdup (PyString_AsString (tmp));
	    }
	    else {
		auth->last = NULL;
	    }
	    Py_DECREF (tmp);
	    tmp = PyObject_GetAttrString (authobj, "first");
	    if (tmp != Py_None) {
		auth->first = g_strdup (PyString_AsString (tmp));
	    }
	    else {
		auth->first = NULL;
	    }
	    Py_DECREF (tmp);
	    tmp = PyObject_GetAttrString (authobj, "lineage");
	    if (tmp != Py_None) {
		auth->lineage = g_strdup (PyString_AsString (tmp));
	    }
	    else {
		auth->lineage = NULL;
	    }
	    Py_DECREF (tmp);
	    tmp = PyObject_GetAttrString (authobj, "honorific");
	    if (tmp != Py_None) {
		auth->honorific = g_strdup (PyString_AsString (tmp));
	    }
	    else {
		auth->honorific = NULL;
	    }
	    Py_DECREF (tmp);
	}
    }

    bibtex_reverse_field (field, brace, quote);

    tmp = (PyObject *) PyObject_NEW (PyBibtexField_Object, & PyBibtexField_Type);
    ((PyBibtexField_Object *) tmp)->obj = field;
    return tmp;
}


static PyObject *
bib_set_offset (PyObject * self, PyObject * args)
{
    BibtexSource * file;
    gint offset = 0;
    PyBibtexSource_Object * file_obj;

    if (! PyArg_ParseTuple(args, "O!:first", &PyBibtexSource_Type, & file_obj))
	return NULL;

    file = file_obj->obj;

    bibtex_source_set_offset (file, offset);

    if (file->error) {
	return NULL;
    }
  
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject *
bib_get_offset (PyObject * self, PyObject * args)
{
    BibtexSource * file;
    gint offset;
    PyObject * tmp;
    PyBibtexSource_Object * file_obj;

    if (! PyArg_ParseTuple(args, "O!:first", &PyBibtexSource_Type, & file_obj))
	return NULL;

    file = file_obj->obj;

    offset = bibtex_source_get_offset (file);
    
    tmp = PyInt_FromLong ((long) offset);
    return tmp;
}


static PyMethodDef bibtexMeth [] = {
    { "open_file", bib_open_file, METH_VARARGS },
    { "open_string", bib_open_string, METH_VARARGS },
    { "next", bib_next, METH_VARARGS },
    { "first", bib_first, METH_VARARGS },
    { "set_offset", bib_set_offset, METH_VARARGS },
    { "get_offset", bib_get_offset, METH_VARARGS },
    { "expand", bib_expand, METH_VARARGS },
    { "get_native", bib_get_native, METH_VARARGS },
    { "get_latex", bib_get_latex, METH_VARARGS },
    { "set_native", bib_set_native, METH_VARARGS },
    { "reverse", bib_reverse, METH_VARARGS },
    { "get_dict", bib_get_dict, METH_VARARGS },
    { "set_string", bib_set_string, METH_VARARGS },
    { "copy_field", bib_copy_field, METH_VARARGS },
    {NULL, NULL, 0},
};


void init_bibtex (void)
{
    bibtex_set_default_handler ();

    g_log_set_handler (G_LOG_DOMAIN, BIB_LEVEL_ERROR,   
		       py_message_handler, NULL);

    (void) Py_InitModule("_bibtex", bibtexMeth);
}


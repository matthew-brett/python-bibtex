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
 
 $Id: recodemodule.c,v 1.1.1.1.2.5 2003/08/13 21:56:11 fredgo Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#include <Python.h>

#include <stdio.h>
#include <recodext.h>

static RECODE_OUTER outer;
char * program_name = "pyrecode";

typedef struct {
  PyObject_HEAD
  RECODE_REQUEST obj;
} PyRecodeRequest_Object;

/* Destructor of BibtexFile */
static void py_delete_recoder (PyRecodeRequest_Object * self) {
    recode_delete_request (self->obj);
    PyObject_DEL (self);
}

static char PyRecodeRequest_Type__doc__[]  = "This is the type of a recoder";

static PyTypeObject PyRecodeRequest_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                              /*ob_size*/
  "RecodeRequest",                 /*tp_name*/
  sizeof(PyRecodeRequest_Object), /*tp_basicsize*/
  0,                              /*tp_itemsize*/
  (destructor)py_delete_recoder,  /*tp_dealloc*/
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
  PyRecodeRequest_Type__doc__
};

static PyObject *
py_new_recoder (PyObject * self, PyObject * args) {
    PyRecodeRequest_Object * ret;

    char * string;
    RECODE_REQUEST request;

    if (! PyArg_ParseTuple(args, "s:request", & string))
	return NULL;

    request = recode_new_request (outer);

    if (request == NULL) {
	PyErr_SetString (PyExc_RuntimeError, "can't initialize new request");
	return NULL;
    }

    if (! recode_scan_request (request, string)) {
        recode_delete_request (request);
	PyErr_SetString (PyExc_TypeError, "can't initialize request");
	return NULL;
    }

    /* Create a new object */
    ret = (PyRecodeRequest_Object *) 
	PyObject_NEW (PyRecodeRequest_Object, & PyRecodeRequest_Type);
    ret->obj = request;

    return (PyObject *) ret;
}

static PyObject *
py_recode (PyObject * self, PyObject * args) {
    char * string;
    int length;

    RECODE_REQUEST request;

    PyObject * tmp;
    PyRecodeRequest_Object * req_obj;

    if (! PyArg_ParseTuple(args, "O!s:recode", & PyRecodeRequest_Type, & req_obj, & string))
	return NULL;

    request = req_obj->obj;
    length  = strlen (string);

    if (length == 0) {
      return PyString_FromString ("");
    }

    string = recode_string (request, string);

    if (string == NULL) {
      PyErr_SetString (PyExc_RuntimeError, "can't convert");
      return NULL;
    }

    tmp = PyString_FromString (string);
    free (string);

    return tmp;
}

static PyMethodDef recodeMeth [] = {
    { "request", py_new_recoder, METH_VARARGS },
    { "recode", py_recode, METH_VARARGS },
    {NULL, NULL, 0},
};


void init_recode (void)
{
    outer = recode_new_outer (true);
    if (outer == NULL) return;

    (void) Py_InitModule ("_recode", recodeMeth);
}


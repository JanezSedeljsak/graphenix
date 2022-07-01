#include <stdio.h>

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "schema.h"
#include "methods.c"

static PyObject *heartbeat(PyObject *self, PyObject *args)
{
  return PyLong_FromLong(12l);
}

static PyObject *create_schema(PyObject *self, PyObject *args)
{
  PyObject *seq;
  char *schema_name;
  int8_t models_count;
  MDef **models;
  PARSE_SCHEMA(args, seq, schema_name, models, models_count);

  SCHEMA_free_models(models, models_count);
  Py_DECREF(seq);
  Py_RETURN_TRUE;
}

static PyObject *delete_schema(PyObject *self, PyObject *args)
{
  Py_RETURN_TRUE;
}

static PyObject *migrate_schema(PyObject *self, PyObject *args)
{
  PyObject *seq;
  char *schema_name;
  int8_t models_count;
  MDef **models;
  PARSE_SCHEMA(args, seq, schema_name, models, models_count);

  SCHEMA_free_models(models, models_count);
  Py_DECREF(seq);
  Py_RETURN_TRUE;
}

static PyObject *print_schema(PyObject *self, PyObject *args)
{
  PyObject *seq;
  char *schema_name;
  int8_t models_count;
  MDef **models;
  PARSE_SCHEMA(args, seq, schema_name, models, models_count);

  printf("\nSchema[%s, models_count=%d]:\n", schema_name, models_count);
  SCHEMA_print_models(models, models_count);

  SCHEMA_free_models(models, models_count);
  Py_DECREF(seq);
  Py_RETURN_TRUE;
}

static PyMethodDef Methods[] = {
    /* testing purposes function (returns 12) */
    {"heartbeat", heartbeat, METH_NOARGS, NULL},

    /* working with db itself */
    {"migrate_schema", migrate_schema, METH_VARARGS, NULL},
    {"delete_schema", delete_schema, METH_VARARGS, NULL},
    {"create_schema", create_schema, METH_VARARGS, NULL},
    {"print_schema", print_schema, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef pybase_core_module = {
    PyModuleDef_HEAD_INIT,
    "pybase_core",
    "Python c extension for data management...",
    -1,
    Methods};

PyMODINIT_FUNC PyInit_pybase_core(void)
{
  return PyModule_Create(&pybase_core_module);
};
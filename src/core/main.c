#include <stdio.h>

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "schema.h"
#include "schema.c"
#include "globals.h"

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
  PARSE_SCHEMA(args, seq, schema_name, models, models_count, 0);
  if (!SCHEMA_create_base(models, models_count, schema_name))
    return NULL;

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
  PARSE_SCHEMA(args, seq, schema_name, models, models_count, 0);

  SCHEMA_free_models(models, models_count);
  Py_DECREF(seq);
  Py_RETURN_TRUE;
}

static PyObject *load_schema(PyObject *self, PyObject *args)
{
  char *schema_name;
  int8_t *models_count = calloc(1, sizeof(int8_t));

  if (!PyArg_ParseTuple(args, "z", &schema_name))
    return NULL;

  MDef **models = SCHEMA_read_base(models_count, schema_name);
  PyObject *res = SCHEMA_pythonize_models(models, *models_count);

  SCHEMA_free_models(models, *models_count);
  free(models_count);
  return res;
}

static PyObject *stringify_schema(PyObject *self, PyObject *args)
{
  PyObject *seq;
  char *schema_name;
  int8_t models_count;
  MDef **models;
  PARSE_SCHEMA(args, seq, schema_name, models, models_count, 0);

  char *str_builder = calloc(1000, sizeof(char));
  sprintf(str_builder, "Schema[%s, models_count=%d]\n", schema_name, models_count);
  int16_t offset = strlen(str_builder);
  SCHEMA_stringify_models(str_builder, offset, models, models_count);

  SCHEMA_free_models(models, models_count);
  Py_DECREF(seq);
  PyObject *res = PyUnicode_FromString(str_builder);
  free(str_builder);
  return res;
}

static PyMethodDef Methods[] = {
    /* testing purposes function (returns 12) */
    {"heartbeat", heartbeat, METH_NOARGS, NULL},

    /* schema operations */
    {"migrate_schema", migrate_schema, METH_VARARGS, NULL},
    {"delete_schema", delete_schema, METH_VARARGS, NULL},
    {"create_schema", create_schema, METH_VARARGS, NULL},
    {"load_schema", load_schema, METH_VARARGS, NULL},
    {"stringify_schema", stringify_schema, METH_VARARGS, NULL},
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
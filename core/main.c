#include <stdio.h>

//#include "methods.c"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "methods.h"

static PyObject *heartbeat(PyObject *self, PyObject *args)
{
  return PyLong_FromLong(12l);
}

static PyObject *create_schema(PyObject *self, PyObject *args)
{
  PyObject *seq;
  //char schema_name[30];
  int seqlen;
  int i;

  if (!PyArg_ParseTuple(args, "O", &seq))
    return NULL;

  seq = PySequence_Fast(seq, "Argument must be iterable");
  if (!seq)
    return NULL;

  seqlen = PySequence_Fast_GET_SIZE(seq);
  for (i = 0; i < seqlen; i++)
  {
    PyObject *item = PySequence_Fast_GET_ITEM(seq, i);
    PyObject *seq_itt = PySequence_Fast(item, "Argument must be iterable");

    PyObject *str = PyUnicode_AsEncodedString(PySequence_Fast_GET_ITEM(seq_itt, 0), "utf-8", "~E~");
    const char *bytes = PyBytes_AS_STRING(str);
    printf("%s %ld:\n", bytes, PyLong_AsLong(PySequence_Fast_GET_ITEM(seq_itt, 1)));

    PyObject *slots = PySequence_Fast(PySequence_Fast_GET_ITEM(seq_itt, 2), "Argument must be iterable");
    int slotslen = PySequence_Fast_GET_SIZE(slots);

    for (int j = 0; j < slotslen; j++)
    {

      PyObject *slot = PySequence_Fast_GET_ITEM(slots, j);
      PyObject *slot_seq = PySequence_Fast(slot, "Argument must be iterable");
      PyObject *slot_str = PyUnicode_AsEncodedString(PySequence_Fast_GET_ITEM(slot_seq, 0), "utf-8", "~E~");
      const char *slot_c_str = PyBytes_AS_STRING(slot_str);
      printf("%s %ld\n", slot_c_str, PyLong_AsLong(PySequence_Fast_GET_ITEM(slot_seq, 1)));
    }
    printf("end slots\n");
    Py_DECREF(item);
  }

  /* clean up, compute, and return result */
  Py_DECREF(seq);
  Py_RETURN_TRUE;
}

static PyObject *delete_schema(PyObject *self, PyObject *args)
{
  Py_RETURN_TRUE;
}

static PyObject *migrate_schema(PyObject *self, PyObject *args)
{
  Py_RETURN_TRUE;
}

static PyMethodDef Methods[] = {
    /* testing purposes function (returns 12) */
    {"heartbeat", heartbeat, METH_NOARGS, NULL},

    /* working with db itself */
    {"migrate_schema", migrate_schema, METH_NOARGS, NULL},
    {"delete_schema", delete_schema, METH_NOARGS, NULL},
    {"create_schema", create_schema, METH_VARARGS, NULL},
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
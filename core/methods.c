#include "methods.h"

#include <Python.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

static PyObject *heartbeat(PyObject *self, PyObject *args)
{
  return PyLong_FromLong(12l);
}

static PyObject *create_base(PyObject *self, PyObject *args)
{
  /*if (mkdir("./some/directory", S_IRWXU | S_IRWXG | S_IRWXO) == -1)
  {
    printf("Error: %s\n", strerror(errno));
    PyErr_SetString(PyExc_TypeError, "Error occured while creating database!");
    return NULL;
  }*/

  return Py_True;
}
static PyObject *delete_base(PyObject *self, PyObject *args)
{
  return Py_True;
}

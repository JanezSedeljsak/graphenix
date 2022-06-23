
#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

static PyObject* hello_world(PyObject *self, PyObject *args) {
    long x = 25l;
    return PyLong_FromLong(x);
}

static PyMethodDef Methods[] = {
   { "hello_world", (PyCFunction)hello_world, METH_NOARGS, NULL },
   { NULL, NULL, 0, NULL }
};

static struct PyModuleDef pybasemodule = {
  PyModuleDef_HEAD_INIT,
  "pybase",
  "Python c extension for data management",
  -1,
  Methods
};

PyMODINIT_FUNC PyInit_pybase(void) {
  return PyModule_Create(&pybasemodule);
};
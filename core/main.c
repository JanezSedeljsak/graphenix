#include <stdio.h>

#include "methods.c"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

static PyMethodDef Methods[] = {
    /* testing purposes function (returns 12) */
    {"heartbeat", heartbeat, METH_NOARGS, NULL},

    /* working with db itself */
    {"create_base", create_base, METH_NOARGS, NULL},
    {"delete_base", delete_base, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef pybasemodule = {
    PyModuleDef_HEAD_INIT,
    "pybase",
    "Python c extension for data management",
    -1,
    Methods};

PyMODINIT_FUNC PyInit_pybase(void)
{
  return PyModule_Create(&pybasemodule);
};
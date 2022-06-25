#ifndef METHODS_H
#define METHODS_H
#include <Python.h>

static PyObject *heartbeat(PyObject *self, PyObject *args);

static PyObject *create_base(PyObject *self, PyObject *args);
static PyObject *delete_base(PyObject *self, PyObject *args);

#endif /* METHODS_H */
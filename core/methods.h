#ifndef METHODS_H
#define METHODS_H
#include <Python.h>

/*
class _Type(Enum):
    UNKNOWN = 0
    BOOL = 1
    INT = 2
    STRING = 3
    TEXT = 4
    DATETIME = 5
    FILE = 6
    LINK = 7
*/

#define UNKNOWN 0
#define BOOL 1
#define INT 2
#define STRING 3
#define TEXT 4
#define DATETIME 5
#define FILE 6
#define LINK 7

typedef struct _fdef
{
    char name[30];
    int8_t cfg;
} FDef;

typedef struct _mdef
{
    char name[30];
    bool is_lazy;
    FDef fields[10];
} MDef;

static PyObject *heartbeat(PyObject *self, PyObject *args);

static PyObject *create_schema(PyObject *self, PyObject *args);
static PyObject *migrate_schema(PyObject *self, PyObject *args);
static PyObject *delete_schema(PyObject *self, PyObject *args);

#endif /* METHODS_H */
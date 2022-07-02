#ifndef SCHEMA_H
#define SCHEMA_H

#include <Python.h>
#include <stdbool.h>

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

#define INDEX 16
#define MULTI 8

char TYPE_ARR[8][10] = {
    "UNKNOWN",
    "BOOL",
    "INT",
    "STRING",
    "TEXT",
    "DATETIME",
    "FILE",
    "LINK"};

typedef struct _fdef
{
    char name[30];
    int8_t cfg;
} FDef;

typedef struct _mdef
{
    char name[30];
    bool is_lazy;
    int8_t field_count;
    FDef **fields;
} MDef;

typedef struct _cfg
{
    int8_t type;
    bool multi;
    bool index;
} Cfg;

// cfg methods
static char *type_str(const int8_t);
static Cfg *load_config(const int8_t);
static int8_t compress_cfg(Cfg *);

// schema methods
static MDef **SCHEMA_parse_models(const PyObject *, int8_t *);
static void SCHEMA_print_models(MDef **, const int8_t);
static void SCHEMA_free_models(MDef **, const int8_t);
static PyObject *pythonize_models(MDef **, const int8_t);

// schema filesystem operations
static void SCHEMA_create_base(MDef **, const int8_t);
static void SCHEMA_read_base(MDef **, int8_t, const char *);

#define PARSE_SCHEMA(args, seq, schema_name, models, models_count) \
    {                                                              \
        if (!PyArg_ParseTuple(args, "zO", &schema_name, &seq))     \
            return NULL;                                           \
        seq = PySequence_Fast(seq, "Models must be iterable!");    \
        if (!seq)                                                  \
            return NULL;                                           \
        models = SCHEMA_parse_models(seq, &models_count);          \
    }

#endif /* SCHEMA_H */
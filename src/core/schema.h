#ifndef SCHEMA_H
#define SCHEMA_H

#include <Python.h>
#include <stdbool.h>

#define UNKNOWN 0
#define BOOL 1
#define INT 2
#define STRING 3
#define TEXT 4
#define DATETIME 5
#define BLOB 6
#define LINK 7

#define INDEX 16
#define MULTI 8

#define SCHEMA_PATH_BASE "./_pyb/"
#define SCHEMA_DEF "/sdef.bin"
//#define SCHEMA_DEF_OFFSETS "/sdef_offsets.bin"

char TYPE_ARR[8][10] = {
    "UNKNOWN",
    "BOOL",
    "INT",
    "STRING",
    "TEXT",
    "DATETIME",
    "BLOB",
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

#define MODEL_SIZE_WITHOUT_FIELDS (sizeof(MDef) - sizeof(FDef **))
#define ZERO_VERSION 0

// cfg methods
static Cfg *load_config(const int8_t);
static int8_t compress_cfg(Cfg *);

// schema methods
static MDef **SCHEMA_parse_models(const PyObject *, int8_t *);
static void SCHEMA_free_models(MDef **, const int8_t);
static PyObject *SCHEMA_pythonize_models(MDef **, const int8_t);

// schema filesystem operations
static bool SCHEMA_create_base(MDef **, const int8_t, const char *);
static MDef **SCHEMA_read_base(int8_t *, const char *);
static void SCHEMA_stringify_models(char *, int16_t, MDef **, const int8_t);

#define PARSE_SCHEMA(args, seq, schema_name, models, models_count, is_create) \
    {                                                                         \
        if (is_create)                                                        \
            return NULL;                                                      \
        if (!PyArg_ParseTuple(args, "zO", &schema_name, &seq))                \
            return NULL;                                                      \
        seq = PySequence_Fast(seq, "Models must be iterable!");               \
        if (!seq)                                                             \
            return NULL;                                                      \
        models = SCHEMA_parse_models(seq, &models_count);                     \
    }

#define MAKE_MODEL_PATH(path_str, schema_path, model_name) \
    {                                                      \
        strcpy(def_path, schema_path);                     \
        strcat(def_path, "/");                             \
        strcat(def_path, model_name);                      \
        strcat(def_path, ".bin");                          \
    }

#endif /* SCHEMA_H */
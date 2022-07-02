#include "schema.h"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*typedef struct _user
{
    u_int64_t id;
    char name[6];
    char surname[10];
    char email[30];
    char password[8];
} user;

static PyObject *create_base(PyObject *self, PyObject *args)
{
  if (mkdir("./_pyb/bazica", 0777) == -1)
  {
    printf("Error: %s\n", strerror(errno));
    PyErr_SetString(PyExc_TypeError, "Error occured while creating database!");
    return NULL;
  }

  FILE *user_schema_stream = fopen("./_pyb/bazica/user_schema.bin", "wb");
  FILE *schema_definition_stream = fopen("./_pyb/bazica/schema_definition.bin", "wb");
  user *usr = calloc(1, sizeof(*usr));

  for (int i = 0; i < 1000; i++) {
    usr->id = i+1;
    strcpy(usr->name, "Janez");
    strcpy(usr->surname, "Sedeljsak");
    strcpy(usr->email, "janez.sedeljsak@gmail.com");
    strcpy(usr->password, "test123");

    fwrite(usr, sizeof(*usr), 1, user_schema_stream);
  }

  fclose(user_schema_stream);
  fclose(schema_definition_stream);
  Py_RETURN_TRUE;
}*/

static char *type_str(const int8_t type)
{
  return TYPE_ARR[type];
}

static Cfg *load_config(const int8_t cfg_value)
{
  int8_t temp = cfg_value;
  Cfg *res = calloc(1, sizeof(Cfg));
  res->index = temp > (INDEX - 1);
  temp %= INDEX;
  res->multi = temp > (MULTI - 1);
  temp %= MULTI;
  res->type = temp;
  return res;
}

static int8_t compress_cfg(Cfg *cfg)
{
  // self.index * 16 + self.multi * 8 + self.type.value
  return cfg->index * INDEX + cfg->multi * MULTI + cfg->type;
}

static MDef **SCHEMA_parse_models(const PyObject *seq, int8_t *n)
{
  int8_t i;
  *n = PySequence_Fast_GET_SIZE(seq);
  MDef **res = calloc(*n, sizeof(MDef *));

  for (i = 0; i < *n; i++)
  {
    res[i] = calloc(1, sizeof(MDef));
    char *model_name;

    PyObject *item = PySequence_Fast_GET_ITEM(seq, i), *slots;
    if (!PyArg_ParseTuple(item, "ziO", &model_name, &res[i]->is_lazy, &slots))
      return NULL;

    slots = PySequence_Fast(slots, "Slots must be iterable");
    if (!slots)
      return NULL;

    res[i]->field_count = PySequence_Fast_GET_SIZE(slots);
    strcpy(res[i]->name, model_name);
    FDef **fields = calloc(res[i]->field_count, sizeof(FDef *));

    for (int j = 0; j < res[i]->field_count; j++)
    {
      PyObject *slot = PySequence_Fast_GET_ITEM(slots, j);
      fields[j] = calloc(1, sizeof(FDef));

      char *field_name;
      if (!PyArg_ParseTuple(slot, "zi", &field_name, &fields[j]->cfg))
      {
        return NULL;
      }

      strcpy(fields[j]->name, field_name);
      res[i]->fields = fields;
      Py_DECREF(slot);
    }

    Py_DECREF(item);
    Py_DECREF(slots);
  }

  return res;
}

static void SCHEMA_print_models(MDef **models, const int8_t n)
{
  int8_t i, j;
  for (i = 0; i < n; i++)
  {
    const MDef *cur_model = models[i];
    printf("\t%s(is_lazy=%d, field_count=%hhd):\n", cur_model->name, cur_model->is_lazy, cur_model->field_count);
    for (j = 0; j < cur_model->field_count; j++)
    {
      const FDef *field = cur_model->fields[j];
      Cfg *cfg = load_config(field->cfg);
      char *type = type_str(cfg->type);
      printf("\t\t%s(type=%s, multi=%d, index=%d)\n", field->name, type, cfg->multi, cfg->index);
    }
  }
}

static void SCHEMA_free_models(MDef **models, const int8_t n)
{
  int8_t i, j;
  for (i = 0; i < n; i++)
  {
    const MDef *cur_model = models[i];
    for (j = 0; j < cur_model->field_count; j++)
    {
      free(cur_model->fields[j]);
    }
    free(cur_model->fields);
    free(models[i]);
  }
  free(models);
}

static void SCHEMA_create_base(MDef **models, const int8_t n)
{
}

static void SCHEMA_read_base(MDef **models, int8_t n, const char *schema_name)
{
}

static PyObject *pythonize_models(MDef **models, const int8_t n)
{
  Py_RETURN_TRUE;
}

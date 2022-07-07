#include "schema.h"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int16_t fields_size = 0;

    for (int j = 0; j < res[i]->field_count; j++)
    {
      PyObject *slot = PySequence_Fast_GET_ITEM(slots, j);
      fields[j] = calloc(1, sizeof(FDef));
      int16_t cfg_value;

      char *field_name;
      if (!PyArg_ParseTuple(slot, "zii", &field_name, &cfg_value, &fields[j]->size))
      {
        return NULL;
      }

      fields[j]->cfg = cfg_value;
      fields_size += fields[j]->size;
      strcpy(fields[j]->name, field_name);
      Py_DECREF(slot);
    }

    res[i]->fields_size = fields_size;
    res[i]->fields = fields;
    Py_DECREF(item);
    Py_DECREF(slots);
  }

  return res;
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

static bool SCHEMA_create_base(MDef **models, int8_t n, const char *schema_name)
{
  FILE *sdef_stream, *model_stream; //, *sdef_offset_stream;
  char *schema_path = calloc(1000, sizeof(char));
  strcpy(schema_path, SCHEMA_PATH_BASE);
  strcat(schema_path, schema_name);

  if (mkdir(schema_path, 0777) == -1)
  {
    printf("Error: %s\n", strerror(errno));
    PyErr_SetString(PyExc_RuntimeError, "Error occured while creating database!");
    return false;
  }

  char def_path[1000];
  strcpy(def_path, schema_path);
  strcat(def_path, SCHEMA_DEF);

  sdef_stream = fopen(def_path, "wb");

  // strcpy(schema_path_cpy, schema_path);
  // strcat(schema_path_cpy, SCHEMA_DEF_OFFSETS);
  // sdef_offset_stream = fopen(schema_path_cpy, "wb");

  int8_t i, j, version = ZERO_VERSION;
  // int32_t *offset = calloc(1, sizeof(int32_t));

  // write number of models and version
  fwrite(&n, 1, 1, sdef_stream);
  fwrite(&version, 1, 1, sdef_stream);
  //*offset = 2;

  for (i = 0; i < n; i++)
  {
    const MDef *cur_model = models[i];
    MAKE_MODEL_PATH(def_path, schema_path, models[i]->name);

    model_stream = fopen(def_path, "wb");
    fwrite(cur_model, MODEL_SIZE_WITHOUT_FIELDS, 1, sdef_stream);
    // fwrite(offset, sizeof(int32_t), 1, sdef_offset_stream);
    //*offset += MODEL_SIZE_WITHOUT_FIELDS;

    for (j = 0; j < cur_model->field_count; j++)
    {
      fwrite(cur_model->fields[j], sizeof(FDef), 1, sdef_stream);
      //*offset += sizeof(FDef);
    }
    fclose(model_stream);
  }
  // fwrite(offset, sizeof(int32_t), 1, sdef_offset_stream);

  free(schema_path);
  // free(schema_path_cpy);
  fclose(sdef_stream);
  // fclose(sdef_offset_stream);
  return true;
}

static MDef **SCHEMA_read_base(int8_t *size, const char *schema_name)
{
  FILE *sdef_stream; //, *sdef_offset_stream;
  char *schema_path = calloc(1000, sizeof(char));
  strcpy(schema_path, SCHEMA_PATH_BASE);
  strcat(schema_path, schema_name);

  strcat(schema_path, SCHEMA_DEF);
  sdef_stream = fopen(schema_path, "rb");

  // strcpy(schema_path_cpy, schema_path);
  // strcat(schema_path_cpy, SCHEMA_DEF_OFFSETS);
  // sdef_offset_stream = fopen(schema_path_cpy, "rb");

  int8_t ver;
  fread(size, 1, 1, sdef_stream);
  fread(&ver, 1, 1, sdef_stream);
  // int32_t *offset = calloc(1, sizeof(int32_t));

  // fread(offset, sizeof(int32_t), 1, sdef_offset_stream);
  MDef **models = calloc(*size, sizeof(MDef *));

  for (int8_t i = 0; i < *size; i++)
  {
    MDef *def = calloc(1, sizeof(MDef));
    fread(def, MODEL_SIZE_WITHOUT_FIELDS, 1, sdef_stream);
    def->fields = calloc(def->field_count, sizeof(FDef *));
    for (int8_t j = 0; j < def->field_count; j++)
    {
      FDef *field_def = calloc(1, sizeof(FDef));
      fread(field_def, sizeof(FDef), 1, sdef_stream);
      def->fields[j] = field_def;
    }

    models[i] = def;
    // fread(offset, sizeof(int32_t), 1, sdef_offset_stream);
  }

  free(schema_path);
  // free(schema_path_cpy);
  fclose(sdef_stream);
  // fclose(sdef_offset_stream);
  return models;
}

static PyObject *SCHEMA_pythonize_models(MDef **models, const int8_t n)
{
  Py_RETURN_TRUE;
}

static void SCHEMA_stringify_models(char *str_builder, int16_t offset, MDef **models, const int8_t n)
{
  int8_t i, j;
  for (i = 0; i < n; i++)
  {
    const MDef *cur_model = models[i];
    sprintf(str_builder + offset, "\t%s(is_lazy=%d, field_count=%hhd, record_size=%hd):\n", cur_model->name, cur_model->is_lazy, cur_model->field_count, cur_model->fields_size);
    offset = strlen(str_builder);
    for (j = 0; j < cur_model->field_count; j++)
    {
      const FDef *field = cur_model->fields[j];
      Cfg *cfg = load_config(field->cfg);
      char *type = TYPE_ARR[cfg->type];
      sprintf(str_builder + offset, "\t\t%s(type=%s, size=%hd, multi=%d, index=%d)\n", field->name, type, field->size, cfg->multi, cfg->index);
      offset = strlen(str_builder);
    }
  }
}

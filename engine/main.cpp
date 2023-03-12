#include <Python.h>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <numeric>
#include "util.cpp"
#include "managers/managers.h"
#include "managers/schema_manager.cpp"
#include "managers/record_manager.cpp"

using namespace std;

static PyObject *create_schema(PyObject *self, PyObject *args)
{
    const char *schema_name;
    PyObject *py_model_names;

    if (!PyArg_ParseTuple(args, "sO", &schema_name, &py_model_names))
    {
        return NULL;
    }

    if (!PyList_Check(py_model_names))
    {
        PyErr_SetString(PyExc_TypeError, "Model names must be a list");
        return NULL;
    }

    int num_models = PyList_Size(py_model_names);
    vector<string> model_names;

    for (int i = 0; i < num_models; i++)
    {
        PyObject *py_model_name = PyList_GetItem(py_model_names, i);
        const char *model_name = PyUnicode_AsUTF8(py_model_name);
        model_names.push_back(string(model_name));
    }

    SchemaManager::create_schema(schema_name, model_names);
    Py_RETURN_NONE;
}

static PyObject *delete_schema(PyObject *self, PyObject *args)
{
    const char *schema_name;

    if (!PyArg_ParseTuple(args, "s", &schema_name))
    {
        return NULL;
    }

    SchemaManager::delete_schema(schema_name);
    Py_RETURN_NONE;
}

static PyObject *schema_exists(PyObject *self, PyObject *args)
{
    const char *schema_name;

    if (!PyArg_ParseTuple(args, "s", &schema_name))
    {
        return NULL;
    }

    bool exists = SchemaManager::schema_exists(schema_name);
    if (exists)
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

static PyObject *schema_add_record(PyObject *self, PyObject *args)
{
    const char *db_name;
    const char *table_name;
    PyObject *py_values;

    if (!PyArg_ParseTuple(args, "ssO", &db_name, &table_name, &py_values))
    {
        return NULL;
    }

    vector<string> values;
    PyObject *iterator = PyObject_GetIter(py_values);
    PyObject *item;

    while ((item = PyIter_Next(iterator)))
    {
        const char *str = PyUnicode_AsUTF8(item);
        values.push_back(string(str));
        Py_DECREF(item);
    }

    Py_DECREF(iterator);

    int64_t offset = RecordManager::create_record(db_name, table_name, values);
    return PyLong_FromLongLong(offset);
}

static PyObject *schema_update_record(PyObject *self, PyObject *args)
{
    const char *schema_name;
    const char *model_name;
    int offset;
    PyObject *py_values;

    if (!PyArg_ParseTuple(args, "ssiO", &schema_name, &model_name, &offset, &py_values))
    {
        return NULL;
    }

    vector<string> values;
    PyObject *iterator = PyObject_GetIter(py_values);
    PyObject *item;

    while ((item = PyIter_Next(iterator)))
    {
        const char *str = PyUnicode_AsUTF8(item);
        values.push_back(string(str));
        Py_DECREF(item);
    }

    Py_DECREF(iterator);

    RecordManager::update_record(schema_name, model_name, offset, values);
    Py_RETURN_NONE;
}

static PyObject *schema_get_record(PyObject *self, PyObject *args)
{
    const char *schema_name;
    const char *model_name;
    int id;
    PyObject *py_field_lengths;

    if (!PyArg_ParseTuple(args, "ssiO", &schema_name, &model_name, &id, &py_field_lengths))
    {
        return NULL;
    }

    vector<int> field_lengths;
    PyObject *iterator = PyObject_GetIter(py_field_lengths);
    PyObject *item;

    while ((item = PyIter_Next(iterator)))
    {
        field_lengths.push_back(PyLong_AsLong(item));
        Py_DECREF(item);
    }

    Py_DECREF(iterator);

    string file_name = get_file_name(schema_name, string(model_name));
    ifstream file(file_name, ios::binary);

    if (!file.is_open())
    {
        PyErr_SetString(PyExc_RuntimeError, "Failed to open binary file");
        return NULL;
    }

    int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);
    int offset = id * record_size;
    file.seekg(offset);

    vector<string> fields;
    char buffer[101];

    for (const auto &length : field_lengths)
    {
        file.read(buffer, length);
        buffer[length] = '\0';
        string field(buffer);
        field.erase(0, field.find_first_not_of(" "));
        field.erase(field.find_last_not_of(" ") + 1);
        fields.push_back(field);
    }

    file.close();

    PyObject *py_fields = PyList_New(field_lengths.size());

    for (long unsigned int i = 0; i < field_lengths.size(); i++)
    {
        PyList_SET_ITEM(py_fields, i, PyUnicode_FromString(fields[i].c_str()));
    }

    return py_fields;
}

static PyMethodDef graphenix_methods[] = {
    {"create_schema", create_schema, METH_VARARGS, "Create a schema with the given name"},
    {"delete_schema", delete_schema, METH_VARARGS, "Delete the schema with the given name"},
    {"schema_exists", schema_exists, METH_VARARGS, "Check if the schema with the given name exists"},
    {"schema_add_record", schema_add_record, METH_VARARGS, "Adds a record"},
    {"schema_update_record", schema_update_record, METH_VARARGS, "Updates a record"},
    {"schema_get_record", schema_get_record, METH_VARARGS, "Get a record for a specific model by id"},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef graphenix_module = {
    PyModuleDef_HEAD_INIT,
    "graphenix_engine",                              /* name of module */
    "This module provides schema operations in C++", /* module documentation, may be NULL */
    -1,                                              /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    graphenix_methods};

PyMODINIT_FUNC PyInit_graphenix_engine(void)
{
    return PyModule_Create(&graphenix_module);
}
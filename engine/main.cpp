#include <Python.h>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <numeric>

using namespace std;

static PyObject *create_schema(PyObject *self, PyObject *args) {
    const char *schema_name;
    PyObject *py_model_names;

    if (!PyArg_ParseTuple(args, "sO", &schema_name, &py_model_names)) {
        return NULL;
    }

    if (!PyList_Check(py_model_names)) {
        PyErr_SetString(PyExc_TypeError, "Model names must be a list");
        return NULL;
    }

    int status = mkdir(schema_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create schema folder");
        return NULL;
    }

    int num_models = PyList_Size(py_model_names);

    for (int i = 0; i < num_models; i++) {
        PyObject *py_model_name = PyList_GetItem(py_model_names, i);
        const char *model_name = PyUnicode_AsUTF8(py_model_name);

        string filename = schema_name;
        filename += "/";
        filename += model_name;
        filename += ".bin";

        ofstream outfile(filename, ios::out | ios::binary);

        if (!outfile) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to create binary file");
            return NULL;
        }

        outfile.close();
    }

    Py_RETURN_NONE;
}

static PyObject *delete_schema(PyObject *self, PyObject *args) {
    const char *schema_name;

    if (!PyArg_ParseTuple(args, "s", &schema_name)) {
        return NULL;
    }

    int status = rmdir(schema_name);
    if (status == -1) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to delete schema folder");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *schema_exists(PyObject *self, PyObject *args) {
    const char *schema_name;

    if (!PyArg_ParseTuple(args, "s", &schema_name)) {
        return NULL;
    }

    // Check if the schema directory exists
    struct stat sb;
    if (stat(schema_name, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

static PyObject *schema_add_record(PyObject *self, PyObject *args) {
    const char *schema_name;
    const char *model_name;
    PyObject *py_values;

    if (!PyArg_ParseTuple(args, "ssO", &schema_name, &model_name, &py_values)) {
        return NULL;
    }

    // Convert Python list to C++ vector of strings
    vector<string> values;
    PyObject *iterator = PyObject_GetIter(py_values);
    PyObject *item;

    while ((item = PyIter_Next(iterator))) {
        const char *str = PyUnicode_AsUTF8(item);
        values.push_back(string(str));
        Py_DECREF(item);
    }

    Py_DECREF(iterator);

    // Open binary file
    string file_name = schema_name;
    file_name += "/" + string(model_name) + ".bin";
    ofstream file(file_name, ios::binary | ios::app);

    if (!file.is_open()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to open binary file");
        return NULL;
    }

    // Write values to binary file
    int num_values = values.size();
    int record_size = num_values * 100;
    char *record = new char[record_size];
    int offset = 0;

    for (const auto &value : values) {
        if (value.length() > 100) {
            PyErr_SetString(PyExc_RuntimeError, "String length exceeds 100 characters");
            return NULL;
        }

        // Copy value to record buffer at fixed offset
        memset(record + offset, ' ', 100);
        memcpy(record + offset, value.c_str(), value.length());

        offset += 100;
    }

    file.write(record, record_size);
    file.close();
    delete[] record;

    Py_RETURN_NONE;
}

static PyObject *schema_get_record(PyObject *self, PyObject *args) {
    const char *schema_name;
    const char *model_name;
    int id;
    PyObject *py_field_lengths;

    if (!PyArg_ParseTuple(args, "ssiO", &schema_name, &model_name, &id, &py_field_lengths)) {
        return NULL;
    }

    // Convert Python list to C++ vector of integers
    vector<int> field_lengths;
    PyObject *iterator = PyObject_GetIter(py_field_lengths);
    PyObject *item;

    while ((item = PyIter_Next(iterator))) {
        field_lengths.push_back(PyLong_AsLong(item));
        Py_DECREF(item);
    }

    Py_DECREF(iterator);

    // Open binary file and seek to the record's position
    string file_name = schema_name;
    file_name += "/" + string(model_name) + ".bin";
    ifstream file(file_name, ios::binary);

    if (!file.is_open()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to open binary file");
        return NULL;
    }

    int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);
    int offset = id * record_size;
    file.seekg(offset);

    // Read the fields from the binary file and convert to Python strings
    vector<string> fields;
    char buffer[101];

    for (const auto &length : field_lengths) {
        file.read(buffer, length);
        buffer[length] = '\0';
        string field(buffer);
        // Trim the string
        field.erase(0, field.find_first_not_of(" "));
        field.erase(field.find_last_not_of(" ") + 1);
        fields.push_back(field);
    }

    file.close();

    // Convert C++ vector of strings to Python list of strings
    PyObject *py_fields = PyList_New(field_lengths.size());

    for (long unsigned int i = 0; i < field_lengths.size(); i++) {
        PyList_SET_ITEM(py_fields, i, PyUnicode_FromString(fields[i].c_str()));
    }

    return py_fields;
}

static PyMethodDef graphenix_methods[] = {
    {"create_schema", create_schema, METH_VARARGS, "Create a schema with the given name"},
    {"delete_schema", delete_schema, METH_VARARGS, "Delete the schema with the given name"},
    {"schema_exists", schema_exists, METH_VARARGS, "Check if the schema with the given name exists"},
    {"schema_add_record", schema_add_record, METH_VARARGS, "Add a record to the model with the given name in the schema with the given name"},
    {"schema_get_record", schema_get_record, METH_VARARGS, "Get a record for a specific model by id"},
    {NULL, NULL, 0, NULL}  /* Sentinel */
};

static struct PyModuleDef graphenix_module = {
    PyModuleDef_HEAD_INIT,
    "graphenix_engine",   /* name of module */
    "This module provides schema operations in C++",  /* module documentation, may be NULL */
    -1, /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    graphenix_methods
};

PyMODINIT_FUNC PyInit_graphenix_engine(void) {
    return PyModule_Create(&graphenix_module);
}
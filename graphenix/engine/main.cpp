#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "parser.hpp"
#include "managers/managers.h"
#include "managers/schema_manager.cpp"
#include "managers/record_manager.cpp"
#include "managers/query_manager.cpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

long heartbeat()
{
    return 12l;
}

void create_schema(const std::string &schema_name, const std::vector<std::string> &model_names,
                   const bool delete_old)
{
    SchemaManager::create_schema(schema_name, model_names, delete_old);
}

void delete_schema(const std::string &schema_name)
{
    SchemaManager::delete_schema(schema_name);
}

bool schema_exists(const std::string &schema_name)
{
    return SchemaManager::schema_exists(schema_name);
}

long long schema_add_record(const std::string &db_name, const std::string &table_name,
                            const py::list &py_values, const std::vector<int> &field_lengths,
                            const int64_t record_size, const std::vector<int> &field_types)
{
    vector<char *> parsed_values = PARSE_RECORD(py_values, field_types, field_lengths);
    int64_t return_val = RecordManager::create_record(db_name, table_name, parsed_values, field_lengths, record_size, field_types);
    DEALLOCATE_RECORD(parsed_values);
    return return_val;
}

void schema_update_record(const std::string &schema_name, const std::string &model_name,
                          const int64_t id, const py::list &py_values,
                          const std::vector<int> &field_lengths, const int64_t record_size,
                          const std::vector<int> &field_types)
{
    vector<char *> parsed_values = PARSE_RECORD(py_values, field_types, field_lengths);
    RecordManager::update_record(schema_name, model_name, id, parsed_values, field_lengths, record_size, field_types);
    DEALLOCATE_RECORD(parsed_values);
}

py::list schema_get_record(const std::string &schema_name, const std::string &model_name,
                           const int64_t id, const std::vector<int> &field_lengths,
                           const std::vector<int> &field_types, const int record_size)
{
    vector<char *> parsed_values = RecordManager::get_record(schema_name, model_name, id, field_lengths, field_types, record_size);
    vector<py::object> record = PYTHNOIZE_RECORD(parsed_values, field_types, field_lengths);
    py::list py_record = py::cast(record);
    DEALLOCATE_RECORD(parsed_values);
    return py_record;
}

void schema_delete_record(const std::string &schema_name, const std::string &model_name,
                          const int64_t id, const int64_t record_size)
{
    RecordManager::delete_record(schema_name, model_name, id, record_size);
}

PYBIND11_MODULE(graphenix_engine2, m)
{
    m.def("heartbeat", &heartbeat, "Validate library is installed");
    m.def("create_schema", &create_schema, "Create a schema with the given name");
    m.def("delete_schema", &delete_schema, "Delete the schema with the given name");
    m.def("schema_exists", &schema_exists, "Check if the schema with the given name exists");
    m.def("schema_add_record", &schema_add_record, "Add a record to the given table");
    m.def("schema_update_record", &schema_update_record, "Update a record in the given table");
    m.def("schema_get_record", &schema_get_record, "Get a record from the given table");
    m.def("schema_delete_record", &schema_delete_record, "Delete a record from a given table");

    m.def("execute_query", &QueryManager::execute_query, "Executes query and retrieves the desired rows");
    m.def("build_record", &QueryManager::build_record, "Builds a record from raw bytes");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
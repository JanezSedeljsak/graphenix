#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "managers/managers.h"
#include "managers/schema_manager.cpp"
#include "managers/record_manager.cpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

long heartbeat()
{
    return 12l;
}

void create_schema(const std::string& schema_name, const std::vector<std::string>& model_names, bool delete_old)
{
    SchemaManager::create_schema(schema_name, model_names, delete_old);
}

void delete_schema(const std::string& schema_name)
{
    SchemaManager::delete_schema(schema_name);
}

bool schema_exists(const std::string& schema_name)
{
    return SchemaManager::schema_exists(schema_name);
}

long long schema_add_record(const std::string& db_name, const std::string& table_name, const std::vector<std::string>& values, const std::vector<int>& field_lengths)
{
    return RecordManager::create_record(db_name, table_name, values, field_lengths);
}

void schema_update_record(const std::string& schema_name, const std::string& model_name, long offset, const std::vector<std::string>& values, const std::vector<int>& field_lengths)
{
    RecordManager::update_record(schema_name, model_name, offset, values, field_lengths);
}

py::list schema_get_record(const std::string& schema_name, const std::string& model_name, int id, const std::vector<int>& field_lengths)
{
    std::vector<std::string> fields = RecordManager::get_record(schema_name, model_name, id, field_lengths);
    return py::cast(fields);
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

    #ifdef VERSION_INFO
        m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
    #else
        m.attr("__version__") = "dev";
    #endif
}
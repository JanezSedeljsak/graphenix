#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <tuple>
#include "parser.hpp"
#include "managers/managers.h"
#include "managers/schema_manager.cpp"
#include "managers/record_manager.cpp"
#include "managers/query_manager.cpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int64_t heartbeat()
{
    return 12l;
}

int64_t model_add_record(const model_def& mdef, const py::list &py_values)
{
    vector<char *> parsed_values = PARSE_RECORD(mdef, py_values);
    int64_t record_id = RecordManager::create_record(mdef, parsed_values);
    // todo insert into all the bptrees
    DEALLOCATE_RECORD(parsed_values);
    return record_id;
}

void model_update_record(const model_def& mdef, const py::list &py_values, const int64_t id)
{
    vector<char *> parsed_values = PARSE_RECORD(mdef, py_values);
    const auto [old_rec, new_rec] = RecordManager::update_record(mdef, parsed_values, id);
    // todo update the records in the bptrees
    DEALLOCATE_RECORD(parsed_values);
}

py::list model_get_record(const model_def& mdef, const int64_t id)
{
    vector<char *> parsed_values = RecordManager::get_record(mdef, id);
    vector<py::object> record = PYTHNOIZE_RECORD(mdef, parsed_values);
    py::list py_record = py::cast(record);
    DEALLOCATE_RECORD(parsed_values);
    return py_record;
}

PYBIND11_MODULE(graphenix_engine2, m)
{
    m.def("heartbeat", &heartbeat, "Validate library is installed");
    
    m.def("create_schema", &SchemaManager::create_schema, "Create a schema with the given name");
    m.def("delete_schema", &SchemaManager::delete_schema, "Delete the schema with the given name");
    m.def("schema_exists", &SchemaManager::schema_exists, "Check if the schema with the given name exists");
    
    m.def("model_add_record", &model_add_record, "Add a record to the given table");
    m.def("model_update_record", &model_update_record, "Update a record in the given table");
    m.def("model_get_record", &model_get_record, "Get a record from the given table");
    m.def("model_delete_record", &RecordManager::delete_record, "Delete a record from a given table");

    m.def("execute_query", &QueryManager::execute_query, "Executes query and retrieves the desired rows");
    m.def("build_record", &QueryManager::build_record, "Builds a record from raw bytes");

    py::class_<model_def>(m, "model_def")
        .def(py::init<>())
        .def_readwrite("db_name", &model_def::db_name)
        .def_readwrite("model_name", &model_def::model_name)
        .def_readwrite("field_sizes", &model_def::field_sizes)
        .def_readwrite("field_types", &model_def::field_types)
        .def_readwrite("field_offsets", &model_def::field_offsets)
        .def_readwrite("field_indexes", &model_def::field_indexes)
        .def_readwrite("field_names", &model_def::field_names)
        .def_readwrite("record_size", &model_def::record_size);

    py::class_<query_object>(m, "query_object")
        .def(py::init<>())
        .def_readwrite("mdef", &query_object::mdef)
        .def_readwrite("limit", &query_object::limit)
        .def_readwrite("offset", &query_object::offset)
        .def_readwrite("field_indexes", &query_object::field_indexes)
        .def_readwrite("order_asc", &query_object::order_asc)
        .def_readwrite("filter_root", &query_object::filter_root);

    py::class_<cond_object>(m, "cond_object")
        .def(py::init<>())
        .def_readwrite("field_name", &cond_object::field_name)
        .def_readwrite("operation_index", &cond_object::operation_index)
        .def_readwrite("value", &cond_object::value);

    py::class_<cond_node>(m, "cond_node")
        .def(py::init<>())
        .def_readwrite("conditions", &cond_node::conditions)
        .def_readwrite("children", &cond_node::children)
        .def_readwrite("is_and", &cond_node::is_and);

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
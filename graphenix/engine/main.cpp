#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <tuple>
#include "parser.hpp"
#include "util.hpp"
#include "managers/managers.h"
#include "managers/schema_manager.cpp"
#include "managers/record_manager.cpp"
#include "managers/query_manager.cpp"
#include "bptree/bptreeindex.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

int64_t heartbeat()
{
    return 12l;
}

int64_t model_add_record(const model_def &mdef, const py::list &py_values)
{
    string file_name = get_file_name(mdef.db_name, mdef.model_name);
    string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);

    fstream file(file_name, ios::binary | ios::in | ios::out);
    fstream ix_file(ix_file_name, ios::binary | ios::in | ios::out);
    tuple<int64_t, int64_t> ixdata = RecordManager::make_pk(file, ix_file);
    const auto [offset, record_id] = ixdata;

    if (!file.is_open())
        throw runtime_error("Failed to open the data file!");

    vector<char *> parsed_values = PARSE_RECORD(mdef, py_values, DO_INSERT, record_id);
    RecordManager::create_record(file, ix_file, mdef, parsed_values, ixdata);

    file.close();
    ix_file.close();
    DEALLOCATE_RECORD(parsed_values);
    return record_id;
}

void model_update_record(const model_def &mdef, const py::list &py_values, const int64_t id)
{
    vector<char *> parsed_values = PARSE_RECORD(mdef, py_values, SKIP, id);
    const auto [old_rec, new_rec] = RecordManager::update_record(mdef, parsed_values, id);
    query_object::compare_and_update_bptree(mdef, old_rec, new_rec, id, DO_UPDATE);
    DEALLOCATE_RECORD(parsed_values);
}

py::list model_get_record(const model_def &mdef, const int64_t id)
{
    vector<char *> parsed_values = RecordManager::get_record(mdef, id);
    vector<py::object> record = PYTHNOIZE_RECORD(mdef, parsed_values);
    py::list py_record = py::cast(record);
    DEALLOCATE_RECORD(parsed_values);
    return py_record;
}

void print_index(string s, string m, string f)
{
    BPTreeIndex<int64_t> bpt(s, m, f);
    bpt.read();
    bpt.load_full_tree();
    bpt.print_index(true);
}

PYBIND11_MODULE(graphenix_engine2, m)
{
    m.def("heartbeat", &heartbeat, "Validate library is installed");
    m.def("print_index", &print_index, "Print index");

    m.def("create_schema", &SchemaManager::create_schema, "Create a schema with the given name");
    m.def("delete_schema", &SchemaManager::delete_schema, "Delete the schema with the given name");
    m.def("schema_exists", &SchemaManager::schema_exists, "Check if the schema with the given name exists");

    m.def("model_add_record", &model_add_record, "Add a record to the given table");
    m.def("model_update_record", &model_update_record, "Update a record in the given table");
    m.def("model_get_record", &model_get_record, "Get a record from the given table");
    m.def("model_delete_record", &RecordManager::delete_record, "Delete a record from a given table");

    m.def("execute_query", &QueryManager::execute_query, "Executes query and retrieves the desired rows");
    m.def("execute_agg_query", &QueryManager::execute_agg_query, "Executes aggregation query and retrieves the desired rows");
    m.def("execute_entity_query", &QueryManager::execute_entity_query, "Executes aggregation query and retrieves the desired rows");
    // m.def("build_record", &QueryManager::build_record, "Builds a record from raw bytes");

    py::class_<model_def>(m, "model_def")
        .def(py::init<>())
        .def_readwrite("db_name", &model_def::db_name)
        .def_readwrite("model_name", &model_def::model_name)
        .def_readwrite("field_sizes", &model_def::field_sizes)
        .def_readwrite("field_types", &model_def::field_types)
        .def_readwrite("field_offsets", &model_def::field_offsets)
        .def_readwrite("field_indexes", &model_def::field_indexes)
        .def_readwrite("field_names", &model_def::field_names)
        .def_readwrite("field_date_indexes", &model_def::field_date_indexes)
        .def_readwrite("record_size", &model_def::record_size);

    py::class_<query_object>(m, "query_object")
        .def(py::init<>())
        .def_readwrite("mdef", &query_object::mdef)
        .def_readwrite("limit", &query_object::limit)
        .def_readwrite("offset", &query_object::offset)
        .def_readwrite("field_indexes", &query_object::field_indexes)
        .def_readwrite("order_asc", &query_object::order_asc)
        .def_readwrite("agg_field_index", &query_object::agg_field_index)
        .def_readwrite("agg_vector", &query_object::agg_vector)
        .def_readwrite("filter_root", &query_object::filter_root)
        .def_readwrite("links", &query_object::links)
        .def_readwrite("link_vector", &query_object::link_vector)
        .def_readwrite("is_subquery", &query_object::is_subquery)
        .def_readwrite("has_ix_constraints", &query_object::has_ix_constraints)
        .def_readwrite("picked_index", &query_object::picked_index);

    py::class_<cond_object>(m, "cond_object")
        .def(py::init<>())
        .def_readwrite("field_name", &cond_object::field_name)
        .def_readwrite("operation_index", &cond_object::operation_index)
        .def_readwrite("value", &cond_object::value);

    py::class_<cond_node>(m, "cond_node")
        .def(py::init<>())
        .def_readwrite("btree_conditions", &cond_node::btree_conditions)
        .def_readwrite("conditions", &cond_node::conditions)
        .def_readwrite("children", &cond_node::children)
        .def_readwrite("is_and", &cond_node::is_and);

    py::class_<aggregate_object>(m, "aggregate_object")
        .def(py::init<>())
        .def_readwrite("option", &aggregate_object::option)
        .def_readwrite("field_index", &aggregate_object::field_index);

    py::class_<link_object>(m, "link_object")
        .def(py::init<>())
        .def_readwrite("is_direct_link", &link_object::is_direct_link)
        .def_readwrite("link_field_index", &link_object::link_field_index)
        .def_readwrite("child_link_field_index", &link_object::child_link_field_index)
        .def_readwrite("limit", &link_object::limit)
        .def_readwrite("offset", &link_object::offset);

    py::class_<Record>(m, "Record")
        .def("attr", &Record::attr)
        .def("as_tuple", &Record::as_tuple)
        .def("as_dict", &Record::as_dict)
        .def("as_str", &Record::as_str)
        .def("__getattr__", [](const Record &self, const std::string &name)
             { return self.attr(name); })
        .def("__str__", [](const Record &self)
             { return self.as_str(); })
        .def("__repr__", [](const Record &self)
             { return self.as_str(); });

    py::class_<RecordView>(m, "RecordView")
        .def("attr", &RecordView::attr)
        .def("as_tuple", &RecordView::as_tuple)
        .def("as_dict", &RecordView::as_dict)
        .def("as_str", &RecordView::as_str)
        .def("__getattr__", [](const RecordView &self, const std::string &name)
             { return self.attr(name); })
        .def("__str__", [](const RecordView &self)
             { return self.as_str(); })
        .def("__repr__", [](const RecordView &self)
             { return self.as_str(); });

    py::class_<View>(m, "View")
        .def(py::init<>())
        .def_readonly("records", &View::records)
        .def_readonly("field_names", &View::field_names)
        .def("at", &View::at)
        .def("size", &View::size)
        .def("as_dict", &View::as_dict)
        .def("as_tuple", &View::as_tuple)
        .def("__getitem__", [](const View &self, const int &index)
             { return self.at(index); });

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}
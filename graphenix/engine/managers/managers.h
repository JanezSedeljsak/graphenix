#ifndef MANAGERS_H
#define MANAGERS_H

#include <string>
#include <vector>
#include <map>
#include <vector>

#include "../view.hpp"

using namespace std;

class SchemaManager {
public:
    static void create_schema(const string& db_name, const vector<model_def>& models, bool delete_old);
    static void migrate_schema(const string& db_name);
    static void delete_schema(const string& db_name);
    static bool schema_exists(const string& db_name);
};

class RecordManager {
public:
    static tuple<int64_t, int64_t> make_pk(
        fstream &file,
        fstream &ix_file
    );
    
    static bool create_record(
        fstream &file,
        fstream &ix_file,
        const model_def& mdef,
        const vector<char*> &values,
        tuple<int64_t, int64_t> ixdata
    );

    static tuple<char *, char *> update_record(
        const model_def& mdef,
        const vector<char*> &values,
        const int64_t record_id
    );

    static void delete_record(
        const model_def& mdef, 
        const int64_t record_id
    );

    static vector<char*> get_record(
        const model_def& mdef, 
        const int64_t record_id
    );
};

class QueryManager {
public:
    static View execute_query(const query_object& qobject, const int depth);
    static vector<py::tuple> execute_entity_query(const query_object& qobject);
    static vector<py::tuple> execute_agg_query(const query_object& qobject);
    // static py::dict build_record(const model_def& mdef, const py::bytes raw_record);
};

#endif
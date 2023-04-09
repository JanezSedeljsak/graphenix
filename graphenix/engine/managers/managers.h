#ifndef MANAGERS_H
#define MANAGERS_H

#include <string>
#include <vector>
#include <map>

using namespace std;

class SchemaManager {
public:
    static void create_schema(const string& db_name, const vector<string>& table_names, bool delete_old);
    static void migrate_schema(const string& db_name);
    static void delete_schema(const string& db_name);
    static bool schema_exists(const string& db_name);
};

class RecordManager {
public:
    static int64_t create_record(
        const string& db_name, 
        const string& table_name, 
        const vector<char*> &values,
        const vector<int>& field_lengths,
        const int64_t record_size,
        const vector<int>& field_types
    );

    static void update_record(
        const string& db_name, 
        const string& table_name, 
        const int64_t record_id, 
        const vector<char*> &values,
        const vector<int>& field_lengths,
        const int64_t record_size,
        const vector<int>& field_types
    );

    static void delete_record(
        const string& db_name, 
        const string& table_name, 
        const int64_t record_id,
        const int64_t record_size
    );

    static vector<char*> get_record(
        const string &db_name,
        const string &table_name,
        const int64_t record_id,
        const vector<int>& field_lengths,
        const vector<int>& field_types,
        const int record_size
    );
};

class QueryManager {
public:
    static vector<py::bytes> execute_query(
        const string &db_name,
        const string &table_name,
        const int record_size
    );

    static py::dict build_record(
        const vector<int>& field_lengths,
        const vector<int>& field_types,
        const vector<string>& field_names,
        const int record_size,
        const py::bytes raw_record
    );
};

class TypeManager {
public:
    // TODO
};

class ErrorManager {
public:
    // TODO (this will handle errors in raw CPP so the errors can then be generated for python)
};

#endif
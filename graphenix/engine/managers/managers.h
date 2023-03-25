#ifndef MANAGERS_H
#define MANAGERS_H

#include <string>
#include <vector>

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
        const bool is_lazy_delete,
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
    // TODO
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
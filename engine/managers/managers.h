#ifndef MANAGERS_H
#define MANAGERS_H

#include <string>
#include <vector>

using namespace std;

class SchemaManager {
public:
    static void create_schema(const string& db_name, const vector<string>& table_names);
    static void migrate_schema(const string& db_name);
    static void delete_schema(const string& db_name);
    static bool schema_exists(const string& db_name);
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
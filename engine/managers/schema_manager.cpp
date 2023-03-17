#include "managers.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include "../util.cpp"

using namespace std;

void SchemaManager::create_schema(const string &db_name, const vector<string> &table_names, bool delete_old)
{
    if (delete_old && SchemaManager::schema_exists(db_name)) 
    {
        SchemaManager::delete_schema(db_name);
    }

    int status = mkdir(db_name.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1)
    {
        throw runtime_error("Failed to create schema folder");
    }

    for (const auto &table_name : table_names)
    {
        string filename = get_file_name(db_name, table_name);
        ofstream outfile(filename, ios::out | ios::binary);
        if (!outfile)
        {
            throw runtime_error("Failed to create binary file for table");
        }

        outfile.close();
    }
}

bool SchemaManager::schema_exists(const string &db_name)
{
    struct stat st;
    return (stat(db_name.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

void SchemaManager::migrate_schema(const string &db_name)
{
    // TODO: Implement the migrate_schema method
}

void SchemaManager::delete_schema(const string &db_name)
{
    filesystem::path dir_path(db_name);
    if (filesystem::exists(dir_path) && filesystem::is_directory(dir_path))
    {
        filesystem::remove_all(dir_path);
    }
}

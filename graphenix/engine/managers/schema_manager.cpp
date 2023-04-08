#include "managers.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <omp.h>
#include "../util.cpp"

using namespace std;

void SchemaManager::create_schema(const string &db_name, const vector<string> &table_names, bool delete_old)
{
    MAKE_GRAPHENIX_DB_DIR();
    if (delete_old && SchemaManager::schema_exists(db_name)) 
    {
        SchemaManager::delete_schema(db_name);
    }

    string db_path = get_db_path(db_name);
    int status = mkdir(db_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1)
    {
        throw runtime_error("Failed to create schema folder");
    }

    #pragma omp parallel for schedule(static) 
    for (const auto &table_name : table_names)
    {
        // create model file
        string filename = get_file_name(db_name, table_name);
        ofstream table_outfile(filename, ios::out | ios::binary);
        if (!table_outfile)
        {
            throw runtime_error("Failed to create binary file for table!");
        }

        table_outfile.close();

        // create index file
        string ix_filename = get_ix_file_name(db_name, table_name);
        ofstream ix_outfile(ix_filename, ios::out | ios::binary);
        if (!ix_outfile)
        {
            throw runtime_error("Failed to create binary file for table index!");
        }

        int64_t deleted_head = -1;
        ix_outfile.write(reinterpret_cast<const char*>(&deleted_head), IX_SIZE);
        ix_outfile.close();
    }
}

bool SchemaManager::schema_exists(const string &db_name)
{
    MAKE_GRAPHENIX_DB_DIR();
    struct stat st;
    string db_path = get_db_path(db_name);
    return (stat(db_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
}

void SchemaManager::migrate_schema(const string &db_name)
{
    // TODO: Implement the migrate_schema method
}

void SchemaManager::delete_schema(const string &db_name)
{
    MAKE_GRAPHENIX_DB_DIR();
    string db_path = get_db_path(db_name);
    filesystem::path dir_path(db_path);
    if (filesystem::exists(dir_path) && filesystem::is_directory(dir_path))
    {
        filesystem::remove_all(dir_path);
    }
}

#ifndef UTIL
#define UTIL

#include <iostream>
#include <string>
#include "parser.hpp"

#define IX_SIZE 8 // size of 8 bytes <==> IX_SIZE
#define PK_IX_HEAD_SIZE IX_SIZE * 2
#define CHUNK_SIZE 10 * 1024 * 1024
#define ID_KEY "_id"

#ifdef _WIN32
#include <direct.h>
#define MAKE_GRAPHENIX_DB_DIR() _mkdir("graphenix_db")
#else
#include <sys/stat.h>
#define MAKE_GRAPHENIX_DB_DIR() mkdir("graphenix_db", 0777)
#endif

using namespace std;

string get_db_path(const string &schema_name)
{
    return "graphenix_db/" + schema_name;
}

string get_file_name(const string &schema_name, const string &model_name)
{
    // eg. school/students.bin;
    return "graphenix_db/" + schema_name + "/" + model_name + ".bin";
}

string get_ix_file_name(const string &schema_name, const string &model_name)
{
    // eg. school/ix_students.bin; <- primary key index
    return "graphenix_db/" + schema_name + "/ix_" + model_name + ".bin";
}

int64_t get_record_offset(const int64_t record_id, fstream &ix_file)
{
    const int64_t position = record_id * IX_SIZE + PK_IX_HEAD_SIZE;
    ix_file.clear();
    ix_file.seekg(0, ios::end);

    const int64_t file_size = ix_file.tellg();
    // cout << "file size: " << file_size << " position " << position << " rec id " << record_id << endl;
    if (position >= file_size)
    {
        int64_t last_id = file_size > static_cast<int64_t>(PK_IX_HEAD_SIZE) ? (file_size / IX_SIZE - 3) : -1;
        string msg = last_id > -1
                         ? message("Record ID (", record_id, ") is out of range!\nLast inserted: ", last_id)
                         : message("No records exist in the table yet");

        throw runtime_error(msg);
    }

    ix_file.seekg(position, ios::beg);
    int64_t ix;
    ix_file.read(reinterpret_cast<char *>(&ix), IX_SIZE);

    return ix;
}

void set_record_inactive(const int64_t record_id, fstream &ix_file)
{
    // cout << "Set record inactive: " << record_id << endl;
    ix_file.seekp(record_id * IX_SIZE + PK_IX_HEAD_SIZE, ios::beg);
    int64_t inactive_status = -1;
    ix_file.write(reinterpret_cast<const char *>(&inactive_status), IX_SIZE);
}

/*
map<int64_t, int64_t> my_map = ;
set<int64_t> my_set;

transform(my_map.begin(), my_map.end(), inserter(my_set, my_set.end()),
    [](const pair<int64_t, int64_t>& p) { return p.first; });
*/

#endif
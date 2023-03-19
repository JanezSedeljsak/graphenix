#ifndef UTIL
#define UTIL

#include <iostream>
#include <string>

#define IX_SZIE sizeof(int64_t)
#define CHUNK_SIZE 10 * 1024 * 1024

using namespace std;

string get_file_name(const string &schema_name, const string &model_name)
{
    // eg. school/students.bin;
    return schema_name + "/" + model_name + ".bin";
}

string get_ix_file_name(const string &schema_name, const string &model_name) 
{
    // eg. school/ix_students.bin; <- primary key index
    return schema_name + "/ix_" + model_name + ".bin";   
}

int64_t get_record_offset(const int64_t record_id, fstream &ix_file)
{
    ix_file.seekg(record_id * IX_SZIE, ios::beg);
    int64_t ix;
    ix_file.read(reinterpret_cast<char*>(&ix), IX_SZIE);

    return ix;
}

void set_record_inactive(const int64_t record_id, fstream &ix_file)
{
    ix_file.seekp(record_id * IX_SZIE, ios::beg);
    int64_t ix = -1;
    ix_file.write(reinterpret_cast<const char*>(&ix), IX_SZIE);
}

#endif
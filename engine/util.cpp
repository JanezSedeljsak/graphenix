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
    // cout << "Set record inactive: " << record_id << endl;
    ix_file.seekp(record_id * IX_SZIE, ios::beg);
    int64_t ix = -1;
    ix_file.write(reinterpret_cast<const char*>(&ix), IX_SZIE);
}

void adjust_indexes(const int64_t start_ix, const int record_size, fstream &ix_file)
{
    ix_file.seekg(0, ios::end);
    int64_t ix_length = ix_file.tellg() / IX_SZIE;

    // cout << "Start index: " << start_ix << " length: " << ix_length << endl;
    for (int i = start_ix; i < ix_length; i++) 
    {
        int64_t new_ix = get_record_offset(i, ix_file) - record_size;
        // cout << "Record offset: " << get_record_offset(i, ix_file) << " New offset: " << new_ix << endl;
        ix_file.seekp(i * IX_SZIE, ios::beg);
        ix_file.write(reinterpret_cast<char*>(&new_ix), IX_SZIE);
    }
}

#endif
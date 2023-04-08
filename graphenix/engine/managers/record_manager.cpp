#include "managers.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <numeric>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include "../util.cpp"

using namespace std;

int64_t RecordManager::create_record(const string &db_name, const string &table_name,
                                     const vector<char *> &values, const vector<int> &field_lengths,
                                     const int64_t record_size, const std::vector<int> &field_types)
{
    string file_name = get_file_name(db_name, table_name);
    string ix_file_name = get_ix_file_name(db_name, table_name);

    ofstream file(file_name, ios::binary | ios::app);
    ofstream ix_file(ix_file_name, ios::binary | ios::app);

    if (!file.is_open())
    {
        return -1;
    }

    int num_values = values.size();

    char *record = new char[record_size + IX_SIZE];
    int64_t offset = file.tellp();

    int field_offset = IX_SIZE;
    int64_t ix = -1;
    memcpy(record, reinterpret_cast<const char*>(&ix), IX_SIZE);

    for (int i = 0; i < num_values; i++)
    {
        int field_size = field_lengths[i];
        memcpy(record + field_offset, values[i], field_size);
        field_offset += field_size;
    }

    file.write(record, record_size + IX_SIZE);
    file.close();
    delete[] record;

    // insert the offset into the binary file (a pointer to the table file)
    int ix_offset = ix_file.tellp();
    ix_file.write(reinterpret_cast<const char *>(&offset), IX_SIZE);
    ix_file.close();

    // return the index of the record in the index file
    return ix_offset / IX_SIZE - 1;
}

void RecordManager::update_record(const string &db_name, const string &table_name,
                                  const int64_t record_id, const vector<char *> &values,
                                  const vector<int> &field_lengths, const int64_t record_size,
                                  const std::vector<int> &field_types)
{
    string file_name = get_file_name(db_name, table_name);
    string ix_file_name = get_ix_file_name(db_name, table_name);

    fstream file(file_name, ios::binary | ios::in | ios::out);
    fstream ix_file(ix_file_name, ios::binary | ios::in);

    if (!file.is_open())
    {
        throw runtime_error("Failed to open binary file");
    }

    int num_values = values.size();

    // don't update deleted_head_index field because it has to stay -1
    char *record = new char[record_size];

    int64_t record_offset = get_record_offset(record_id, ix_file);
    if (record_offset == -1)
    {
        string msg = message("Record with id ", record_id, " has been deleted!");
        throw runtime_error(msg);
    }

    file.seekp(record_offset + IX_SIZE);

    int field_offset = 0;
    for (int i = 0; i < num_values; i++)
    {
        int field_size = field_lengths[i];
        memcpy(record + field_offset, values[i], field_size);
        field_offset += field_size;
    }

    file.write(record, record_size);
    file.close();

    ix_file.close();
    delete[] record;
}

void RecordManager::delete_record(const string &db_name, const string &table_name,
                                  const int64_t record_id, const int64_t record_size)
{
    string ix_file_name = get_ix_file_name(db_name, table_name);
    fstream ix_file(ix_file_name, ios::binary | ios::in | ios::out);

    int64_t record_offset = get_record_offset(record_id, ix_file);
    set_record_inactive(record_id, ix_file);

    string file_name = get_file_name(db_name, table_name);
    fstream file(file_name, ios::binary | ios::in | ios::out);

    file.seekp(0, ios::end);

    file.close();
    ix_file.close();
}

vector<char*> RecordManager::get_record(const string &db_name, const string &table_name,
                                        const int64_t record_id, const vector<int> &field_lengths,
                                        const std::vector<int> &field_types, const int record_size)
{
    string file_name = get_file_name(db_name, table_name);
    string ix_file_name = get_ix_file_name(db_name, table_name);

    fstream file(file_name, ios::binary | ios::in);
    fstream ix_file(ix_file_name, ios::binary | ios::in);

    if (!file.is_open())
    {
        throw runtime_error("Failed to open binary file");
    }

    int64_t record_offset = get_record_offset(record_id, ix_file);
    // cout << "Record offset" << record_offset << endl;
    if (record_offset == -1)
    {
        string msg = message("Record with id ", record_id, " has been deleted!");
        throw runtime_error(msg);
    }

    file.seekg(record_offset + IX_SIZE, ios::beg);
    const size_t fields_count = field_lengths.size();
    char* buffer = new char[record_size];
    file.read(buffer, record_size);

    vector<char*> bin_values(fields_count);
    size_t offset = 0;
    for (size_t i = 0; i < fields_count; i++)
    {
        char *field_buffer = new char[field_lengths[i]];
        memcpy(field_buffer, buffer + offset, field_lengths[i]);
        offset += field_lengths[i];
        bin_values[i] = field_buffer;
    }

    delete[] buffer;

    ix_file.close();
    file.close();
    return bin_values;
}
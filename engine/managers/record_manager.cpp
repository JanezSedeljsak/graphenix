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
                                     const vector<string> &values, const vector<int> &field_lengths,
                                     const int64_t record_size)
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
    // int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);

    char *record = new char[record_size];
    int64_t offset = file.tellp();

    int field_offset = 0;
    for (int i = 0; i < num_values; i++)
    {
        const auto &value = values[i];
        int field_size = field_lengths[i];

        if (static_cast<int>(value.length()) > field_size)
        {
            throw runtime_error("Field exceeds max size!");
        }

        memset(record + field_offset, ' ', field_size);
        memcpy(record + field_offset, value.c_str(), value.length());

        field_offset += field_size;
    }

    file.write(record, record_size);
    file.close();
    delete[] record;

    // insert the offset into the binary file (a pointer to the table file)
    int ix_offset = ix_file.tellp();
    ix_file.write(reinterpret_cast<const char *>(&offset), IX_SZIE);
    ix_file.close();

    // return the index of the record in the index file
    return ix_offset / IX_SZIE;
}

void RecordManager::update_record(const string &db_name, const string &table_name,
                                  const int64_t record_id, const vector<string> &values,
                                  const vector<int> &field_lengths, const int64_t record_size)
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
    // int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);

    char *record = new char[record_size];

    int64_t record_offset = get_record_offset(record_id, ix_file);
    if (record_offset == -1)
    {
        throw runtime_error("Record has been deleted");
    }

    file.seekp(record_offset);

    int field_offset = 0;
    for (int i = 0; i < num_values; i++)
    {
        const auto &value = values[i];
        int field_size = field_lengths[i];

        if (static_cast<int>(value.length()) > field_size)
        {
            throw runtime_error("Field exceeds max size!");
        }

        memset(record + field_offset, ' ', field_size);
        memcpy(record + field_offset, value.c_str(), value.length());

        field_offset += field_size;
    }

    file.write(record, record_size);
    file.close();

    ix_file.close();
    delete[] record;
}

void RecordManager::delete_record(const string &db_name, const string &table_name,
                                  const int64_t record_id, const bool is_lazy_delete,
                                  const int64_t record_size)
{
    string ix_file_name = get_ix_file_name(db_name, table_name);
    fstream ix_file(ix_file_name, ios::binary | ios::in | ios::out);

    if (is_lazy_delete)
    {
        set_record_inactive(record_id, ix_file);
        ix_file.close();
        return;
    }

    int64_t record_offset = get_record_offset(record_id, ix_file);
    set_record_inactive(record_id, ix_file);

    string file_name = get_file_name(db_name, table_name);
    fstream file(file_name, ios::binary | ios::in | ios::out);

    // int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);

    file.seekp(0, ios::end);
    int64_t bytes_to_move = file.tellp() - (record_offset + record_size);

    if (bytes_to_move > 0)
    {
        char *buffer = new char[CHUNK_SIZE];
        int64_t remaining_bytes = bytes_to_move;
        file.seekp(record_offset + record_size, ios::beg);

        while (remaining_bytes > 0)
        {
            int64_t bytes_to_read = min(remaining_bytes, (int64_t)CHUNK_SIZE);
            file.read(buffer, bytes_to_read);
            file.seekp(record_offset + record_size + bytes_to_move - remaining_bytes, ios::beg);
            file.write(buffer, bytes_to_read);
            remaining_bytes -= bytes_to_read;
        }

        adjust_indexes(record_id + 1, record_size, ix_file);
        delete[] buffer;
    }

    file.close();
    ix_file.close();
    filesystem::resize_file(file_name, record_offset + bytes_to_move);
}

vector<string> RecordManager::get_record(const string &db_name, const string &table_name,
                                         const int64_t record_id, const vector<int> &field_lengths)
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
    if (record_offset == -1)
    {
        throw runtime_error("Record has been deleted");
    }

    file.seekg(record_offset, ios::beg);

    vector<string> fields;
    auto max_element_ptr = max_element(field_lengths.begin(), field_lengths.end());
    char buffer[*max_element_ptr + 1];

    for (const auto &length : field_lengths)
    {
        file.read(buffer, length);
        buffer[length] = '\0';
        string field(buffer);
        field.erase(0, field.find_first_not_of(" "));
        field.erase(field.find_last_not_of(" ") + 1);
        fields.push_back(field);
    }

    ix_file.close();
    file.close();
    return fields;
}
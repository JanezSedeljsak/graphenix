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
#include<algorithm>
#include "../util.cpp"

using namespace std;

int64_t RecordManager::create_record(const string &db_name, const string &table_name, 
                                     const vector<string> &values, const vector<int>& field_lengths)
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
    int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);
    
    char *record = new char[record_size];
    int64_t offset = file.tellp();

    int field_offset = 0;
    for (int i = 0; i < num_values; i++)
    {
        const auto& value = values[i];
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
    ix_file.write(reinterpret_cast<const char*>(&offset), IX_SZIE);
    ix_file.close();

    // return the index of the record in the index file
    return ix_offset / IX_SZIE;
}

void RecordManager::update_record(const string &db_name, const string &table_name, 
                                  const int64_t record_id, const vector<string> &values, 
                                  const vector<int>& field_lengths)
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
    int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);

    char *record = new char[record_size];

    int64_t record_offset = get_record_offset(record_id, ix_file);
    file.seekp(record_offset);

    int field_offset = 0;
    for (int i = 0; i < num_values; i++)
    {
        const auto& value = values[i];
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

void RecordManager::lazy_delete_record(const string &db_name, const string &table_name, const int64_t record_id)
{
}

void RecordManager::delete_record(const string &db_name, const string &table_name, const int64_t record_id)
{
}

vector<string> RecordManager::get_record(const string &db_name, const string &table_name, 
                                         const int64_t record_id, const vector<int>& field_lengths) 
{
    string file_name = get_file_name(db_name, table_name);
    string ix_file_name = get_ix_file_name(db_name, table_name);

    fstream file(file_name, ios::binary | ios::in);
    fstream ix_file(ix_file_name, ios::binary | ios::in);

    if (!file.is_open()) {
        throw runtime_error("Failed to open binary file");
    }

    int64_t record_offset = get_record_offset(record_id, ix_file);
    file.seekg(record_offset, ios::beg);

    vector<string> fields;
    auto max_element_ptr = max_element(field_lengths.begin(), field_lengths.end());
    char buffer[*max_element_ptr + 1];

    for (const auto &length : field_lengths) {
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
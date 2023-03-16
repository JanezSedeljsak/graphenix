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

int64_t RecordManager::create_record(const string &db_name, const string &table_name, const vector<string> &values, const vector<int>& field_lengths)
{
    string file_name = get_file_name(db_name, table_name);
    ofstream file(file_name, ios::binary | ios::app);

    if (!file.is_open())
    {
        return -1;
    }

    int num_values = values.size();
    int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);
    
    char *record = new char[record_size];
    int offset = file.tellp();

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

    // reutrn the offset as index
    // eg. if the offset would be 800 and the record_size is 200 the returned value will be 4
    return offset / record_size;
}

void RecordManager::update_record(const string &db_name, const string &table_name, const int64_t record_id, const vector<string> &values, const vector<int>& field_lengths)
{
    string file_name = get_file_name(db_name, table_name);
    fstream file(file_name, ios::binary | ios::in | ios::out);

    if (!file.is_open())
    {
        throw runtime_error("Failed to open binary file");
    }

    int num_values = values.size();
    int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);

    char *record = new char[record_size];
    file.seekp(record_id * record_size);

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
}

void RecordManager::lazy_delete_record(const string &db_name, const string &table_name, const int64_t record_id)
{
}

void RecordManager::delete_record(const string &db_name, const string &table_name, const int64_t record_id)
{
}

vector<string> RecordManager::get_record(const string &db_name, const string &table_name, const int64_t record_id, const vector<int>& field_lengths) 
{
    string file_name = get_file_name(db_name, table_name);
    fstream file(file_name, ios::binary | ios::in);

    if (!file.is_open()) {
        throw runtime_error("Failed to open binary file");
    }

    int record_size = accumulate(field_lengths.begin(), field_lengths.end(), 0);
    int offset = record_id * record_size;

    file.seekg(offset);
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

    file.close();
    return fields;
}
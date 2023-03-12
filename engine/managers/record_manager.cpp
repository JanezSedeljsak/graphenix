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
#include "../util.cpp"

using namespace std;

int64_t RecordManager::create_record(const string &db_name, const string &table_name, const vector<string> &values)
{
    string file_name = get_file_name(db_name, table_name);
    ofstream file(file_name, ios::binary | ios::app);

    if (!file.is_open())
    {
        return -1;
    }

    int num_values = values.size();
    int record_size = num_values * 100;
    char *record = new char[record_size];
    int offset = file.tellp();

    for (const auto &value : values)
    {
        if (value.length() > 100)
        {
            return -1;
        }

        memset(record, ' ', 100);
        memcpy(record, value.c_str(), value.length());

        file.write(record, 100);
    }

    file.close();
    delete[] record;

    // reutrn the offset as index
    // eg. if the offset would be 800 and the record_size is 200 the returned value will be 4
    return offset / record_size;
}

void RecordManager::update_record(const string &db_name, const string &table_name, const int64_t record_id, const vector<string> &values)
{
    string file_name = get_file_name(db_name, table_name);
    fstream file(file_name, ios::binary | ios::in | ios::out);

    if (!file.is_open())
    {
        throw runtime_error("Failed to open binary file");
    }

    int record_size = values.size() * 100;
    char *record = new char[record_size];
    int offset_in_record = 0;

    file.seekp(record_id * record_size);

    for (const auto &value : values)
    {
        if (value.length() > 100)
        {
            throw runtime_error("String length exceeds 100 characters");
        }

        memset(record + offset_in_record, ' ', 100);
        memcpy(record + offset_in_record, value.c_str(), value.length());

        offset_in_record += 100;
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

vector<string> RecordManager::get_record(const string &db_name, const string &table_name, const int64_t record_id)
{
    vector<string> empty;
    return empty;
}
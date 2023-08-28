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
#include <tuple>

#include "managers.h"
#include "../util.hpp"

using namespace std;

tuple<int64_t, int64_t> RecordManager::make_pk(fstream &file, fstream &ix_file)
{
    int64_t ix_offset, offset, ix;
    int64_t head_ptr;
    ix_file.seekg(0, ios::beg);
    ix_file.read(reinterpret_cast<char *>(&head_ptr), IX_SIZE);

    if (head_ptr == -1)
    {
        // insert the offset into the binary file (a pointer to the table file)
        ix_file.seekg(0, ios::end);
        ix_offset = ix_file.tellg();
        file.seekg(0, ios::end);
        offset = file.tellg();
        ix_file.write(reinterpret_cast<char *>(&offset), IX_SIZE);
    }
    else
    {
        offset = head_ptr;
        file.seekg(offset, ios::beg);
        file.read(reinterpret_cast<char *>(&ix), IX_SIZE); // read next ptr (this will be the new head)
        ix_file.seekg(0, ios::beg);
        ix_file.write(reinterpret_cast<char *>(&ix), IX_SIZE);
        if (ix == -1)
        {
            // if head is deleted so needs to be the tail...
            ix_file.write(reinterpret_cast<char *>(&ix), IX_SIZE);
        }

        ix_file.seekg(0, ios::end);
        ix_offset = ix_file.tellg();
        ix_file.write(reinterpret_cast<char *>(&offset), IX_SIZE);
    }

    return make_tuple(offset, (ix_offset - PK_IX_HEAD_SIZE) / IX_SIZE);
}

bool RecordManager::create_record(fstream &file, fstream &ix_file,
                                  const model_def &mdef, const vector<char *> &values,
                                  tuple<int64_t, int64_t> ixdata)
{
    const auto [offset, record_id] = ixdata;
    int num_values = values.size();
    char *record = new char[mdef.record_size + IX_SIZE];
    int64_t ix = -1;
    memcpy(record, reinterpret_cast<const char *>(&ix), IX_SIZE);
    int field_offset = IX_SIZE;

    for (int i = 0; i < num_values; i++)
    {
        // skip virtual links can only be created through queries
        if (mdef.field_types[i] == VIRTUAL_LINK)
            continue;

        int field_size = mdef.field_sizes[i];
        memcpy(record + field_offset, values[i], field_size);
        field_offset += field_size;
    }

    file.seekg(offset, ios::beg);
    file.write(record, mdef.record_size + IX_SIZE);
    delete[] record;

    return true;
}

// return record_id, old_record and new_record
tuple<char *, char *>
RecordManager::update_record(const model_def &mdef, const vector<char *> &values, const int64_t record_id)
{
    string file_name = get_file_name(mdef.db_name, mdef.model_name);
    string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);

    fstream file(file_name, ios::binary | ios::in | ios::out);
    fstream ix_file(ix_file_name, ios::binary | ios::in);
    int64_t record_offset = get_record_offset(record_id, ix_file);
    if (record_offset == -1)
    {
        string msg = message("Record with id ", record_id, " has been deleted!");
        throw runtime_error(msg);
    }

    char *old_buffer = new char[mdef.record_size];
    char *record = new char[mdef.record_size];
    bool has_indexed_fields = std::any_of(mdef.field_indexes.begin(), mdef.field_indexes.end(),
                                          [](bool value)
                                          { return value; });

    if (has_indexed_fields)
    {
        file.seekg(record_offset + IX_SIZE, ios::beg);
        file.read(old_buffer, mdef.record_size);
    }

    if (!file.is_open())
    {
        throw runtime_error("Failed to open binary file");
    }

    // don't update deleted_head_index field because it has to stay -1
    file.seekp(record_offset + IX_SIZE);

    int field_offset = 0;
    for (size_t i = 0; i < values.size(); i++)
    {
        // skip virtual links can only be created through queries
        if (mdef.field_types[i] == VIRTUAL_LINK)
            continue;

        int field_size = mdef.field_sizes[i];
        memcpy(record + field_offset, values[i], field_size);
        field_offset += field_size;
    }

    file.write(record, mdef.record_size);
    file.close();

    ix_file.close();
    return make_tuple(old_buffer, record);
}

void RecordManager::delete_record(const model_def &mdef, const int64_t record_id)
{
    string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);
    fstream ix_file(ix_file_name, ios::binary | ios::in | ios::out);

    int64_t record_offset = get_record_offset(record_id, ix_file);
    set_record_inactive(record_id, ix_file);

    string file_name = get_file_name(mdef.db_name, mdef.model_name);
    fstream file(file_name, ios::binary | ios::in | ios::out);

    bool has_indexed_fields = std::any_of(mdef.field_indexes.begin(), mdef.field_indexes.end(),
                                          [](bool value)
                                          { return value; });

    if (has_indexed_fields)
    {
        file.seekg(record_offset + IX_SIZE, ios::beg);
        char *buffer = new char[mdef.record_size];
        file.read(buffer, mdef.record_size);
        query_object::compare_and_update_bptree(mdef, buffer, buffer, record_id, DO_DELETE);
        delete[] buffer;
    }

    // read head offset
    ix_file.seekg(0, ios::beg);
    int64_t ix;
    ix_file.read(reinterpret_cast<char *>(&ix), IX_SIZE);
    char *rec_offset_ptr = new char[IX_SIZE];
    memcpy(rec_offset_ptr, reinterpret_cast<const char *>(&record_offset), IX_SIZE);

    if (ix == -1)
    {
        // if no deletd records yet => set head and tail to deleted offset
        ix_file.seekg(0, ios::beg);
        ix_file.write(rec_offset_ptr, IX_SIZE);
        ix_file.write(rec_offset_ptr, IX_SIZE);
    }
    else
    {
        // read tail offset
        ix_file.seekg(IX_SIZE, ios::beg);
        ix_file.read(reinterpret_cast<char *>(&ix), IX_SIZE);

        file.seekg(ix, ios::beg);
        file.write(rec_offset_ptr, IX_SIZE);
    }

    delete[] rec_offset_ptr;
    file.close();
    ix_file.close();
}

vector<char *> RecordManager::get_record(const model_def &mdef, const int64_t record_id)
{
    string file_name = get_file_name(mdef.db_name, mdef.model_name);
    string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);

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
    const size_t fields_count = mdef.field_sizes.size();
    char *buffer = new char[mdef.record_size];
    file.read(buffer, mdef.record_size);

    vector<char *> bin_values(fields_count);
    size_t offset = 0;
    for (size_t i = 0; i < fields_count; i++)
    {
        // skip virtual links can only be created through queries
        if (mdef.field_types[i] == VIRTUAL_LINK)
            continue;

        char *field_buffer = new char[mdef.field_sizes[i]];
        memcpy(field_buffer, buffer + offset, mdef.field_sizes[i]);
        offset += mdef.field_sizes[i];
        bin_values[i] = field_buffer;
    }

    delete[] buffer;

    ix_file.close();
    file.close();
    return bin_values;
}
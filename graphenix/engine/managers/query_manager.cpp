#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <numeric>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <omp.h>

#include "managers.h"
#include "../util.cpp"
#include "../parser.hpp"

inline std::map<int64_t, int64_t> ix_read_all(std::fstream &ix_file)
{
    ix_file.seekg(0, std::ios::end);
    const int64_t file_size = ix_file.tellg();

    std::map<int64_t, int64_t> ix2offset;
    std::vector<char> buffer(CHUNK_SIZE);

    int64_t offset = PK_IX_HEAD_SIZE;
    ix_file.seekg(PK_IX_HEAD_SIZE, std::ios::beg);

    while (offset < file_size)
    {
        const int64_t current_chunk_size = std::min(file_size - offset, static_cast<int64_t>(CHUNK_SIZE));
        ix_file.read(buffer.data(), current_chunk_size);

        for (int64_t i = 0; i < current_chunk_size; i += IX_SIZE)
        {
            const int64_t rec_offset = *reinterpret_cast<int64_t *>(&buffer[i]);
            if (rec_offset >= 0)
            {
                int64_t rec_ix = (i + offset) / IX_SIZE - 2;
                ix2offset[rec_ix] = rec_offset;
            }
        }

        offset += current_chunk_size;
    }

    return ix2offset;
}

std::vector<py::bytes> QueryManager::execute_query(const model_def& mdef)
{
    std::string file_name = get_file_name(mdef.db_name, mdef.table_name);
    std::string ix_file_name = get_ix_file_name(mdef.db_name, mdef.table_name);

    std::fstream file(file_name, std::ios::binary | std::ios::in | std::ios::out);
    std::fstream ix_file(ix_file_name, std::ios::binary | std::ios::in | std::ios::out);

    std::map<int64_t, int64_t> ix2offset = ix_read_all(ix_file);
    int64_t map_size = static_cast<int64_t>(ix2offset.size());
    std::vector<py::bytes> rows(map_size);
    char* buffer = new char[mdef.record_size + IX_SIZE];

    int idx = 0;
    for (const auto& [ix, record_offset] : ix2offset)
    {
        file.seekg(record_offset + IX_SIZE, ios::beg);
        file.read(buffer, mdef.record_size);
        memcpy(buffer + mdef.record_size, reinterpret_cast<const char *>(&ix), IX_SIZE);
        rows[idx++] = py::bytes(buffer, mdef.record_size + IX_SIZE);
    }

    delete[] buffer;
    ix_file.close();
    file.close();
    return rows;
}

py::dict QueryManager::build_record(const model_def& mdef, const py::bytes raw_record)
{
    char* buffer = new char[mdef.record_size + IX_SIZE];
    char* raw_record_data;
    Py_ssize_t raw_rec_size;
    PyBytes_AsStringAndSize(raw_record.ptr(), &raw_record_data, &raw_rec_size);
    memcpy(buffer, raw_record_data, mdef.record_size + IX_SIZE);

    const int64_t fields_count = mdef.field_sizes.size();
    vector<char*> bin_values(fields_count);
    int64_t offset = 0;

    for (int64_t i = 0; i < fields_count; i++)
    {
        char *field_buffer = new char[mdef.field_sizes[i]];
        memcpy(field_buffer, buffer + offset, mdef.field_sizes[i]);
        offset += mdef.field_sizes[i];
        bin_values[i] = field_buffer;
    }

    std::vector<py::object> record_vector = PYTHNOIZE_RECORD(mdef, bin_values);
    std::unordered_map<std::string, py::object> record;
    for (int64_t i = 0; i < fields_count; i++) 
    {
        record[mdef.field_names[i]] = record_vector[i];
    }

    int64_t ix;
    memcpy(&ix, buffer + mdef.record_size, IX_SIZE);
    record[ID_KEY] = py::cast(ix);

    delete[] buffer;
    return py::cast(record);
}

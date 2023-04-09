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

namespace py = pybind11;

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

std::vector<py::bytes> QueryManager::execute_query(const std::string& db_name, const std::string& table_name, 
                                                   const int record_size)
{
    std::string file_name = get_file_name(db_name, table_name);
    std::string ix_file_name = get_ix_file_name(db_name, table_name);

    std::fstream file(file_name, std::ios::binary | std::ios::in | std::ios::out);
    std::fstream ix_file(ix_file_name, std::ios::binary | std::ios::in | std::ios::out);

    std::map<int64_t, int64_t> ix2offset = ix_read_all(ix_file);
    int64_t map_size = static_cast<int64_t>(ix2offset.size());
    std::vector<py::bytes> rows(map_size);
    char* buffer = new char[record_size + IX_SIZE];

    int idx = 0;
    for (const auto& [ix, record_offset] : ix2offset)
    {
        file.seekg(record_offset + IX_SIZE, ios::beg);
        file.read(buffer, record_size);
        memcpy(buffer + record_size, reinterpret_cast<const char *>(&ix), IX_SIZE);
        rows[idx++] = py::bytes(buffer, record_size + IX_SIZE);
    }

    delete[] buffer;
    ix_file.close();
    file.close();
    return rows;
}

py::dict QueryManager::build_record(const vector<int>& field_lengths, const vector<int>& field_types,
                                    const vector<string>& field_names, const int record_size,
                                    const py::bytes raw_record)
{
    char* buffer = new char[record_size + IX_SIZE];
    char* raw_record_data;
    Py_ssize_t raw_rec_size;
    PyBytes_AsStringAndSize(raw_record.ptr(), &raw_record_data, &raw_rec_size);
    memcpy(buffer, raw_record_data, record_size + IX_SIZE);

    const int64_t fields_count = field_lengths.size();
    vector<char*> bin_values(fields_count);
    int64_t offset = 0;

    for (int64_t i = 0; i < fields_count; i++)
    {
        char *field_buffer = new char[field_lengths[i]];
        memcpy(field_buffer, buffer + offset, field_lengths[i]);
        offset += field_lengths[i];
        bin_values[i] = field_buffer;
    }

    std::vector<py::object> record_vector = PYTHNOIZE_RECORD(bin_values, field_types, field_lengths);
    std::unordered_map<std::string, py::object> record;
    for (int64_t i = 0; i < fields_count; i++) 
    {
        record[field_names[i]] = record_vector[i];
    }

    int64_t ix;
    memcpy(&ix, buffer + record_size, IX_SIZE);
    record[ID_KEY] = py::cast(ix);

    delete[] buffer;
    return py::cast(record);
}

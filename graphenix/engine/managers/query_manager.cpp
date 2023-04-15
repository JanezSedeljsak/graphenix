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
#include <algorithm>

#include "managers.h"
#include "../util.cpp"
#include "../parser.hpp"

inline std::unordered_map<int64_t, int64_t> ix_read_all(std::fstream &ix_file)
{
    ix_file.seekg(0, std::ios::end);
    const int64_t file_size = ix_file.tellg();

    std::unordered_map<int64_t, int64_t> ix2offset;
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


std::vector<py::bytes> QueryManager::execute_query(const query_object& qobject)
{
    const auto& mdef = qobject.mdef;
    std::string file_name = get_file_name(mdef.db_name, mdef.model_name);
    std::string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);

    std::fstream file(file_name, std::ios::binary | std::ios::in | std::ios::out);
    std::fstream ix_file(ix_file_name, std::ios::binary | std::ios::in | std::ios::out);

    // TODO: if any field is defined that has a index tree find those records - only if conidtions on thoose fields are set
    // if there are multiple read all of them and then make a set intersection between them
    // later it will also need to handle OR/AND operators
    // if there are no filters and no order fields that don't have index trees and a limit is defined take only <qobject.limit> amount of records
    const std::unordered_map<int64_t, int64_t> ix2offset = ix_read_all(ix_file);
    const size_t map_size = ix2offset.size();
    ix_file.close();
    int64_t idx = 0;
    
    std::unordered_map<int64_t, int64_t> offset2ix;
    std::vector<int64_t> offsets(map_size);
    for (const auto& [ix, record_offset] : ix2offset)
    {
        offset2ix[record_offset] = ix;
        offsets[idx++] = record_offset;
    }

    std::sort(offsets.begin(), offsets.end());
    std::vector<std::vector<int64_t>> clusters = clusterify(offsets);

    char** raw_rows = new char*[map_size];
    std::vector<py::bytes> rows(map_size);
    bool check_limit = qobject.limit > 0;
    bool is_sorting = qobject.field_indexes.size() > 0;
    idx = 0;

    char* buffer = new char[MAX_CLUSTER_SIZE + mdef.record_size + IX_SIZE];
    bool break_cluster_loop = false;
    for (const auto& cluster : clusters)
    {
        const int64_t first_offset = cluster.front();
        const int64_t last_offset = cluster.back();
        file.seekg(first_offset, ios::beg);
        file.read(buffer, last_offset + mdef.record_size + IX_SIZE - first_offset);
        for (const auto& offset : cluster)
        {
            raw_rows[idx] = new char[mdef.record_size + IX_SIZE];
            memcpy(raw_rows[idx], buffer + offset + IX_SIZE - first_offset, mdef.record_size);
            memcpy(raw_rows[idx++] + mdef.record_size, reinterpret_cast<const char *>(&offset2ix[offset]), IX_SIZE);
            // TODO: filtering for non indexed fields should be here
            if (check_limit && !is_sorting && idx > qobject.limit)
            {
                std::cout << "break early\n";
                break_cluster_loop = true;
                break;
            }
        }

        if (break_cluster_loop)
        {
            break;
        }
    }

    delete[] buffer;
    file.close();

    if (is_sorting)
        std::sort(raw_rows, raw_rows + map_size, [&](char* a, char* b) {
            return qobject(a, b);
        });

    if (check_limit && idx > qobject.limit)
        rows.resize(qobject.limit);

    for (size_t i = 0, n = rows.size(); i < n; i++)
        rows[i] = py::bytes(raw_rows[i], mdef.record_size + IX_SIZE);

    for (int64_t i = 0; i < idx; i++)
        delete[] raw_rows[i];

    delete[] raw_rows;
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

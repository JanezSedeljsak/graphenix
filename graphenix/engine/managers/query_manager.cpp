#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <string>
#include <numeric>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "managers.h"
#include "../util.cpp"
#include "../parser.hpp"

inline void ix_read_all(std::fstream &ix_file, std::vector<std::pair<int64_t, int64_t>> &offsets)
{
    ix_file.seekg(0, std::ios::end);
    const int64_t file_size = ix_file.tellg();
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
                // ix2offset[rec_ix] = rec_offset;
                // offset2ix[rec_offset] = rec_ix;
                offsets.push_back(std::make_pair(rec_offset, rec_ix));
            }
        }

        offset += current_chunk_size;
    }
}

std::vector<py::bytes> QueryManager::execute_query(const query_object &qobject)
{
    const auto &mdef = qobject.mdef;
    std::string file_name = get_file_name(mdef.db_name, mdef.model_name);
    std::string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);

    std::fstream file(file_name, std::ios::binary | std::ios::in | std::ios::out);
    std::fstream ix_file(ix_file_name, std::ios::binary | std::ios::in | std::ios::out);

    // TODO: if any field is defined that has a index tree find those records - only if conidtions on thoose fields are set
    // if there are multiple read all of them and then make a set intersection between them
    // later it will also need to handle OR/AND operators
    // if there are no filters and no order fields that don't have index trees and a limit is defined take only <qobject.limit> amount of records
    // hashmap_ii ix2offset;
    // hashmap_ii offset2ix;
    std::vector<std::pair<int64_t, int64_t>> offsets;

    ix_read_all(ix_file, offsets);
    const size_t ix_amount = offsets.size();
    ix_file.close();

    std::sort(offsets.begin(), offsets.end());
    const auto &clusters = clusterify(offsets);

    // check limit is implemented this way so that if limit is >= 1000 it sorts at the end
    const bool check_limit = 0 < qobject.limit;
    const bool is_sorting = qobject.field_indexes.size() > 0;
    const size_t K = qobject.limit + qobject.offset;

    std::vector<char *> raw_rows;
    raw_rows.reserve(!check_limit ? ix_amount : K);
    std::priority_queue<char *, std::vector<char *>, query_object> pq(qobject);

    char *buffer = new char[MAX_CLUSTER_SIZE + mdef.record_size];
    bool break_cluster_loop = false;

    for (const auto &cluster : clusters)
    {
        const int64_t first_offset = cluster.front().first;
        const int64_t last_offset = cluster.back().first;
        file.seekg(first_offset + IX_SIZE, ios::beg);
        file.read(buffer, last_offset + mdef.record_size - first_offset);
        for (const auto &offset : cluster)
        {
            char *current_record = new char[mdef.record_size + IX_SIZE];
            memcpy(current_record, buffer + offset.first - first_offset, mdef.record_size);
            memcpy(current_record + mdef.record_size, reinterpret_cast<const char *>(&offset.second), IX_SIZE);
            const bool valid = const_cast<query_object&>(qobject).validate_conditions(current_record);
            if (!valid)
            {
                delete[] current_record;
                continue;
            }

            if (is_sorting && check_limit)
            {
                if (pq.size() < K)
                    pq.push(current_record);
                else if (qobject(current_record, pq.top()))
                {
                    pq.pop();
                    pq.push(current_record);
                }
                else
                    delete[] current_record;
            }
            else
            {
                raw_rows.push_back(current_record);
                if (!is_sorting && check_limit && raw_rows.size() > K)
                {
                    delete[] raw_rows[K];
                    raw_rows.resize(K);
                    break_cluster_loop = true;
                    break;
                }
            }
        }

        if (break_cluster_loop)
        {
            break;
        }
    }

    delete[] buffer;
    file.close();

    if (is_sorting && check_limit)
    {
        while (!pq.empty())
        {
            raw_rows.push_back(pq.top());
            pq.pop();
        }

        std::reverse(raw_rows.begin(), raw_rows.end());
    }
    else if (is_sorting)
    {
        std::sort(raw_rows.begin(), raw_rows.end(), [&](char *a, char *b)
                  { return qobject(a, b); });
    }

    // the end index is set to K if we have limiting
    // we need to take into account the offset which is subtracted for correct index matching when setting the result
    size_t end = (check_limit ? std::min(K, raw_rows.size()) : raw_rows.size()) - qobject.offset;
    std::vector<py::bytes> rows(end);
    for (size_t i = 0; i < end; i++)
        rows[i] = py::bytes(raw_rows[i + qobject.offset], mdef.record_size + IX_SIZE);

    for (size_t i = 0; i < raw_rows.size(); i++)
        delete[] raw_rows[i];

    return rows;
}

py::dict QueryManager::build_record(const model_def &mdef, const py::bytes raw_record)
{
    char *buffer = new char[mdef.record_size + IX_SIZE];
    char *raw_record_data;
    Py_ssize_t raw_rec_size;
    PyBytes_AsStringAndSize(raw_record.ptr(), &raw_record_data, &raw_rec_size);
    memcpy(buffer, raw_record_data, mdef.record_size + IX_SIZE);

    const int64_t fields_count = mdef.field_sizes.size();
    vector<char *> bin_values(fields_count);
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

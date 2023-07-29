#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <numeric>
#include <limits>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <variant>
#include <any>
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

inline py::tuple build_record(const model_def &mdef, const py::bytes raw_record)
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
        // skip virtual links can only be created through queries
        if (mdef.field_types[i] == VIRTUAL_LINK)
            continue;

        char *field_buffer = new char[mdef.field_sizes[i]];
        memcpy(field_buffer, buffer + offset, mdef.field_sizes[i]);
        offset += mdef.field_sizes[i];
        bin_values[i] = field_buffer;
    }

    std::vector<py::object> record_vector = PYTHNOIZE_RECORD(mdef, bin_values);
    py::tuple record(fields_count + 1); // +1 for ID
    for (int64_t i = 1; i <= fields_count; i++)
        record[i] = record_vector[i - 1];

    int64_t ix;
    memcpy(&ix, buffer + mdef.record_size, IX_SIZE);
    record[0] = py::cast(ix);

    delete[] buffer;
    return record;
}

std::vector<py::tuple> QueryManager::execute_agg_query(const query_object &qobject)
{
    const auto &mdef = qobject.mdef;
    std::string file_name = get_file_name(mdef.db_name, mdef.model_name);
    std::string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);

    std::fstream file(file_name, std::ios::binary | std::ios::in | std::ios::out);
    std::fstream ix_file(ix_file_name, std::ios::binary | std::ios::in | std::ios::out);
    std::vector<std::pair<int64_t, int64_t>> offsets;

    ix_read_all(ix_file, offsets);
    ix_file.close();

    const bool is_global_agg = qobject.agg_field_index == -1;
    const FIELD_TYPE field_type = static_cast<FIELD_TYPE>(mdef.field_types[qobject.agg_field_index]);
    const int64_t key_field_offset = mdef.field_offsets[qobject.agg_field_index];
    const int64_t key_field_size = mdef.field_offsets[qobject.agg_field_index];

    std::unordered_map<std::variant<std::string, double, int64_t>, std::vector<py::object>> hashmap;
    query_object qobject_temp = const_cast<query_object &>(qobject);
    std::vector<py::object> agg_tuple = qobject_temp.make_agg_tuple();

    if (is_global_agg)
        hashmap[0] = agg_tuple;

    std::sort(offsets.begin(), offsets.end());
    const auto &clusters = clusterify(offsets);

    char *buffer = new char[MAX_CLUSTER_SIZE + mdef.record_size];
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
            const bool valid = qobject_temp.validate_conditions(current_record);
            if (!valid)
            {
                delete[] current_record;
                continue;
            }

            if (is_global_agg)
            {
                qobject_temp.compare_and_swap(current_record, hashmap[0]);
            }
            else
            {
                char *field_ptr = current_record + key_field_offset;
                std::variant<std::string, double, int64_t> key;
                switch (field_type)
                {
                case INT:
                case LINK:
                case BOOL:
                case DATETIME:
                    key = *reinterpret_cast<int64_t *>(field_ptr);
                    break;

                case STRING:
                {
                    std::string str_key(field_ptr, key_field_size);
                    key = str_key;
                    break;
                }

                case DOUBLE:
                    key = *reinterpret_cast<double *>(field_ptr);
                    break;

                default:
                    throw std::runtime_error("Invaldi type for group key!");
                    break;
                }

                auto it = hashmap.find(key);
                if (it == hashmap.end())
                {
                    // add new object to hashmap
                    std::vector<py::object> temp_tuple = qobject_temp.make_agg_tuple();
                    hashmap[key] = temp_tuple;
                }

                qobject_temp.compare_and_swap(current_record, hashmap[key]);
            }
        }
    }

    file.close();
    delete[] buffer;
    std::vector<py::tuple> rows;

    if (is_global_agg)
    {
        std::vector<py::object> global_agg_vector = hashmap[0];
        py::tuple global_agg(global_agg_vector.size());
        for (size_t i = 0; i < global_agg_vector.size(); i++)
            global_agg[i] = global_agg_vector[i];

        rows.push_back(global_agg);
        return rows;
    }

    rows.resize(hashmap.size());
    size_t idx = 0;
    for (const auto &pair : hashmap)
    {
        std::vector<py::object> global_agg_vector = pair.second;
        py::tuple global_agg(global_agg_vector.size() + 1);
        for (size_t i = 0; i < global_agg_vector.size(); i++)
            global_agg[i + 1] = global_agg_vector[i];

        global_agg[0] = pair.first;
        rows[idx] = global_agg;
        idx++;
    }

    return rows;
}

std::vector<py::tuple> QueryManager::execute_entity_query(const query_object &qobject)
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
    ix_file.close();

    if (qobject.is_subquery && qobject.has_ix_constraints)
    {
        // used for direct links
        offsets.erase(std::remove_if(offsets.begin(), offsets.end(),
                                     [&](const std::pair<int64_t, int64_t> &p)
                                     {
                                         return qobject.ix_constraints.find(p.second) == qobject.ix_constraints.end();
                                     }),
                      offsets.end());
    }

    const size_t ix_amount = offsets.size();
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
            const bool valid = const_cast<query_object &>(qobject).validate_conditions(current_record);
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
    std::vector<py::tuple> rows(end);
    for (size_t i = 0; i < end; i++)
    {
        py::bytes raw_record = py::bytes(raw_rows[i + qobject.offset], mdef.record_size + IX_SIZE);
        rows[i] = build_record(mdef, raw_record);
    }

    for (size_t i = 0; i < raw_rows.size(); i++)
        delete[] raw_rows[i];

    return rows;
}

std::vector<py::tuple> QueryManager::execute_query(const query_object &qobject, const int depth)
{
    // execute main query
    std::vector<py::tuple> root_result = QueryManager::execute_entity_query(qobject);
    const size_t subquery_count = qobject.link_vector.size();
    if (subquery_count == 0 || root_result.size() == 0)
        return root_result;

    // preapre ix constraints for subquery execution
    const size_t record_size = qobject.mdef.field_offsets.size() + 1;
    std::vector<std::unordered_map<int64_t, std::vector<py::tuple>>> subquery_result_maps(subquery_count);
    std::vector<std::unordered_set<int64_t>> ix_set(subquery_count);
    for (py::tuple rec_tuple : root_result)
    {
        for (size_t i = 0; i < subquery_count; i++)
        {
            // we add 1 to the field_index because of PK which is not in the default fields vector
            const int field_index = qobject.links[i].is_direct_link
                                        ? (qobject.links[i].link_field_index + 1)
                                        : 0;

            int64_t link_key = 0;
            if (py::isinstance<py::tuple>(rec_tuple[field_index]))
                link_key = py::cast<int64_t>(rec_tuple[field_index][0]);
            else
                link_key = py::cast<int64_t>(rec_tuple[field_index]);

            if (link_key == -1)
                continue;

            ix_set[i].insert(link_key);
            subquery_result_maps[i].emplace(link_key, std::vector<py::tuple>());
        }
    }

    // recursively execute subqueries and push data into maps
    for (size_t i = 0; i < subquery_count; i++)
    {
        query_object subquery = qobject.link_vector[i];
        if (subquery.has_ix_constraints)
            subquery.ix_constraints = ix_set[i];
        else
        {
            // TODO: add the ix_set[i] to subquery.filter_root.conditions
        }

        std::vector<py::tuple> subquery_result = QueryManager::execute_query(subquery, depth + 1);
        auto &current_map = subquery_result_maps[i];
        for (py::tuple rec_tuple : subquery_result)
        {
            const int child_link_index = qobject.links[i].child_link_field_index + 1;
            const int64_t link_key = py::cast<int64_t>(rec_tuple[child_link_index]);
            if (link_key == -1)
                continue;

            current_map[link_key].push_back(rec_tuple);
        }
    }

    // loop through the root result and append the subquery results
    std::vector<py::object> temp_record(record_size);
    for (size_t j = 0, n = root_result.size(); j < n; j++)
    {
        py::tuple current = root_result[j];
        for (size_t i = 0; i < record_size; i++)
            temp_record[i] = current[i];

        for (size_t i = 0; i < subquery_count; i++)
        {
            auto &current_map = subquery_result_maps[i];
            const int field_index = qobject.links[i].is_direct_link
                                        ? (qobject.links[i].link_field_index + 1)
                                        : 0;

            int64_t link_key = 0;
            if (py::isinstance<py::tuple>(current[field_index]))
                link_key = py::cast<int64_t>(current[field_index][0]);
            else
                link_key = py::cast<int64_t>(current[field_index]);

            std::vector<py::tuple> groupped_records = current_map[link_key];
            if (qobject.links[i].is_direct_link)
            {
                if (groupped_records.size() > 0)
                    temp_record[field_index] = groupped_records[0];
                else
                    temp_record[field_index] = py::cast(-1);
            }
            else
            {
                const size_t list_size = groupped_records.size();
                const int list_field_index = qobject.links[i].link_field_index + 1;

                if (qobject.links[i].offset >= list_size)
                    temp_record[list_field_index] = py::list();

                else if (qobject.links[i].limit == 0 && qobject.links[i].offset == 0)
                    temp_record[list_field_index] = py::cast(groupped_records);

                else
                {
                    size_t end_idx = qobject.links[i].limit > 0
                                         ? std::min(list_size, qobject.links[i].offset + qobject.links[i].limit)
                                         : list_size;

                    std::vector<py::tuple> records_with_limit = std::vector<py::tuple>(
                        groupped_records.begin() + qobject.links[i].offset,
                        groupped_records.begin() + end_idx);

                    temp_record[list_field_index] = py::cast(records_with_limit);
                }
            }
        }

        py::tuple record_with_links(record_size);
        for (size_t i = 0; i < record_size; i++)
            record_with_links[i] = temp_record[i];

        root_result[j] = record_with_links;
    }

    return root_result;
}
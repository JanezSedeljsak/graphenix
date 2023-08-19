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
#include <tuple>
#include <algorithm>
#include <variant>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "managers.h"
#include "../util.hpp"
#include "../parser.hpp"
#include "../view.hpp"

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

inline void ix_read_with_constraints(std::fstream &ix_file,
                                     std::vector<std::pair<int64_t, int64_t>> &offsets,
                                     const std::unordered_set<int64_t> &ixs)
{
    for (const auto &ix : ixs)
    {
        const int64_t rec_offset = get_record_offset(ix, ix_file);
        offsets.push_back(std::make_pair(rec_offset, ix));
    }
}

inline py::tuple build_record(const model_def &mdef, char *raw_record)
{
    const int64_t fields_count = mdef.field_sizes.size();
    vector<char *> bin_values(fields_count);
    int64_t offset = 0;

    for (int64_t i = 0; i < fields_count; i++)
    {
        // skip virtual links can only be created through queries
        if (mdef.field_types[i] == VIRTUAL_LINK)
            continue;

        char *field_buffer = new char[mdef.field_sizes[i]];
        memcpy(field_buffer, raw_record + offset, mdef.field_sizes[i]);
        offset += mdef.field_sizes[i];
        bin_values[i] = field_buffer;
    }

    std::vector<py::object> record_vector = PYTHNOIZE_RECORD(mdef, bin_values);
    py::tuple record(fields_count + 1); // +1 for ID
    for (int64_t i = 1; i <= fields_count; i++)
        record[i] = record_vector[i - 1];

    int64_t ix;
    memcpy(&ix, raw_record + mdef.record_size, IX_SIZE);
    record[0] = py::cast(ix);

    DEALLOCATE_RECORD(bin_values);
    return record;
}

inline std::unordered_set<int64_t> evaluate_btree_conditions_every(const model_def &mdef, cond_node &cnode)
{
    std::unordered_set<int64_t> every_result;
    bool first_set_initialized = false;

    for (auto &cobject : cnode.btree_conditions)
    {
        std::unordered_set<int64_t> current_result = cobject.validate_indexed(mdef);
        if (!first_set_initialized)
        {
            every_result = current_result;
            first_set_initialized = true;
        }
        else
            every_result = make_set_intersection(every_result, current_result);
    }

    return every_result;
}

inline std::unordered_set<int64_t> evaluate_btree_conditions_some(const model_def &mdef, cond_node &cnode)
{
    std::unordered_set<int64_t> some_result = std::unordered_set<int64_t>();
    for (auto cobject : cnode.btree_conditions)
    {
        // make set union
        std::unordered_set<int64_t> current_result = cobject.validate_indexed(mdef);
        some_result.insert(current_result.begin(), current_result.end());
    }

    return some_result;
}

inline void visit_and_evaluate(const model_def &mdef, cond_node &cnode, const int depth)
{
    // root is evaluated seperately
    if (cnode.btree_conditions.size() > 0 && depth != 0)
        cnode.tree_ixs = cnode.is_and
                             ? evaluate_btree_conditions_every(mdef, cnode)
                             : evaluate_btree_conditions_some(mdef, cnode);

    for (auto &child : cnode.children)
        visit_and_evaluate(mdef, child, depth + 1);
}

inline bool is_instant_limit(const query_object &qobject)
{
    // instant limit if limit is defined and no ordering or conditions
    return (
        qobject.limit > 0 &&
        qobject.field_indexes.size() == 0 &&
        qobject.filter_root.conditions.size() == 0 &&
        qobject.filter_root.children.size() == 0);
}

inline std::tuple<bool, std::unordered_set<int64_t>>
search_for_pk_conditions(const model_def &mdef, cond_node &cnode,
                         const std::vector<std::pair<int64_t, int64_t>> &offsets)
{
    std::unordered_set<int64_t> condition_indexes;
    std::vector<int> indexes_to_remove;

    for (size_t i = 0; i < cnode.conditions.size(); ++i)
    {
        std::unordered_set<int64_t> current_condition_indexes;
        auto &cond = cnode.conditions[i];
        if (!cond.field_name.empty())
            continue;

        switch (cond.operation_index)
        {
        case EQUAL:
        {
            const int64_t int_val = py::cast<int64_t>(cond.value);
            current_condition_indexes.insert(int_val);
            indexes_to_remove.push_back(i);
            break;
        }
        case IS_IN:
        {
            if (!py::isinstance<py::list>(cond.value))
            {
                throw std::runtime_error("You did not provide a list as the IN/NOT_IN argument");
                break;
            }

            indexes_to_remove.push_back(i);
            for (py::handle item : cond.value)
            {
                const int64_t current_key = py::cast<int64_t>(item);
                current_condition_indexes.insert(current_key);
            }
            break;
        }
        case LESS:
        case LESS_OR_EQUAL:
        case GREATER:
        case GREATER_OR_EQUAL:
        {
            const int64_t int_val = py::cast<int64_t>(cond.value);
            indexes_to_remove.push_back(i);
            for (const auto &pair : offsets)
                if ((cond.operation_index == LESS && pair.second < int_val) ||
                    (cond.operation_index == LESS_OR_EQUAL && pair.second <= int_val) ||
                    (cond.operation_index == GREATER && pair.second > int_val) ||
                    (cond.operation_index == GREATER_OR_EQUAL && pair.second >= int_val))
                {
                    current_condition_indexes.insert(pair.second);
                }

            break;
        }
        case BETWEEN:
        {
            py::tuple interval = py::cast<py::tuple>(cond.value);
            const int64_t low = py::cast<int64_t>(interval[0]);
            const int64_t high = py::cast<int64_t>(interval[1]);

            indexes_to_remove.push_back(i);
            for (const auto &pair : offsets)
                if (low <= pair.second && pair.second <= high)
                    current_condition_indexes.insert(pair.second);

            break;
        }

        default:
            break;
        }

        condition_indexes = indexes_to_remove.size() > 1
                                ? make_set_intersection(condition_indexes, current_condition_indexes)
                                : current_condition_indexes;
    }

    for (auto rit = indexes_to_remove.rbegin(); rit != indexes_to_remove.rend(); ++rit)
        cnode.conditions.erase(cnode.conditions.begin() + *rit);

    return make_tuple(indexes_to_remove.size() > 0, condition_indexes);
}

std::vector<py::tuple> QueryManager::execute_agg_query(const query_object &qobject)
{
    const auto &mdef = qobject.mdef;
    std::string file_name = get_file_name(mdef.db_name, mdef.model_name);
    std::string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);

    std::fstream file(file_name, std::ios::binary | std::ios::in | std::ios::out);
    std::fstream ix_file(ix_file_name, std::ios::binary | std::ios::in | std::ios::out);
    query_object qobject_editable = const_cast<query_object &>(qobject);
    std::vector<std::pair<int64_t, int64_t>> offsets;

    ix_read_all(ix_file, offsets);
    ix_file.close();

    std::unordered_set<int64_t> instant_pk_filter;
    bool has_pk_filter = false;

    // chech for indexed conditions in condition root and update offsets
    if (qobject.filter_root.btree_conditions.size() > 0)
    {
        std::unordered_set<int64_t> indexed_conditions = evaluate_btree_conditions_every(mdef, qobject_editable.filter_root);
        qobject_editable.filter_root.btree_conditions.clear();

        has_pk_filter = true;
        instant_pk_filter.insert(indexed_conditions.begin(), indexed_conditions.end());
    }

    if (qobject.filter_root.conditions.size() > 0)
    {
        const auto [has_pk_conditions, indexed_conditions] = search_for_pk_conditions(mdef, qobject_editable.filter_root, offsets);
        if (has_pk_conditions)
        {
            has_pk_filter = true;
            instant_pk_filter.insert(indexed_conditions.begin(), indexed_conditions.end());
        }
    }

    if (has_pk_filter)
    {
        offsets.erase(std::remove_if(offsets.begin(), offsets.end(),
                                     [&](const std::pair<int64_t, int64_t> &p)
                                     { return instant_pk_filter.count(p.second) == 0; }),
                      offsets.end());
    }

    if (!offsets.size())
        return std::vector<py::tuple>();

    // evaluate condition tree
    visit_and_evaluate(mdef, qobject_editable.filter_root, 0);
    const bool is_global_agg = qobject.agg_field_index == -1;
    const FIELD_TYPE field_type = static_cast<FIELD_TYPE>(mdef.field_types[qobject.agg_field_index]);
    const int64_t key_field_offset = mdef.field_offsets[qobject.agg_field_index];

    std::unordered_map<std::variant<std::string, double, int64_t>, std::vector<py::object>> hashmap;
    std::vector<py::object> agg_tuple = qobject_editable.make_agg_tuple();

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
            const bool valid = qobject_editable.validate_conditions(offset.second, current_record);
            if (!valid)
            {
                delete[] current_record;
                continue;
            }

            if (is_global_agg)
            {
                qobject_editable.compare_and_swap(current_record, hashmap[0]);
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
                    const size_t str_size = std::strlen(field_ptr);
                    std::string str_key(field_ptr, str_size);
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
                    std::vector<py::object> temp_tuple = qobject_editable.make_agg_tuple();
                    hashmap[key] = temp_tuple;
                }

                qobject_editable.compare_and_swap(current_record, hashmap[key]);
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

    std::sort(rows.begin(), rows.end());
    return rows;
}

std::vector<py::tuple> QueryManager::execute_entity_query(const query_object &qobject)
{
    const auto &mdef = qobject.mdef;
    std::string file_name = get_file_name(mdef.db_name, mdef.model_name);
    std::string ix_file_name = get_ix_file_name(mdef.db_name, mdef.model_name);

    std::fstream file(file_name, std::ios::binary | std::ios::in | std::ios::out);
    std::fstream ix_file(ix_file_name, std::ios::binary | std::ios::in | std::ios::out);

    query_object qobject_editable = const_cast<query_object &>(qobject);
    std::vector<std::pair<int64_t, int64_t>> offsets;
    const bool check_limit = qobject.limit > 0;
    const bool is_sorting = qobject.field_indexes.size() > 0;
    const size_t K = qobject.limit + qobject.offset;

    std::unordered_set<int64_t> instant_pk_filter;
    bool has_pk_filter = false;

    // check for instaint limiting through an index condition
    if (is_sorting && check_limit && qobject.is_indexed_instant_limit() && false)
    {
        // todo get first K elements with qobject.filter_root.btree_conditions[0]
        has_pk_filter = true;
    }

    // chech for indexed conditions in condition root
    else if (qobject.filter_root.btree_conditions.size() > 0)
    {
        std::unordered_set<int64_t> indexed_conditions = evaluate_btree_conditions_every(mdef, qobject_editable.filter_root);
        qobject_editable.filter_root.btree_conditions.clear();

        has_pk_filter = true;
        instant_pk_filter = indexed_conditions;
    }

    if (has_pk_filter && instant_pk_filter.size() < IX_THRESHOLD)
    {
        ix_read_with_constraints(ix_file, offsets, instant_pk_filter);
        ix_file.close();
    }
    else
    {
        ix_read_all(ix_file, offsets);
        ix_file.close();
    }

    // check for PK conditions
    if (qobject.filter_root.conditions.size() > 0)
    {
        const auto [has_pk_conditions, indexed_conditions] = search_for_pk_conditions(mdef, qobject_editable.filter_root, offsets);
        if (has_pk_conditions)
        {
            has_pk_filter = true;
            instant_pk_filter.insert(indexed_conditions.begin(), indexed_conditions.end());
        }
    }

    // check for subquery conditions
    if (qobject.is_subquery && qobject.has_ix_constraints)
    {
        // used for direct links
        has_pk_filter = true;
        instant_pk_filter.insert(qobject.ix_constraints.begin(), qobject.ix_constraints.end());
    }

    if (has_pk_filter)
    {
        offsets.erase(std::remove_if(offsets.begin(), offsets.end(),
                                     [&](const std::pair<int64_t, int64_t> &p)
                                     { return instant_pk_filter.count(p.second) == 0; }),
                      offsets.end());
    }

    // evaluate condition tree
    visit_and_evaluate(mdef, qobject_editable.filter_root, 0);

    if (!offsets.size())
        return std::vector<py::tuple>();

    if (is_instant_limit(qobject))
        offsets.resize(K);

    const size_t ix_amount = offsets.size();
    std::sort(offsets.begin(), offsets.end());
    const auto &clusters = clusterify(offsets);

    std::vector<char *> raw_rows;
    raw_rows.reserve(!check_limit ? ix_amount : K);
    std::priority_queue<char *, std::vector<char *>, query_object> pq(qobject);

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
            const bool valid = qobject_editable.validate_conditions(offset.second, current_record);
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
            }
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
    const size_t size_without_offset = check_limit ? std::min(K, raw_rows.size()) : raw_rows.size();
    const size_t end = size_without_offset > qobject.offset ? (size_without_offset - qobject.offset) : 0;

    std::vector<py::tuple> rows(end);
    for (size_t i = 0; i < end; i++)
    {
        py::tuple parsed_record = build_record(mdef, raw_rows[i + qobject.offset]);
        // here -1 MARKS PK this is why we have -2 as default
        if (qobject.picked_index != -2)
        {
            py::tuple picked(1);
            picked[0] = parsed_record[qobject.picked_index + 1];
            parsed_record = picked;
        }

        rows[i] = parsed_record;
    }

    for (size_t i = 0; i < raw_rows.size(); i++)
        delete[] raw_rows[i];

    return rows;
}

View QueryManager::execute_query(const query_object &qobject, const int depth)
{
    // execute main query
    const auto &mdef = qobject.mdef;
    std::vector<py::tuple> root_result = QueryManager::execute_entity_query(qobject);
    const size_t subquery_count = qobject.link_vector.size();
    const size_t result_size = root_result.size();
    if (subquery_count == 0 || result_size == 0)
        return View::make_view(mdef.field_names, root_result,
                               mdef.field_date_indexes, mdef.model_name);

    // preapre ix constraints for subquery execution
    std::vector<std::unordered_map<int64_t, View>> subquery_result_maps(subquery_count);
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
            subquery_result_maps[i].emplace(link_key, View(
                                                          qobject.link_vector[i].mdef.field_names,
                                                          qobject.link_vector[i].mdef.model_name));
        }
    }

    // recursively execute subqueries and push data into maps
    for (size_t i = 0; i < subquery_count; i++)
    {
        query_object subquery = qobject.link_vector[i];
        const int field_index = qobject.links[i].child_link_field_index;
        if (subquery.has_ix_constraints)
            subquery.ix_constraints = ix_set[i];
        else if (qobject.link_vector[i].mdef.field_indexes[field_index])
        {
            py::list current_ixs;
            for (const auto &ix : ix_set[i])
                current_ixs.append(ix);

            std::string fname = qobject.link_vector[i].mdef.field_names[field_index];
            query_object qobject_editable = const_cast<query_object &>(qobject);
            cond_object cobj{fname, IS_IN, current_ixs};
            qobject_editable.filter_root.btree_conditions.push_back(cobj);
        }

        View subquery_result = QueryManager::execute_query(subquery, depth + 1);
        auto &current_map = subquery_result_maps[i];
        for (Record rec_tuple : subquery_result.records)
        {
            const int child_link_index = qobject.links[i].child_link_field_index + 1;
            const int64_t link_key = py::cast<int64_t>(rec_tuple.record[child_link_index]);
            if (link_key == -1)
                continue;

            current_map[link_key].records.push_back(rec_tuple);
        }
    }

    // loop through the root result and append the subquery results
    for (size_t j = 0, n = root_result.size(); j < n; j++)
    {
        py::tuple &current = root_result[j];
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

            View groupped_records = current_map[link_key];
            if (qobject.links[i].is_direct_link)
            {
                if (groupped_records.size() > 0)
                    current[field_index] = RecordView(groupped_records);
                else
                    current[field_index] = py::cast(-1);
            }
            else
            {
                const size_t list_size = groupped_records.size();
                const int list_field_index = qobject.links[i].link_field_index + 1;

                if (qobject.links[i].offset >= list_size)
                    current[list_field_index] = View(qobject.link_vector[i].mdef.field_names,
                                                     qobject.link_vector[i].mdef.model_name);

                else if (qobject.links[i].limit == 0 && qobject.links[i].offset == 0)
                    current[list_field_index] = groupped_records;

                else
                {
                    const size_t end_idx = qobject.links[i].limit > 0
                                               ? std::min(list_size, qobject.links[i].offset + qobject.links[i].limit)
                                               : list_size;

                    std::vector<Record> records_with_limit = std::vector<Record>(
                        groupped_records.records.begin() + qobject.links[i].offset,
                        groupped_records.records.begin() + end_idx);

                    View view_instance = View(qobject.link_vector[i].mdef.field_names,
                                              qobject.link_vector[i].mdef.model_name);

                    view_instance.records = records_with_limit;
                    current[list_field_index] = view_instance;
                }
            }
        }
    }

    return View::make_view(mdef.field_names, root_result,
                           mdef.field_date_indexes, mdef.model_name);
}
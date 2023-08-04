#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <regex>
#include <unordered_set>

#include <pybind11/pybind11.h>

#include "bptree/bptreeindex.hpp"

#define IX_SIZE 8 // size of 8 bytes <==> IX_SIZE
#define PK_IX_HEAD_SIZE IX_SIZE * 2
#define CHUNK_SIZE 10 * 1024 * 1024
#define ID_KEY "_id"

#define MIN_CLUSTER_SIZE 50
#define MAX_CLUSTER_SIZE 1000000 // 1 MB

typedef std::unordered_map<int64_t, int64_t> hashmap_ii;
typedef std::vector<int64_t> vector_i64;
typedef std::vector<int> vector_i;

namespace py = pybind11;

enum FIELD_TYPE
{
    INT = 0,
    STRING = 1,
    BOOL = 2,
    DATETIME = 3,
    LINK = 4,
    DOUBLE = 5,
    VIRTUAL_LINK = 6
};

enum FILTER_OPERATION_TYPE
{
    EQUAL = 0,
    NOTEQUAL = 1,
    GREATER = 2,
    GREATER_OR_EQUAL = 3,
    LESS = 4,
    LESS_OR_EQUAL = 5,
    REGEX = 6,
    IS_IN = 7,
    NOT_IN = 8,
    BETWEEN = 9
};

enum AGGREGATE_OPERATION
{
    SUM = 0,
    MIN = 1,
    MAX = 2,
    COUNT = 3
};

struct aggregate_object
{
    int option, field_index;
};

struct model_def
{
    std::string db_name;
    std::string model_name;
    vector_i field_sizes;
    vector_i field_types;
    vector_i field_offsets;
    std::vector<bool> field_indexes;
    std::vector<int> field_date_indexes;
    std::vector<std::string> field_names;
    int64_t record_size;
};

struct cond_object
{
    std::string field_name;
    int operation_index;
    py::object value;

    std::unordered_set<int64_t> validate_indexed(const model_def &mdef)
    {
        int idx = -1;
        if (!field_name.empty())
            for (size_t i = 0; i < mdef.field_names.size(); ++i)
                if (mdef.field_names[i] == field_name)
                    idx = static_cast<int>(i);

        const bool not_pk = idx != -1;
        const int offset = not_pk ? mdef.field_offsets[idx] : mdef.record_size;
        const int size = not_pk ? mdef.field_sizes[idx] : IX_SIZE;
        const FIELD_TYPE type = not_pk ? static_cast<FIELD_TYPE>(mdef.field_types[idx]) : INT;
        std::unordered_set<int64_t> result = std::unordered_set<int64_t>();

        switch (type)
        {
        case INT:
        case DATETIME:
        case LINK:
        {
            BPTreeIndex<int64_t> bpt("user", "uuid");
            switch (operation_index)
            {
            case IS_IN:
            {
                if (!py::isinstance<py::list>(value))
                {
                    throw std::runtime_error("You did not provide a list as the IN/NOT_IN argument");
                    return result;
                }

                for (py::handle item : value)
                {
                    const int64_t current_key = py::cast<int64_t>(item);
                    const auto &found = bpt.find(current_key);
                    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
                    result.insert(flatten.begin(), flatten.end());
                }
                return result;
            }
            case EQUAL:
            {
                const int64_t current_key = py::cast<int64_t>(value);
                const auto &found = bpt.find(current_key);
                const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
                result.insert(flatten.begin(), flatten.end());
                return result;
            }
            case BETWEEN:
            {
                // TODO run between search in btree
                return result;
            }
            default:
                throw std::runtime_error("Invalid index operation");
            }
        }
        case STRING:
        {
        }
        default:
            throw std::runtime_error("Invalid type for indexing");
        }

        return result;
    }

    bool validate(model_def &mdef, char *record)
    {
        int idx = -1;
        if (!field_name.empty())
            for (size_t i = 0; i < mdef.field_names.size(); ++i)
                if (mdef.field_names[i] == field_name)
                    idx = static_cast<int>(i);

        const bool not_pk = idx != -1;
        const int offset = not_pk ? mdef.field_offsets[idx] : mdef.record_size;
        const int size = not_pk ? mdef.field_sizes[idx] : IX_SIZE;
        const FIELD_TYPE type = not_pk ? static_cast<FIELD_TYPE>(mdef.field_types[idx]) : INT;
        char *cmp_field = record + offset;

        if (operation_index == IS_IN || operation_index == NOT_IN)
        {
            if (!py::isinstance<py::list>(value))
            {
                throw std::runtime_error("You did not provide a list as the IN/NOT_IN argument");
                return false;
            }

            // if we find a matching item and we are in IS_IN then we return TRUE
            // if we find a matching item and we are in NOT_IN we return FALSE
            const bool if_found_value = operation_index == IS_IN;
            py::list py_list = py::cast<py::list>(value);
            switch (type)
            {
            case INT:
            case DATETIME:
            case LINK:
            {
                const int64_t int_value = *reinterpret_cast<int64_t *>(cmp_field);
                for (py::handle item : py_list)
                {
                    const int64_t other = py::cast<int64_t>(item);
                    if (int_value == other)
                        return if_found_value;
                }

                return !if_found_value;
            }
            case STRING:
            {
                for (py::handle item : py_list)
                {
                    py::str py_str = py::cast<py::str>(item);
                    const std::string other = py_str;
                    if (std::strcmp(cmp_field, other.c_str()) == 0)
                        return if_found_value;
                }

                return !if_found_value;
            }
            case DOUBLE:
            {
                const double double_value = *reinterpret_cast<double *>(cmp_field);
                for (py::handle item : py_list)
                {
                    const double other = py::cast<double>(item);
                    if (double_value == other)
                        return if_found_value;
                }

                return !if_found_value;
            }
            case VIRTUAL_LINK:
            case BOOL:
                throw std::runtime_error("Cannot use VirtualLink or Boolean for checking IN condition!");
                break;

            default:
                throw std::runtime_error("Invalid comperator type!");
                break;
            }

            return false;
        }

        if (operation_index == BETWEEN)
        {
            py::tuple interval = py::cast<py::tuple>(value);
            switch (type)
            {
            case INT:
            case DATETIME:
            case LINK:
            {
                const int64_t int_value = *reinterpret_cast<int64_t *>(cmp_field);
                const int64_t low = py::cast<int64_t>(interval[0]);
                const int64_t high = py::cast<int64_t>(interval[1]);
                return low <= int_value && int_value <= high;
            }
            case STRING:
            {
                py::str py_low = py::cast<py::str>(interval[0]);
                py::str py_high = py::cast<py::str>(interval[1]);
                const std::string low = py_low;
                const std::string high = py_high;

                return 0 <= std::strcmp(cmp_field, low.c_str()) && std::strcmp(cmp_field, high.c_str()) <= 0;
            }
            case DOUBLE:
            {
                const double double_value = *reinterpret_cast<double *>(cmp_field);
                const double low = py::cast<double>(interval[0]);
                const double high = py::cast<double>(interval[1]);
                return low <= double_value && double_value <= high;
            }
            case VIRTUAL_LINK:
            case BOOL:
                throw std::runtime_error("Cannot use VirtualLink or Boolean for checking BETWEEN!");
                break;

            default:
                throw std::runtime_error("Invalid comperator type!");
                break;
            }

            return false;
        }

        int cmp_res = 0;
        switch (type)
        {
        case INT:
        case DATETIME:
        case LINK:
        {
            const int64_t int_a = *reinterpret_cast<int64_t *>(cmp_field);
            const int64_t int_b = py::cast<int64_t>(value);
            cmp_res = int_a - int_b;
            break;
        }
        case STRING:
        {
            py::str py_str = py::cast<py::str>(value);
            std::string val_b = py_str;
            if (operation_index == REGEX)
            {
                try
                {
                    const std::regex regex_obj(val_b);
                    const std::string trimed_value = std::string(cmp_field, size);
                    return std::regex_match(trimed_value, regex_obj);
                }
                catch (const std::regex_error &e)
                {
                    // if invalid regex just return false
                    return false;
                }

                return true;
            }

            cmp_res = std::strcmp(cmp_field, val_b.c_str());
            break;
        }
        case BOOL:
        {
            const bool bool_a = *reinterpret_cast<bool *>(cmp_field);
            const bool bool_b = py::cast<bool>(value);
            cmp_res = bool_a - bool_b;
            break;
        }
        case DOUBLE:
        {
            const double double_a = *reinterpret_cast<double *>(cmp_field);
            const double double_b = py::cast<double>(value);
            cmp_res = double_a - double_b;
            break;
        }
        case VIRTUAL_LINK:
            throw std::runtime_error("Cannot use VirtualLink for filtering!");
            break;

        default:
            throw std::runtime_error("Invalid comperator type!");
            break;
        }

        // std::cout << "Idx " << idx << " offset " << offset << " rec " << cmp_field << " " << value << std::endl;
        // std::cout << "res " << cmp_res << std::endl;
        switch (operation_index)
        {
        case EQUAL:
            return cmp_res == 0;
        case NOTEQUAL:
            return cmp_res != 0;
        case LESS:
            return cmp_res < 0;
        case LESS_OR_EQUAL:
            return cmp_res <= 0;
        case GREATER:
            return cmp_res > 0;
        case GREATER_OR_EQUAL:
            return cmp_res >= 0;
        default:
            throw std::runtime_error("Invalid comperator operation!");
            return false;
        }
    }
};

struct cond_node
{
    std::vector<cond_object> btree_conditions, conditions;
    std::vector<cond_node> children;
    bool is_and;

    // after evaluation of btree conditions we generate a IX vector
    // if AND_NODE THEN -> if (IX not in vector) validaion = FALSE else continue validation
    // if OR_NODE THEN -> if (IX in vector) validation = TRUE else continue validation
    std::unordered_set<int64_t> tree_ixs;
};

struct link_object
{
    bool is_direct_link;        // VirtualLink -> false, Link -> true
    int link_field_index;       // the index of the field in the parent table
    int child_link_field_index; // the index of the field in the child table (Link -> -1 - PK)

    // limit & offset get evaluated after query execution in subqueries
    size_t limit, offset;
};

struct query_object
{
    // entity data
    model_def mdef;

    // ordering
    vector_i field_indexes;
    std::vector<bool> order_asc;

    // filters
    cond_node filter_root;

    // limit + offset
    size_t limit, offset;

    // aggregation
    int agg_field_index;
    std::vector<aggregate_object> agg_vector;

    // linking
    std::vector<link_object> links;
    std::vector<query_object> link_vector;
    bool has_ix_constraints, is_subquery;
    std::unordered_set<int64_t> ix_constraints;

    // single field select
    int picked_index;

    bool validate_conditions(const int64_t record_id, char *record)
    {
        auto &current_node = filter_root;
        const bool valid = current_node.is_and
                               ? validate_conditions_every(record_id, record, current_node)
                               : validate_conditions_some(record_id, record, current_node);

        return valid;
    }

    bool validate_conditions_every(const int64_t record_id, char *record, cond_node &cnode)
    {
        if (cnode.btree_conditions.size() > 0 && cnode.tree_ixs.count(record_id) == 0)
            return false;

        for (auto &cond : cnode.conditions)
        {
            const bool valid = cond.validate(mdef, record);
            if (!valid)
                return false;
        }

        for (auto &child : cnode.children)
        {
            const bool valid = child.is_and
                                   ? validate_conditions_every(record_id, record, child)
                                   : validate_conditions_some(record_id, record, child);

            if (!valid)
                return false;
        }

        return true;
    }

    bool validate_conditions_some(const int64_t record_id, char *record, cond_node &cnode)
    {
        if (cnode.btree_conditions.size() > 0 && cnode.tree_ixs.count(record_id) > 0)
            return true;

        for (auto &cond : cnode.conditions)
        {
            const bool valid = cond.validate(mdef, record);
            if (valid)
                return true;
        }

        for (auto &child : cnode.children)
        {
            const bool valid = child.is_and
                                   ? validate_conditions_every(record_id, record, child)
                                   : validate_conditions_some(record_id, record, child);

            if (valid)
                return true;
        }

        return false;
    }

    void compare_and_swap(char *record, std::vector<py::object> &current)
    {
        size_t len = agg_vector.size();
        for (size_t i = 0; i < len; i++)
        {
            const AGGREGATE_OPERATION agg_func = static_cast<AGGREGATE_OPERATION>(agg_vector[i].option);
            if (agg_func == COUNT)
            {
                int64_t current_value = py::cast<int64_t>(current[i]);
                current[i] = py::cast(current_value + 1);
                continue;
            }

            const int idx = agg_vector[i].field_index;
            const int offset = mdef.field_offsets[idx];
            const FIELD_TYPE type = static_cast<FIELD_TYPE>(mdef.field_types[idx]);

            char *field_ptr = record + offset;
            switch (type)
            {
            case INT:
            case DATETIME:
            case LINK:
            {
                const int64_t int_value = *reinterpret_cast<int64_t *>(field_ptr);
                const int64_t current_value = py::cast<int64_t>(current[i]);
                switch (agg_func)
                {
                case MIN:
                    current[i] = py::cast(std::min(int_value, current_value));
                    break;

                case MAX:
                    current[i] = py::cast(std::max(int_value, current_value));
                    break;

                case SUM:
                    current[i] = py::cast(int_value + current_value);
                    break;

                case COUNT:
                    break;
                }
                break;
            }

            case DOUBLE:
            {
                const double double_value = *reinterpret_cast<double *>(field_ptr);
                const double current_value = py::cast<double>(current[i]);
                switch (agg_func)
                {
                case MIN:
                    current[i] = py::cast(std::min(double_value, current_value));
                    break;

                case MAX:
                    current[i] = py::cast(std::max(double_value, current_value));
                    break;

                case SUM:
                {
                    double new_sum = double_value + current_value;
                    current[i] = py::cast(new_sum);
                    break;
                }
                case COUNT:
                    break;
                }
                break;
            }

            default:
                throw std::runtime_error("Invalid aggregate type!");
                break;
            }
        }
    }

    std::vector<py::object> make_agg_tuple()
    {
        std::vector<py::object> agg_tuple(agg_vector.size());
        for (size_t i = 0; i < agg_vector.size(); i++)
        {
            const int idx = agg_vector[i].field_index;
            const FIELD_TYPE type = idx != -1 ? static_cast<FIELD_TYPE>(mdef.field_types[idx]) : INT;
            switch (agg_vector[i].option)
            {
            case SUM:
            case COUNT:
                if (type == DOUBLE)
                    agg_tuple[i] = py::cast(0.0);
                else
                    agg_tuple[i] = py::cast(0);

                break;

            case MIN:
                if (type == DOUBLE)
                    agg_tuple[i] = py::cast(std::numeric_limits<double>::max());
                else
                    agg_tuple[i] = py::cast(std::numeric_limits<int64_t>::max());

                break;

            case MAX:
                if (type == DOUBLE)
                    agg_tuple[i] = py::cast(std::numeric_limits<double>::min());
                else
                    agg_tuple[i] = py::cast(std::numeric_limits<int64_t>::min());

                break;

            default:
                throw std::runtime_error("Invalid aggregation functions was used!");
                break;
            }
        }

        return agg_tuple;
    }

    bool operator()(char *a, char *b) const
    {
        size_t len = field_indexes.size();
        for (size_t i = 0; i < len; i++)
        {
            const int idx = field_indexes[i];
            const bool not_pk = idx != -1;

            // if idx equals -1 it means the column is PK (primary key - id)
            const int offset = not_pk ? mdef.field_offsets[idx] : mdef.record_size;
            const int size = not_pk ? mdef.field_sizes[idx] : IX_SIZE;
            const FIELD_TYPE type = not_pk ? static_cast<FIELD_TYPE>(mdef.field_types[idx]) : INT;

            char *val_a = a + offset;
            char *val_b = b + offset;

            int cmp_res = 0;
            switch (type)
            {
            case INT:
            case DATETIME:
            case LINK:
            {
                const int64_t int_a = *reinterpret_cast<int64_t *>(val_a);
                const int64_t int_b = *reinterpret_cast<int64_t *>(val_b);
                cmp_res = int_a - int_b;
                break;
            }
            case STRING:
            {
                cmp_res = std::memcmp(val_a, val_b, size);
                break;
            }
            case BOOL:
            {
                const bool bool_a = *reinterpret_cast<bool *>(val_a);
                const bool bool_b = *reinterpret_cast<bool *>(val_b);
                cmp_res = bool_a - bool_b;
                break;
            }
            case DOUBLE:
            {
                const double double_a = *reinterpret_cast<double *>(val_a);
                const double double_b = *reinterpret_cast<double *>(val_b);
                cmp_res = double_a - double_b;
                break;
            }
            case VIRTUAL_LINK:
                throw std::runtime_error("Cannot use VirtualLink as comperator!");
                break;

            default:
                throw std::runtime_error("Invalid comperator type!");
                break;
            }

            if (!order_asc[i])
            {
                // order desc
                cmp_res = -cmp_res;
            }

            if (cmp_res != 0)
            {
                // values are not equal
                return cmp_res < 0;
            }
        }

        return false;
    }
};

template <typename... Args>
std::string message(Args &&...args)
{
    std::ostringstream oss;
    using List = int[];
    (void)List{0, ((void)(oss << std::forward<Args>(args)), 0)...};
    return oss.str();
}

inline std::vector<py::object> PYTHNOIZE_RECORD(const model_def &mdef, const std::vector<char *> bin_values)
{
    const auto &field_types = mdef.field_types;
    const size_t fields_count = field_types.size();
    std::vector<py::object> record(fields_count);
    for (size_t i = 0; i < fields_count; i++)
    {
        int64_t int_val;
        bool bool_val;
        std::string str_val;
        double double_val;

        switch (field_types[i])
        {
        case INT:
        case DATETIME:
        case LINK:
            memcpy(&int_val, bin_values[i], sizeof(int64_t));
            record[i] = py::cast(int_val);
            // std::cout << "int value  " << int_val << std::endl;
            break;

        case STRING:
            str_val = bin_values[i];
            record[i] = py::cast(str_val);
            // std::cout << "str value  " << str_val << " length " << str_val.length() << std::endl;
            break;

        case BOOL:
            memcpy(&bool_val, bin_values[i], sizeof(bool));
            record[i] = py::cast(bool_val);
            // std::cout << "bool value " << bool_val << std::endl;
            break;

        case DOUBLE:
            memcpy(&double_val, bin_values[i], sizeof(double));
            record[i] = py::cast(double_val);
            break;

        case VIRTUAL_LINK:
            // virtual link initializes to an empty list
            record[i] = py::list();
            break;

        default:
            std::string msg = message("Unknwon type was passed at index ", i, " type: ", field_types[i]);
            throw std::runtime_error(msg);
        }
    }

    return record;
}

inline std::vector<char *> PARSE_RECORD(const model_def &mdef, const py::list &py_values)
{
    const auto &field_types = mdef.field_types;
    const auto &field_sizes = mdef.field_sizes;

    const size_t fields_count = field_types.size();
    std::vector<char *> parsed_values(fields_count);
    for (size_t i = 0; i < fields_count; i++)
    {
        int64_t int_val;
        bool bool_val;
        std::string str_val;
        double double_val;
        parsed_values[i] = new char[field_sizes[i]];

        switch (field_types[i])
        {
        case INT:
        case DATETIME:
        case LINK:
            int_val = py::cast<int64_t>(py_values[i]);
            memcpy(parsed_values[i], &int_val, sizeof(int64_t));
            break;

        case STRING:
            str_val = py::cast<std::string>(py_values[i]);
            if (static_cast<int>(str_val.length()) > field_sizes[i])
            {
                std::string msg = message("Field exceeds max length ", field_sizes[i], " (", str_val, ")");
                throw std::runtime_error(msg);
            }
            strncpy(parsed_values[i], str_val.c_str(), field_sizes[i]);
            break;

        case BOOL:
            int_val = py::cast<int64_t>(py_values[i]);
            bool_val = int_val != 0;
            memcpy(parsed_values[i], &bool_val, sizeof(bool));
            break;

        case DOUBLE:
            double_val = py::cast<double>(py_values[i]);
            memcpy(parsed_values[i], &double_val, sizeof(double));
            break;

        case VIRTUAL_LINK:
            // skip virtual links when storing records
            break;

        default:
            std::string msg = message("Unknwon type was passed at index ", i, " type: ", field_types[i]);
            throw std::runtime_error(msg);
        }
    }
    return parsed_values;
}

inline void DEALLOCATE_RECORD(std::vector<char *> record_values)
{
    for (auto &heap_allocated_val : record_values)
    {
        delete[] heap_allocated_val;
    }
}

#endif // PARSER_HPP
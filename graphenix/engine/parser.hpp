#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <regex>

#include <pybind11/pybind11.h>

#define IX_SIZE 8 // size of 8 bytes <==> IX_SIZE
#define PK_IX_HEAD_SIZE IX_SIZE * 2
#define CHUNK_SIZE 10 * 1024 * 1024
#define ID_KEY "_id"

#define MIN_CLUSTER_SIZE 3
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
    DOUBLE = 5
};

enum FILTER_OPERATION_TYPE
{
    EQUAL = 0,
    NOTEQUAL = 1,
    GREATER = 2,
    GREATER_OR_EQUAL = 3,
    LESS = 4,
    LESS_OR_EQUAL = 5,
    REGEX = 6
};

struct model_def
{
    std::string db_name;
    std::string model_name;
    vector_i field_sizes;
    vector_i field_types;
    vector_i field_offsets;
    std::vector<bool> field_indexes;
    std::vector<std::string> field_names;
    int64_t record_size;
};

struct cond_object
{
    std::string field_name;
    int operation_index;
    py::object value;

    bool validate(model_def &mdef, char *record)
    {
        int idx = -1;
        for (size_t i = 0; i < mdef.field_names.size(); ++i)
            if (mdef.field_names[i] == field_name)
                idx = static_cast<int>(i);
        
        const bool not_pk = idx != -1;
        const int offset = not_pk ? mdef.field_offsets[idx] : mdef.record_size;
        const int size = not_pk ? mdef.field_sizes[idx] : IX_SIZE;
        const FIELD_TYPE type = not_pk ? static_cast<FIELD_TYPE>(mdef.field_types[idx]) : INT;
        char *cmp_field = record + offset;

        int cmp_res = 0;
        switch (type)
        {
        case INT:
        case DATETIME:
        {
            const int64_t int_a = *reinterpret_cast<int64_t *>(cmp_field);
            const int64_t int_b = py::cast<int64_t>(value);
            cmp_res = int_a - int_b;
            break;
        }
        case STRING:
        {
            py::str py_str = value;
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

            size_t cmp_size = std::min(std::strlen(cmp_field), val_b.size());
            cmp_res = std::memcmp(cmp_field, val_b.c_str(), cmp_size);
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
        default:
            throw std::runtime_error("Invalid comperator type!");
            break;
        }

        std::cout << "Idx " << idx << " offset " << offset << " rec " << cmp_field << " " << value << std::endl;
        std::cout << "res " << cmp_res << std::endl;
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
    std::vector<cond_object> conditions;
    std::vector<cond_node> children;
    bool is_and;
};

struct query_object
{
    // entity data
    model_def mdef;

    // order data
    vector_i field_indexes;
    std::vector<bool> order_asc;

    // filters
    cond_node filter_root;

    // limit + offset
    size_t limit, offset;

    bool validate_conditions(char *record)
    {
        auto &current_node = filter_root;
        const bool valid = current_node.is_and
                               ? validate_conditions_every(record, current_node)
                               : validate_conditions_some(record, current_node);

        return valid;
    }

    bool validate_conditions_every(char *record, cond_node &cnode)
    {
        for (auto &cond : cnode.conditions)
        {
            const bool valid = cond.validate(mdef, record);
            if (!valid)
                return false;
        }

        for (auto &child : cnode.children)
        {
            const bool valid = child.is_and
                                   ? validate_conditions_every(record, child)
                                   : validate_conditions_some(record, child);

            if (!valid)
                return false;
        }

        return true;
    }

    bool validate_conditions_some(char *record, cond_node &cnode)
    {
        for (auto &cond : cnode.conditions)
        {
            const bool valid = cond.validate(mdef, record);
            if (valid)
                return true;
        }

        for (auto &child : cnode.children)
        {
            const bool valid = child.is_and
                                   ? validate_conditions_every(record, child)
                                   : validate_conditions_some(record, child);

            if (valid)
                return true;
        }

        return false;
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
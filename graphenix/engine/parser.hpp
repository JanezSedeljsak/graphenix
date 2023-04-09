#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>

#include <pybind11/pybind11.h>

enum FIELD_TYPE
{
    INT = 0,
    STRING = 1,
    BOOL = 2,
    DATETIME = 3,
    LINK_SINGLE = 4
};

namespace py = pybind11;

template <typename... Args>
std::string message(Args &&...args)
{
    std::ostringstream oss;
    using List = int[];
    (void)List{0, ((void)(oss << std::forward<Args>(args)), 0)...};
    return oss.str();
}

inline std::vector<py::object> PYTHNOIZE_RECORD(const std::vector<char *> bin_values, const std::vector<int> &field_types,
                                                const std::vector<int> &field_lengths)
{
    const size_t fields_count = field_types.size();
    std::vector<py::object> record(fields_count);
    for (size_t i = 0; i < fields_count; i++)
    {
        int64_t int_val;
        bool bool_val;
        std::string str_val;

        switch (field_types[i])
        {
        case INT:
        case DATETIME:
        case LINK_SINGLE:
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

        default:
            std::string msg = message("Unknwon type was passed at index ", i, " type: ", field_types[i]);
            throw std::runtime_error(msg);
        }
    }

    return record;
}

inline std::vector<char *> PARSE_RECORD(const py::list &py_values, const std::vector<int> &field_types,
                                        const std::vector<int> &field_lengths)
{
    const size_t fields_count = field_types.size();
    std::vector<char *> parsed_values(fields_count);
    for (size_t i = 0; i < fields_count; i++)
    {
        int64_t int_val;
        bool bool_val;
        std::string str_val;
        parsed_values[i] = new char[field_lengths[i]];

        switch (field_types[i])
        {
        case INT:
        case DATETIME:
        case LINK_SINGLE:
            int_val = py::cast<int64_t>(py_values[i]);
            memcpy(parsed_values[i], &int_val, sizeof(int64_t));
            break;

        case STRING:
            str_val = py::cast<std::string>(py_values[i]);
            if (static_cast<int>(str_val.length()) > field_lengths[i])
            {
                std::string msg = message("Field exceeds max length ", field_lengths[i], " (", str_val, ")");
                throw std::runtime_error(msg);
            }
            strncpy(parsed_values[i], str_val.c_str(), field_lengths[i]);
            break;

        case BOOL:
            int_val = py::cast<int64_t>(py_values[i]);
            bool_val = int_val != 0;
            memcpy(parsed_values[i], &bool_val, sizeof(bool));
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
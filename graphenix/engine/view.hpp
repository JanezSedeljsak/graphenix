#ifndef VIEW_HPP
#define VIEW_HPP

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <pybind11/pybind11.h>

struct View;

struct Record
{
    const View *view;
    py::tuple record;

    py::object attr(const std::string &field_name) const;
    py::tuple as_tuple() const;
    py::dict as_dict() const;
    py::str as_str() const;
    View get_view() const;

    static Record make_record(View *view, const py::tuple &record);
};

struct View
{
    std::string model_name;
    std::vector<Record> records;
    std::vector<std::string> field_names;

    Record at(size_t idx) const
    {
        if (idx < 0 || idx > size())
            throw std::runtime_error("View index out of bounds");

        return records[idx];
    }

    std::vector<py::dict> as_dict() const
    {
        std::vector<py::dict> result;
        for (const auto &record : records)
            result.push_back(record.as_dict());

        return result;
    }

    std::vector<py::tuple> as_tuple() const
    {
        std::vector<py::tuple> result;
        for (const auto &record : records)
            result.push_back(record.as_tuple());

        return result;
    }

    size_t size() const
    {
        return records.size();
    }

    static View make_view(const std::vector<std::string> &fieldnames,
                          const std::vector<py::tuple> &rows,
                          const std::vector<int> date_indexes,
                          const std::string &modelname)
    {
        View *view_instance = new View;
        const size_t result_size = rows.size();
        view_instance->model_name = modelname;
        view_instance->field_names = std::vector<std::string>(fieldnames);
        view_instance->field_names.insert(view_instance->field_names.begin(), "id");
        view_instance->records.resize(result_size);
        py::module datetime_module = py::module::import("datetime");

        for (size_t i = 0; i < result_size; i++)
        {
            // check if any fields are dates and convert
            for (const auto &dindex : date_indexes)
                rows[i][dindex + 1] = datetime_module.attr("datetime").attr("fromtimestamp")(rows[i][dindex + 1]);

            Record record_instance = Record::make_record(view_instance, rows[i]);
            view_instance->records[i] = record_instance;
        }

        return *view_instance;
    }

    static View make_empty(const std::vector<std::string> &fieldnames, const std::string &modelname)
    {
        View *view_instance = new View;
        view_instance->model_name = modelname;
        view_instance->field_names = std::vector<std::string>(fieldnames);
        view_instance->field_names.insert(view_instance->field_names.begin(), "id");
        view_instance->records = std::vector<Record>();
        return *view_instance;
    }
};

struct RecordView
{
    std::string model_name;
    std::vector<std::string> field_names;
    py::tuple record;

    py::object attr(const std::string &field_name) const;
    py::tuple as_tuple() const;
    py::dict as_dict() const;
    py::str as_str() const;

    static RecordView from_view(const View &view);
};

py::object Record::attr(const std::string &field_name) const
{
    const View &view_ref = *view;
    auto it = std::find(view_ref.field_names.begin(), view_ref.field_names.end(), field_name);
    if (it == view_ref.field_names.end())
        throw std::runtime_error("Field name not found in View.");

    size_t idx = std::distance(view_ref.field_names.begin(), it);
    return record[idx];
}

py::object RecordView::attr(const std::string &field_name) const
{
    auto it = std::find(field_names.begin(), field_names.end(), field_name);
    if (it == field_names.end())
        throw std::runtime_error("Field name not found in View.");

    size_t idx = std::distance(field_names.begin(), it);
    return record[idx];
}

py::tuple Record::as_tuple() const
{
    return record;
}

py::tuple RecordView::as_tuple() const
{
    return record;
}

py::dict Record::as_dict() const
{
    py::dict result;
    for (size_t i = 0; i < record.size(); ++i)
    {
        py::str key = py::cast(view->field_names[i]);
        result[key] = record[i];
    }

    return result;
}

py::dict RecordView::as_dict() const
{
    py::dict result;
    for (size_t i = 0; i < record.size(); ++i)
    {
        py::str key = py::cast(field_names[i]);
        result[key] = record[i];
    }

    return result;
}

py::str Record::as_str() const
{
    std::stringstream ss;
    ss << view->model_name << "(";

    for (size_t i = 0; i < view->field_names.size(); ++i)
    {
        const py::str py_str = py::cast<py::str>(record[i]);
        const std::string str = py::cast<std::string>(py_str);
        ss << view->field_names[i] << "=" << str;
        if (i < view->field_names.size() - 1)
            ss << ", ";
    }

    ss << ")";
    return ss.str();
}

py::str RecordView::as_str() const
{
    std::stringstream ss;
    ss << model_name << "(";

    for (size_t i = 0; i < field_names.size(); ++i)
    {
        const py::str py_str = py::cast<py::str>(record[i]);
        const std::string str = py::cast<std::string>(py_str);
        ss << field_names[i] << "=" << str;
        if (i < field_names.size() - 1)
            ss << ", ";
    }

    ss << ")";
    return ss.str();
}

View Record::get_view() const
{
    return *view;
}

Record Record::make_record(View *view, const py::tuple &record)
{
    Record *record_instance = new Record;
    record_instance->view = view;
    record_instance->record = record;

    return *record_instance;
}

RecordView RecordView::from_view(const View &view)
{
    RecordView *recordview_instance = new RecordView;
    recordview_instance->model_name = view.model_name;
    recordview_instance->field_names = std::vector<std::string>(view.field_names);
    recordview_instance->record = view.records[0].record;

    return *recordview_instance;
}

#endif // VIEW_HPP
#ifndef UTIL
#define UTIL

#include <iostream>
#include <string>
#include "parser.hpp"

#ifdef _WIN32
#include <direct.h>
#define MAKE_GRAPHENIX_DB_DIR() _mkdir("graphenix_db")
#else
#include <sys/stat.h>
#define MAKE_GRAPHENIX_DB_DIR() mkdir("graphenix_db", 0777)
#endif

using namespace std;

inline string get_db_path(const string &schema_name)
{
    return "graphenix_db/" + schema_name;
}

inline string get_file_name(const string &schema_name, const string &model_name)
{
    // eg. school/students.bin;
    return "graphenix_db/" + schema_name + "/" + model_name + ".bin";
}

inline string get_ix_file_name(const string &schema_name, const string &model_name)
{
    // eg. school/ix_students.bin; <- primary key index
    return "graphenix_db/" + schema_name + "/ix_" + model_name + ".bin";
}

inline string get_field_ix_file_name(const string &schema_name, const string &model_name, const string &field_name)
{
    // eg. school/fix_students_birthdate.bin; <- field search tree
    return "graphenix_db/" + schema_name + "/fix_" + model_name + "_" + field_name + ".bin";
}

int64_t get_record_offset(const int64_t record_id, fstream &ix_file)
{
    const int64_t position = record_id * IX_SIZE + PK_IX_HEAD_SIZE;
    ix_file.clear();
    ix_file.seekg(0, ios::end);

    const int64_t file_size = ix_file.tellg();
    // cout << "file size: " << file_size << " position " << position << " rec id " << record_id << endl;
    if (position >= file_size)
    {
        int64_t last_id = file_size > static_cast<int64_t>(PK_IX_HEAD_SIZE) ? (file_size / IX_SIZE - 3) : -1;
        string msg = last_id > -1
                         ? message("Record ID (", record_id, ") is out of range!\nLast inserted: ", last_id)
                         : message("No records exist in the table yet");

        throw runtime_error(msg);
    }

    ix_file.seekg(position, ios::beg);
    int64_t ix;
    ix_file.read(reinterpret_cast<char *>(&ix), IX_SIZE);

    return ix;
}

void set_record_inactive(const int64_t record_id, fstream &ix_file)
{
    // cout << "Set record inactive: " << record_id << endl;
    ix_file.seekp(record_id * IX_SIZE + PK_IX_HEAD_SIZE, ios::beg);
    int64_t inactive_status = -1;
    ix_file.write(reinterpret_cast<const char *>(&inactive_status), IX_SIZE);
}

inline std::vector<std::vector<std::pair<int64_t, int64_t>>> clusterify(std::vector<std::pair<int64_t, int64_t>> &offsets)
{
    std::vector<std::vector<std::pair<int64_t, int64_t>>> clusters;
    std::vector<std::pair<int64_t, int64_t>> current_cluster;
    current_cluster.emplace_back(-1, -1); // Dummy point for initialization

    const size_t n = offsets.size();
    for (size_t i = 0; i < n; ++i)
    {
        const auto &offset = offsets[i];
        const int64_t diff = offset.first - current_cluster.back().first;

        if (diff <= MAX_CLUSTER_SIZE)
            current_cluster.push_back(offset);
        else
        {
            if (current_cluster.size() >= MIN_CLUSTER_SIZE)
                clusters.emplace_back(std::move(current_cluster));
            else
            {
                for (const auto &record : current_cluster)
                {
                    clusters.emplace_back(std::vector<std::pair<int64_t, int64_t>>{record});
                }
            }
            current_cluster.clear();
            current_cluster.push_back(offset);
        }
    }

    if (current_cluster.size() >= MIN_CLUSTER_SIZE)
        clusters.emplace_back(std::move(current_cluster));
    else
    {
        for (const auto &record : current_cluster)
        {
            clusters.emplace_back(std::vector<std::pair<int64_t, int64_t>>{record});
        }
    }

    return clusters;
}

template <typename T>
unordered_set<T> make_set_union(const unordered_set<T> &set1, const unordered_set<T> &set2)
{
    unordered_set<T> union_set(set1);
    union_set.insert(set2.begin(), set2.end());
    return union_set;
}

template <typename T>
unordered_set<T> make_set_intersection(const unordered_set<T> &set1, const unordered_set<T> &set2)
{
    unordered_set<T> intersection_set;
    if (set1.size() > set2.size())
    {
        for (const auto &element : set2)
            if (set1.count(element) != 0)
                intersection_set.insert(element);
    }
    else
    {
        for (const auto &element : set1)
            if (set2.count(element) != 0)
                intersection_set.insert(element);
    }

    return intersection_set;
}

#endif
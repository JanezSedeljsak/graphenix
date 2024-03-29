#ifndef BPTREENODE_HPP
#define BPTREENODE_HPP

#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>
#include <memory>

#define BLOCK_SIZE 1024
#define IX_SIZE 8 // size of 8 bytes <==> IX_SIZE
#define NODE_METADATA_SIZE IX_SIZE * 4;

using namespace std;

template <typename T>
class BPTreeNode
{
    static_assert(is_same<T, string>::value ||
                      is_same<T, int64_t>::value ||
                      is_same<T, double>::value,
                  "T must be string / int64_t / double");

public:
    int64_t offset;
    bool is_leaf, is_cached;
    vector<T> keys;
    vector<int64_t> children, data;
    vector<shared_ptr<BPTreeNode<T>>> actual_children;
    int64_t prev, next;
    int key_size;

    // inline void deallocate_children()
    // {
    //     // children are stored on the heap and thus need to be deallocated manually
    //     for (auto &child : actual_children)
    //     {
    //         child->deallocate_children();
    //         delete child;
    //     }
    //     actual_children.clear();
    // }

    inline void flush()
    {
        is_cached = false;
        is_leaf = true;
        next = -1;
        prev = -1;
        keys.clear();
        data.clear();
        children.clear();
        actual_children.clear();
        // deallocate_children();
    }

    inline BPTreeNode<T> *get_from_offset(fstream &ix_file, int offset)
    {
        BPTreeNode<T> *child = new BPTreeNode<T>(offset, key_size);
        child->read(ix_file);
        return child;
    }

    int64_t get_capacity()
    {
#ifndef FIXED_CAPACITY
        int64_t data_size = BLOCK_SIZE - NODE_METADATA_SIZE;
        int pair_size = IX_SIZE + key_size;
        return data_size / pair_size;
#else
        return FIXED_CAPACITY;
#endif
    }

    BPTreeNode(int64_t _offset, int size)
    {
        flush();
        key_size = size;
        offset = _offset;
    }

    void set_prev(shared_ptr<BPTreeNode<T>> node)
    {
        prev = node->offset;
    }

    void set_next(shared_ptr<BPTreeNode<T>> node)
    {
        next = node->offset;
    }

    void pop_item(T key, int64_t record_id)
    {
        auto itter = std::find(keys.begin(), keys.end(), key);
        if (itter != keys.end())
        {
            const size_t data_idx = std::distance(keys.begin(), itter);
            if (data_idx < data.size() && data[data_idx] == record_id)
            {
                keys.erase(itter);
                data.erase(data.begin() + data_idx);
            }
        }
    }

    void write(fstream &ix_file)
    {
        ix_file.seekp(offset, ios::beg);
        size_t num_keys = keys.size();
        char *buffer = new char[BLOCK_SIZE];
        memset(buffer, -1, BLOCK_SIZE);
        char *buffer_ptr = buffer;
        memcpy(buffer_ptr, reinterpret_cast<const char *>(&num_keys), IX_SIZE);
        memcpy(buffer_ptr + IX_SIZE, reinterpret_cast<const char *>(&is_leaf), IX_SIZE);
        memcpy(buffer_ptr + 2 * IX_SIZE, reinterpret_cast<const char *>(&prev), IX_SIZE);
        memcpy(buffer_ptr + 3 * IX_SIZE, reinterpret_cast<const char *>(&next), IX_SIZE);
        buffer_ptr += NODE_METADATA_SIZE;

        for (const auto &key : keys)
        {
            if constexpr (is_same_v<T, string>)
                memcpy(buffer_ptr, key.c_str(), key_size);
            else
                memcpy(buffer_ptr, reinterpret_cast<const char *>(&key), key_size);

            buffer_ptr += key_size;
        }

        if (is_leaf)
        {
            for (const auto &item : data)
            {
                memcpy(buffer_ptr, reinterpret_cast<const char *>(&item), IX_SIZE);
                buffer_ptr += IX_SIZE;
            }
        }
        else
        {
            for (const auto &child : children)
            {
                memcpy(buffer_ptr, reinterpret_cast<const char *>(&child), IX_SIZE);
                buffer_ptr += IX_SIZE;
            }
        }

        ix_file.write(buffer, BLOCK_SIZE);
        delete[] buffer;
    }

    shared_ptr<BPTreeNode<T>> get_prev(fstream &ix_file)
    {
        return prev != -1
                   ? shared_ptr<BPTreeNode<T>>(get_from_offset(ix_file, prev))
                   : nullptr;
    }

    shared_ptr<BPTreeNode<T>> get_next(fstream &ix_file)
    {
        return next != -1
                   ? shared_ptr<BPTreeNode<T>>(get_from_offset(ix_file, next))
                   : nullptr;
    }

    /**
     * @brief This method is a simple DFS to read the tree into memory
     * @param ix_file : stream for reading the file (shouldn't be closed while reading)
     */
    void read_recursive(fstream &ix_file)
    {
        read(ix_file); // read current
        is_cached = true;
        actual_children.clear();
        for (const auto &offset : children)
        {
            if (offset != -1)
            {
                BPTreeNode<T> *temp = get_from_offset(ix_file, offset);
                actual_children.push_back(shared_ptr<BPTreeNode<T>>(temp));
                temp->read_recursive(ix_file);
            }
        }
    }

    void read(fstream &ix_file)
    {
        flush();
        ix_file.seekg(offset, ios::beg);
        char *buffer = new char[BLOCK_SIZE];
        ix_file.read(buffer, BLOCK_SIZE);

        int64_t num_keys = *reinterpret_cast<int64_t *>(buffer);
        is_leaf = *reinterpret_cast<bool *>(buffer + 1 * IX_SIZE);
        prev = *reinterpret_cast<int64_t *>(buffer + 2 * IX_SIZE);
        next = *reinterpret_cast<int64_t *>(buffer + 3 * IX_SIZE);

        // cout << "Read num keys: " << num_keys << endl;
        char *buffer_ptr = buffer + 4 * IX_SIZE;
        for (int i = 0; i < num_keys; i++)
        {
            if constexpr (is_same_v<T, string>)
            {
                string str_key(buffer_ptr, key_size);
                keys.push_back(str_key);
            }
            else if constexpr (is_same_v<T, int64_t>)
            {
                keys.push_back(*reinterpret_cast<int64_t *>(buffer_ptr));
            }
            else
            {
                keys.push_back(*reinterpret_cast<double *>(buffer_ptr));
            }

            buffer_ptr += key_size;
        }

        if (is_leaf)
            data.assign(
                reinterpret_cast<int64_t *>(buffer_ptr),
                reinterpret_cast<int64_t *>(buffer_ptr + num_keys * IX_SIZE));
        else
            children.assign(
                reinterpret_cast<int64_t *>(buffer_ptr),
                reinterpret_cast<int64_t *>(buffer_ptr + (num_keys + 1) * IX_SIZE));

        delete[] buffer;
    }

    void print(bool is_recursive, const size_t depth)
    {
        const std::string prefix(depth, '\t');
        cout << prefix << "Is leaf: " << is_leaf << endl;
        cout << prefix << "Offset: " << offset << endl;
        cout << prefix << "Children: " << children.size() << endl;
        cout << prefix << "Prev: " << prev << endl;
        cout << prefix << "Next: " << next << endl;
        for (size_t i = 0; i < keys.size(); i++)
        {
            cout << prefix << "Key: " << keys[i];
            if (is_leaf)
            {
                cout << ", " << data[i];
            }
            cout << endl;
        }

        cout << prefix << "-----------------" << endl;
        if (!is_recursive)
            return;

        for (auto &child : actual_children)
            child->print(true, depth + 1);
    }
};

#endif // BPTREENODE_HPP
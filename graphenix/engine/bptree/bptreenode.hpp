#include <vector>
#include <iostream>
#include <cstring>
#include <fstream>

#define BLOCK_SIZE 1024
#define IX_SIZE 8 // size of 8 bytes <==> IX_SIZE

using namespace std;

class BPTreeNode
{
public:

    inline void flush()
    {
        is_leaf = true;
        offset = 0;
        next = -1;
        prev = -1;
        keys.clear();
        data.clear();
        children.clear();
        actual_children.clear();
    }

    int64_t offset;
    bool is_leaf;
    vector<int64_t> keys;
    vector<int64_t> children, data;
    vector<BPTreeNode> actual_children;
    int64_t prev, next;

    BPTreeNode(int64_t _offset)
    {
        offset = _offset;
        flush();
    }

    void write(fstream &ix_file)
    {
        ix_file.seekp(offset, std::ios::beg);
        size_t num_keys = keys.size();
        char *buffer = new char[BLOCK_SIZE];
        memset(buffer, -1, BLOCK_SIZE);
        char *buffer_ptr = buffer;
        memcpy(buffer_ptr, reinterpret_cast<const char *>(&num_keys), IX_SIZE);
        memcpy(buffer_ptr + IX_SIZE, reinterpret_cast<const char *>(&is_leaf), IX_SIZE);
        memcpy(buffer_ptr + 2 * IX_SIZE, reinterpret_cast<const char *>(&prev), IX_SIZE);
        memcpy(buffer_ptr + 3 * IX_SIZE, reinterpret_cast<const char *>(&next), IX_SIZE);
        buffer_ptr += IX_SIZE * 4;
        for (const auto &key : keys)
        {
            memcpy(buffer_ptr, reinterpret_cast<const char *>(&key), IX_SIZE);
            buffer_ptr += IX_SIZE;
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

    BPTreeNode *get_nth_child(int idx)
    {
        int file_offset = children[idx];
        BPTreeNode *child = new BPTreeNode(file_offset);
        return child;
    }

    /**
     * @brief This method is a simple DFS to read the tree into memory
     * @param ix_file : stream for reading the file (shouldn't be closed while reading)
     */
    void read_recursive(fstream &ix_file)
    {
        read(ix_file); // read current
        actual_children.clear();
        for (const auto &child : children)
        {
            if (child != -1)
            {
                BPTreeNode temp(child); // create with offset
                temp.read(ix_file);
                actual_children.push_back(temp);
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
        is_leaf = *reinterpret_cast<int64_t *>(buffer + 1 * IX_SIZE);
        prev = *reinterpret_cast<int64_t *>(buffer + 2 * IX_SIZE);
        next = *reinterpret_cast<int64_t *>(buffer + 3 * IX_SIZE);

        cout << "Read num keys: " << num_keys << endl;
        char *buffer_ptr = buffer + 4 * IX_SIZE;
        keys.assign(reinterpret_cast<int64_t *>(buffer_ptr), reinterpret_cast<int64_t *>(buffer_ptr + num_keys * IX_SIZE));
        buffer_ptr += num_keys * IX_SIZE;
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

    void print(bool is_recursive)
    {
        cout << "Offset: " << offset << endl;
        cout << "Children: " << children.size() << endl;
        for (size_t i = 0; i < keys.size(); i++)
        {
            cout << "Node: " << keys[i];
            if (is_leaf)
            {
                cout << ", " << data[i];
            }
            cout << endl;
        }

        if (!is_recursive)
            return;

        for (auto &child : actual_children)
            child.print(true);
    }
};
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include "bptreenode.hpp"

using namespace std;

template <typename T>
class BPTreeIndex
{
public:
    typedef pair<pair<int, int>, shared_ptr<BPTreeNode<T>>> LeafMatchInterval;
    string ix_filename;
    shared_ptr<BPTreeNode<T>> root;
    int key_size;

    inline void init(const string &model_name, const string &field_name)
    {
        // when stable should use -> get_field_ix_file_name;
        ix_filename = "bpt_ix_" + model_name + "_" + field_name + ".bin";
        root = nullptr;
    }

    BPTreeIndex(string model_name, string field_name)
    {
        init(model_name, field_name);
        key_size = IX_SIZE;
    }

    BPTreeIndex(string model_name, string field_name, int size)
    {
        init(model_name, field_name);
        key_size = size;
    }

    static void print_nodes_with_intervals(const vector<LeafMatchInterval> &nodes)
    {
        for (const auto &node : nodes)
        {
            cout << "Start " << node.first.first << " End: " << node.first.second << endl;
            for (int i = node.first.first; i <= node.first.second; i++)
                cout << "Key: " << node.second->keys[i] << " Value: " << node.second->data[i] << endl;
        }
    }

    static unordered_set<int64_t> flatten_intervals_to_ptrs(const vector<LeafMatchInterval> &nodes)
    {
        unordered_set<int64_t> ptrs;
        for (const auto &node : nodes)
            for (int i = node.first.first; i <= node.first.second; i++)
                ptrs.insert(node.second->data[i]);

        return ptrs;
    }

    void create()
    {
        if (filesystem::exists(ix_filename))
            return;

        ofstream ix_file(ix_filename, ios::binary | ios::out);
        BPTreeNode<T> *node_ptr = new BPTreeNode<T>(2 * IX_SIZE, key_size);
        root = shared_ptr<BPTreeNode<T>>(node_ptr);

        int64_t head_ptr = -1, first_free = 2 * IX_SIZE;
        char buffer[2 * IX_SIZE];
        memcpy(buffer, reinterpret_cast<const char *>(&head_ptr), IX_SIZE);
        memcpy(buffer + IX_SIZE, reinterpret_cast<const char *>(&first_free), IX_SIZE);

        ix_file.seekp(0, ios::beg);
        ix_file.write(buffer, 2 * IX_SIZE);
        ix_file.close();
    }

    void write()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        ix_file.seekp(root->offset, ios::beg);
        root->write(ix_file);
        set_head_ptr(ix_file, root->offset);
        ix_file.close();
    }

    inline int64_t get_head_ptr(fstream &ix_file)
    {
        int64_t head_ptr;
        ix_file.seekg(0, ios::beg);
        ix_file.read(reinterpret_cast<char *>(&head_ptr), IX_SIZE);
        return head_ptr;
    }

    inline int64_t get_next_free(fstream &ix_file)
    {
        int64_t next_free;
        ix_file.seekg(IX_SIZE, ios::beg);
        ix_file.read(reinterpret_cast<char *>(&next_free), IX_SIZE);
        return next_free;
    }

    inline void set_head_ptr(fstream &ix_file, int64_t head_ptr)
    {
        ix_file.seekp(0, ios::beg);
        ix_file.write(reinterpret_cast<const char *>(&head_ptr), IX_SIZE);
    }

    inline void set_next_free(fstream &ix_file, int64_t next_free)
    {
        ix_file.seekp(IX_SIZE, ios::beg);
        ix_file.write(reinterpret_cast<const char *>(&next_free), IX_SIZE);
    }

    void read()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        root->offset = get_head_ptr(ix_file);
        root->read(ix_file);
        ix_file.close();
    }

    void load_full_tree()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        root->offset = get_head_ptr(ix_file);
        root->read_recursive(ix_file);
        ix_file.close();
    }

    void print_index(bool is_recursive)
    {
        if (root != nullptr)
            root->print(is_recursive);
    }

    void delete_index()
    {
        if (filesystem::exists(ix_filename))
            filesystem::remove(ix_filename);
    }

    inline bool generic_equal(const T &a, const T &b)
    {
        if constexpr (is_same_v<T, string>)
            return strcmp(a.c_str(), b.c_str()) == 0;
        else
            return a == b;
    }

    /**
     * @brief Returns the range[from, to] (interval) in the leaf node where the keys match
     *
     * @param search generic search parameter (the key we search by)
     * @param node the actual node that was read from the file (should be type is_leaf)
     * @param ix_file the reader object
     * @return vector<LeafMatchRange>
     */
    inline LeafMatchInterval search_leaf(const T &search, shared_ptr<BPTreeNode<T>> node, fstream &ix_file)
    {
        less<T> generic_less;
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (generic_equal(node->keys[mid], search))
            {
                int from = mid, to = mid;
                while (from > 0 && generic_equal(node->keys[from - 1], search))
                    from--;

                while (to < node->keys.size() - 1 && generic_equal(node->keys[to + 1], search))
                    to++;

                return make_pair(make_pair(from, to), node);
            }

            if (!generic_less(node->keys[mid], search))
                high = mid;
            else
                low = mid + 1;
        }

        return make_pair(make_pair(-1, -1), nullptr);
    }

    inline void extend_node_interval(vector<LeafMatchInterval> &nodes, const T &search, shared_ptr<BPTreeNode<T>> node, fstream &ix_file, const int64_t idx)
    {
        shared_ptr<BPTreeNode<T>> child;
        if (!node->is_cached)
        {
            BPTreeNode<T> *node_ptr = new BPTreeNode<T>(node->children[idx], key_size);
            child = shared_ptr<BPTreeNode<T>>(node_ptr);
            child->read(ix_file);
        }
        else
            child = node->actual_children[idx];

        if (child->is_leaf)
        {
            LeafMatchInterval found = search_leaf(search, child, ix_file);
            if (found.first.first != -1)
                nodes.push_back(found);
        }
        else
        {
            const auto &curr_nodes = search_internal(search, child, ix_file);
            nodes.insert(nodes.end(), curr_nodes.begin(), curr_nodes.end());
        }
    }

    /**
     * @brief Returns a vector of leaf nodes that match the specific value
     *
     * @param search
     * @param node
     * @param ix_file
     * @return vector<LeafMatchRange>
     */
    inline vector<LeafMatchInterval> search_internal(const T &search, shared_ptr<BPTreeNode<T>> node, fstream &ix_file)
    {
        vector<LeafMatchInterval> nodes;
        less<T> generic_less;
        bool found_equal = false;
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (generic_equal(node->keys[mid], search))
            {
                found_equal = true;
                break;
            }

            if (!generic_less(node->keys[mid], search))
                high = mid;
            else
                low = mid + 1;
        }

        if (!found_equal)
        {
            extend_node_interval(nodes, search, node, ix_file, low);
            return nodes;
        }

        int from = low, to = low;
        while (from > 0 && generic_equal(node->keys[from - 1], search))
            from--;

        while (to < node->keys.size() - 1 && generic_equal(node->keys[to + 1], search))
            to++;

        for (int i = from; i <= to; i++)
            extend_node_interval(nodes, search, node, ix_file, i);

        return nodes;
    }

    vector<LeafMatchInterval> find(const T &search)
    {
        if (root == nullptr)
            return {};

        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        if (!root->is_cached)
            root->read(ix_file);

        vector<LeafMatchInterval> nodes;
        if (root->is_leaf)
        {
            LeafMatchInterval found = search_leaf(search, root, ix_file);
            if (found.first.first != -1)
                nodes.push_back(found);
        }
        else
            nodes = search_internal(search, root, ix_file);

        ix_file.close();
        return nodes;
    }

    inline BPTreeNode<T> *insert_into_leaf(shared_ptr<BPTreeNode<T>> node, T &key, int64_t record_offset)
    {
    }

    inline BPTreeNode<T> *insert_into_internal(shared_ptr<BPTreeNode<T>> node, T &key, int64_t record_offset)
    {
    }

    BPTreeNode<T> *insert(T &key, int64_t record_offset)
    {
        if (root == nullptr)
            read(); // loads root from file

        if (root->is_leaf)
            insert_into_leaf(root, key, record_offset);
        else
            insert_into_internal(root, key, record_offset);
    }

    inline bool remove_from_leaf(shared_ptr<BPTreeNode<T>> node, T &key, int64_t record_offset)
    {
        return true;
    }

    inline bool remove_from_internal(shared_ptr<BPTreeNode<T>> node, T &key, int64_t record_offset)
    {
        return true;
    }

    bool remove(T &key, int64_t record_offset)
    {
        if (root == nullptr)
            read(); // loads root from file

        return root->is_leaf
                   ? delete_from_leaf(root, key, record_offset)
                   : delete_from_internal(root, key, record_offset);
    }
};
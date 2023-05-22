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
    typedef pair<pair<int, int>, BPTreeNode<T> *> LeafMatchInterval;
    string ix_filename;
    BPTreeNode<T> *root;
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
        ix_file.close();
        root = new BPTreeNode<T>(0, key_size);
        ix_file.close();
    }

    void write()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        root->write(ix_file);
        ix_file.close();
    }

    void read()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        root->read(ix_file);
        ix_file.close();
    }

    void load_full_tree()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
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
    inline LeafMatchInterval search_leaf(const T &search, BPTreeNode<T> *node, fstream &ix_file)
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

    /**
     * @brief Returns a vector of leaf nodes that match the specific value
     *
     * @param search
     * @param node
     * @param ix_file
     * @return vector<LeafMatchRange>
     */
    inline vector<LeafMatchInterval> search_internal(const T &search, BPTreeNode<T> *node, fstream &ix_file)
    {
        vector<LeafMatchInterval> nodes;
        less<T> generic_less;
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (generic_equal(node->keys[mid], search))
                break;

            if (!generic_less(node->keys[mid], search))
                high = mid;
            else
                low = mid + 1;
        }

        int from = low, to = low;
        while (from > 0 && generic_equal(node->keys[from - 1], search))
            from--;

        while (to < node->keys.size() - 1 && generic_equal(node->keys[to + 1], search))
            to++;

        for (int i = from; i <= to; i++)
        {
            unique_ptr<BPTreeNode<T>> child;
            if (!node->is_cached)
            {
                child = make_unique<BPTreeNode<T>>(node->children[i], key_size);
                child->read(ix_file);
            }
            else
            {
                child = move(node->actual_children[i]);
            }

            if (child->is_leaf)
                nodes.push_back(search_leaf(search, child.get(), ix_file));
            else
            {
                const auto &curr_nodes = search_internal(search, child.get(), ix_file);
                nodes.insert(nodes.end(), curr_nodes.begin(), curr_nodes.end());
            }

            if (!node->is_cached)
                node->actual_children[i] = move(child);
        }

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
            nodes.push_back(search_leaf(search, root, ix_file));
        else
            nodes = search_internal(search, root, ix_file);

        ix_file.close();
        return nodes;
    }
};
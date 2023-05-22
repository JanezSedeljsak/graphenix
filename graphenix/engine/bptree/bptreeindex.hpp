#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <fstream>
#include "bptreenode.hpp"

using namespace std;

template <typename T>
class BPTreeIndex
{
public:
    string ix_filename;
    BPTreeNode<T> *root;
    int key_size;

    inline void init(const string &model_name, const string &field_name)
    {
        // when stable should use -> get_field_ix_file_name;
        ix_filename = "bpt_ix_" + model_name + "_" + field_name + ".bin";
        root = NULL;
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
        if (root != NULL)
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

    inline pair<int, BPTreeNode<T> *> search_leaf(const T &search, BPTreeNode<T> *node, fstream &ix_file)
    {
        less<T> generic_less;
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (generic_equal(node->keys[mid], search))
                return make_pair(mid, node);

            if (!generic_less(node->keys[mid], search))
                high = mid;
            else
                low = mid + 1;
        }

        return make_pair<int, BPTreeNode<T> *>(-1, NULL);
    }

    inline pair<int, BPTreeNode<T> *> search_internal(const T &search, BPTreeNode<T> *node, fstream &ix_file)
    {
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

        unique_ptr<BPTreeNode<T>> child(new BPTreeNode<T>(node->children[low], key_size));
        child->read(ix_file);

        return child->is_leaf
                   ? search_leaf(search, child.get(), ix_file)
                   : search_internal(search, child.get(), ix_file);
    }

    pair<int, BPTreeNode<T> *> find(const T &search)
    {
        if (root == NULL)
            return make_pair<int, BPTreeNode<T> *>(-1, NULL);

        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        root->read(ix_file);
        pair<int, BPTreeNode<T> *> res = root->is_leaf
                                             ? search_leaf(search, root, ix_file)
                                             : search_internal(search, root, ix_file);

        ix_file.close();
        return res;
    }
};
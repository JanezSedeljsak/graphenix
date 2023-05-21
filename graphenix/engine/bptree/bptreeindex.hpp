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

    BPTreeIndex(string model_name, string field_name)
    {
        // when stable should use -> get_field_ix_file_name;
        ix_filename = "bpt_ix_" + model_name + "_" + field_name + ".bin";
        root = NULL;
    }

    void create()
    {
        if (filesystem::exists(ix_filename))
            return;

        ofstream ix_file(ix_filename, ios::binary | ios::out);
        ix_file.close();
        root = new BPTreeNode<T>(0);
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

    inline pair<int, BPTreeNode<T> *> search_leaf(int64_t search, BPTreeNode<T> *node, fstream &ix_file)
    {
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (node->keys[mid] == search)
                return make_pair(mid, node);

            if (node->keys[mid] > search)
                high = mid;
            else
                low = mid + 1;
        }

        return make_pair<int, BPTreeNode<T> *>(-1, NULL);
    }

    inline pair<int, BPTreeNode<T> *> search_internal(int64_t search, BPTreeNode<T> *node, std::fstream &ix_file)
    {
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (node->keys[mid] == search)
                break;

            if (node->keys[mid] > search)
                high = mid;
            else
                low = mid + 1;
        }

        unique_ptr<BPTreeNode<T>> child(new BPTreeNode<T>(node->children[low]));
        child->read(ix_file);

        return child->is_leaf
                   ? search_leaf(search, child.get(), ix_file)
                   : search_internal(search, child.get(), ix_file);
    }

    pair<int, BPTreeNode<T> *> find(T search)
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
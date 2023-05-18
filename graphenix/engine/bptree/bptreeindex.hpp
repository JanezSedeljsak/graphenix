#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <fstream>
#include "bptreenode.hpp"

using namespace std;

class BPTreeIndex
{
public:
    string ix_filename;
    BPTreeNode *root;

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
        root = new BPTreeNode(0);
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

    inline BPTreeNode *search_leaf(int64_t search, BPTreeNode *node, fstream &ix_file)
    {
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (node->keys[mid] == search)
                return node;

            if (node->keys[mid] > search)
                high = mid;
            else
                low = mid + 1;
        }

        return NULL;
    }

    inline BPTreeNode *search_internal(int64_t search, BPTreeNode *node, std::fstream &ix_file)
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

        unique_ptr<BPTreeNode> child(new BPTreeNode(node->children[low]));
        child->read(ix_file);

        return child->is_leaf 
            ? search_leaf(search, child.get(), ix_file) 
            : search_internal(search, child.get(), ix_file);
    }

    BPTreeNode *find(int64_t search)
    {
        if (root == NULL)
            return NULL;

        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        root->read(ix_file);
        BPTreeNode *node = !root->is_leaf
                               ? search_leaf(search, root, ix_file)
                               : search_internal(search, root, ix_file);

        ix_file.close();
        return node;
    }
};
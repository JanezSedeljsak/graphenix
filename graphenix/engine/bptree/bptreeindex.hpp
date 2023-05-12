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
    string ix_file;
    BPTreeNode *root;

    BPTreeIndex(string model_name, string field_name)
    {
        // when stable should use -> get_field_ix_file_name;
        ix_file = "bpt_ix_" + model_name + "_" + field_name + ".bin";
        root = NULL;
    }

    void create()
    {
        if (filesystem::exists(ix_file))
            return;

        ofstream ix_file_stream(ix_file, ios::binary | ios::out);
        ix_file_stream.close();
        root = new BPTreeNode(0);
    }

    void make()
    {
        fstream ix_file_stream(ix_file, ios::binary | ios::in | ios::out);
        root->write(ix_file_stream);
    }

    void read()
    {
        fstream ix_file_stream(ix_file, ios::binary | ios::in | ios::out);
        root->read(ix_file_stream);
    }

    void print_index()
    {
        if (root != NULL)
            root->print();
    }

    void delete_index()
    {
        if (filesystem::exists(ix_file))
            filesystem::remove(ix_file);
    }
};
#define IS_TESTING
#ifdef IS_TESTING

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define FIXED_CAPACITY 10
#include <vector>
#include "doctest.h"
#include "benchmark.h"
#include "../bptree/bptreeindex.hpp"
#include "test_helpers.hpp"

TEST_CASE("Crete integer index and search")
{
    BPTreeIndex<int64_t> bpt("__test", "user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.root->keys = {2, 4, 6, 7, 8, 10};
    bpt.root->data = {4, 8, 16, 32, 64, 128};
    bpt.write();
    CHECK(bpt.root->is_leaf == true);
    FIND_MULTIPLE(bpt);
    FIND_SINGLE(bpt);
    FIND_NONE(bpt);

    bpt.root->flush();
    bpt.read();
    FIND_MULTIPLE(bpt);
    FIND_SINGLE(bpt);
    FIND_NONE(bpt);

    bpt.delete_index();
}

TEST_CASE("Crete string index and search")
{
    BPTreeIndex<string> bpt("__test", "user", "uuid", 10);
    bpt.delete_index();
    bpt.create();
    bpt.root->keys = {"a", "ab", "ac", "ad", "fdsada", "fzaaaa"};
    bpt.root->data = {4, 8, 16, 32, 64, 256};
    bpt.write();
    CHECK(bpt.root->is_leaf == true);
    FIND_MULTIPLE_STR(bpt);
    FIND_SINGLE_STR(bpt);
    FIND_NONE_STR(bpt);

    bpt.root->flush();
    bpt.read();
    FIND_MULTIPLE_STR(bpt);
    FIND_SINGLE_STR(bpt);
    FIND_NONE_STR(bpt);

    bpt.delete_index();
}

TEST_CASE("Crete index, insert and do basic search")
{
    BPTreeIndex<int64_t> bpt("__test", "user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.root->keys = {2, 4, 6, 8, 8, 10};
    bpt.root->data = {4, 8, 16, 32, 64, 128};
    bpt.write();
    bpt.root->flush();
    // this creates a cached object if we only do read
    // the find will read the ix_file again and the inserted record won't exist
    bpt.load_full_tree();
    bpt.insert(3, 256);
    CHECK_IF_BASIC_INSERT_WORKED(bpt);

    bpt.write();
    bpt.root->flush();
    bpt.read();
    CHECK_IF_BASIC_INSERT_WORKED(bpt);

    bpt.delete_index();
}

#endif // is testing
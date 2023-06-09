#define IS_TESTING
#ifdef IS_TESTING

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define FIXED_CAPACITY 10
#include <vector>
#include "doctest.h"
#include "benchmark.h"
#include "../bptree/bptreeindex.hpp"

inline void FIND_MULTIPLE(BPTreeIndex<int64_t> &bpt)
{
    const auto &found = bpt.find(8);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 2);
    CHECK(flatten.count(32) > 0);
    CHECK(flatten.count(64) > 0);
}

inline void FIND_SINGLE(BPTreeIndex<int64_t> &bpt)
{
    const auto &found = bpt.find(10);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 1);
    CHECK(flatten.count(128) > 0);
}

inline void FIND_NONE(BPTreeIndex<int64_t> &bpt)
{
    const auto &found = bpt.find(42231);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 0);
}

TEST_CASE("Crete integer index and search")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.root->keys = {2, 4, 6, 8, 8, 10};
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

inline void FIND_MULTIPLE_STR(BPTreeIndex<string> &bpt)
{
    const auto &found = bpt.find("a");
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 3);
    CHECK(flatten.count(4) > 0);
    CHECK(flatten.count(8) > 0);
    CHECK(flatten.count(16) > 0);
}

inline void FIND_SINGLE_STR(BPTreeIndex<string> &bpt)
{
    const auto &found = bpt.find("fzaaaa");
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 1);
    CHECK(flatten.count(256) > 0);
}

inline void FIND_NONE_STR(BPTreeIndex<string> &bpt)
{
    const auto &found = bpt.find("fzaaaaa"); // one extra 'a' as the key
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 0);
}

TEST_CASE("Crete string index and search")
{
    BPTreeIndex<string> bpt("user", "uuid", 10);
    bpt.delete_index();
    bpt.create();
    bpt.root->keys = {"a", "a", "a", "ad", "fdsada", "fzaaaa"};
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

inline void CHECK_IF_BASIC_INSERT_WORKED(BPTreeIndex<int64_t> &bpt)
{
    CHECK(bpt.root->keys.size() == 7); // 6 existing + 1 inserted
    const auto &found = bpt.find(3);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.count(256) > 0);
}

TEST_CASE("Crete index, insert and do basic search")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
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
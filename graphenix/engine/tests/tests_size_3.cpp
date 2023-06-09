#define IS_TESTING
#ifdef IS_TESTING

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define FIXED_CAPACITY 3LL
#include <vector>
#include "doctest.h"
#include "benchmark.h"
#include "../bptree/bptreeindex.hpp"

TEST_CASE("Crete index, insert with create subtree")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.insert(1, 100);
    bpt.insert(4, 500);
    bpt.insert(0, 50);
    bpt.root->flush();
    bpt.load_full_tree();
    bpt.insert(3, 45);
    bpt.root->flush();
    bpt.load_full_tree();

    CHECK(bpt.root->keys.size() == 1);
    CHECK(bpt.root->keys[0] == 3);

    CHECK(bpt.root->actual_children[0]->keys[0] == 0);
    CHECK(bpt.root->actual_children[0]->data[0] == 50);
    CHECK(bpt.root->actual_children[0]->keys[1] == 1);
    CHECK(bpt.root->actual_children[0]->data[1] == 100);

    CHECK(bpt.root->actual_children[1]->keys[0] == 3);
    CHECK(bpt.root->actual_children[1]->data[0] == 45);
    CHECK(bpt.root->actual_children[1]->keys[1] == 4);
    CHECK(bpt.root->actual_children[1]->data[1] == 500);

    CHECK(bpt.root->actual_children[0]->next == bpt.root->actual_children[1]->offset);
    CHECK(bpt.root->actual_children[1]->prev == bpt.root->actual_children[0]->offset);
    bpt.delete_index();
}

TEST_CASE("Crete index, insert with create subtree")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.insert(1, 170);
    bpt.insert(3, 110);
    bpt.insert(2, 150);
    bpt.insert(1, 220);
    bpt.root->flush();
    bpt.load_full_tree();
    const auto &found = bpt.find(1);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 2);
    CHECK(flatten.count(170) > 0);
    CHECK(flatten.count(220) > 0);
    CHECK(flatten.count(150) == 0);
    bpt.delete_index();
}

#endif // is testing
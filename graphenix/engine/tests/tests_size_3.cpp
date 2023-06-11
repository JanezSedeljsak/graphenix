#define IS_TESTING
#ifdef IS_TESTING

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define FIXED_CAPACITY 3LL
#include <vector>
#include "doctest.h"
#include "benchmark.h"
#include "../bptree/bptreeindex.hpp"
#include "test_helpers.hpp"

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

TEST_CASE("Crete index, insert with create subtree 2")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.insert(1, 500);
    bpt.insert(2, 400);
    bpt.insert(3, 300);
    bpt.insert(4, 200);
    bpt.insert(5, 150);
    bpt.root->flush();
    bpt.load_full_tree();

    CHECK(bpt.root->keys.size() == 1);

    CHECK(bpt.root->actual_children[0]->keys[0] == 1);
    CHECK(bpt.root->actual_children[0]->data[0] == 500);
    CHECK(bpt.root->actual_children[0]->keys[1] == 2);
    CHECK(bpt.root->actual_children[0]->data[1] == 400);

    CHECK(bpt.root->actual_children[1]->keys[0] == 3);
    CHECK(bpt.root->actual_children[1]->data[0] == 300);
    CHECK(bpt.root->actual_children[1]->keys[1] == 4);
    CHECK(bpt.root->actual_children[1]->data[1] == 200);
    CHECK(bpt.root->actual_children[1]->keys[2] == 5);
    CHECK(bpt.root->actual_children[1]->data[2] == 150);

    CHECK(bpt.root->actual_children[0]->next == bpt.root->actual_children[1]->offset);
    CHECK(bpt.root->actual_children[1]->prev == bpt.root->actual_children[0]->offset);
    bpt.delete_index();
}

TEST_CASE("Crete index, insert with create subtree and search")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.insert(1, 170);
    bpt.insert(3, 110);
    bpt.insert(2, 150);
    bpt.insert(1, 220);
    bpt.root->flush();
    const auto &found = bpt.find(1);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 2);
    CHECK(flatten.count(170) > 0);
    CHECK(flatten.count(220) > 0);
    CHECK(flatten.count(150) == 0);
    bpt.delete_index();
}

TEST_CASE("Crete index, insert same + create subtree and search")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.insert(1, 170);
    bpt.insert(1, 110);
    bpt.insert(1, 150);
    bpt.insert(1, 220);
    bpt.root->flush();
    const auto &found = bpt.find(1);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 4);
    CHECK(flatten.count(170) > 0);
    CHECK(flatten.count(110) > 0);
    CHECK(flatten.count(150) > 0);
    CHECK(flatten.count(220) > 0);
    bpt.delete_index();
}

TEST_CASE("Crete index, insert same + create subtree and search")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    bpt.insert(1, 170);
    bpt.insert(2, 110);
    bpt.insert(2, 150);
    bpt.insert(1, 220);
    bpt.root->flush();
    const auto &found = bpt.find(1);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == 2);
    CHECK(flatten.count(170) > 0);
    CHECK(flatten.count(220) > 0);

    const auto &found_2 = bpt.find(2);
    const auto &flatten_2 = bpt.flatten_intervals_to_ptrs(found_2);
    CHECK(flatten_2.size() == 2);
    CHECK(flatten_2.count(110) > 0);
    CHECK(flatten_2.count(150) > 0);
    bpt.delete_index();
}

TEST_CASE("Crete index, insert + create subtree with internal and search")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    vector<pair<int64_t, int64_t>> items{make_pair(2, 500), make_pair(3, 400), make_pair(4, 300),
                                         make_pair(5, 200), make_pair(6, 150), make_pair(1, 270),
                                         make_pair(0, 370)};

    for (const auto &item : items)
        bpt.insert(item.first, item.second);

    for (const auto &item : items)
    {
        const auto &found = bpt.find(item.first);
        const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
        CHECK(flatten.size() == 1);
        CHECK(flatten.count(item.second) > 0);
    }

    bpt.delete_index();
}

TEST_CASE("Crete index, insert same + create subtree with internal and search")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    vector<pair<int64_t, int64_t>> items{make_pair(1, 500), make_pair(1, 400), make_pair(1, 300),
                                         make_pair(1, 200), make_pair(1, 150), make_pair(1, 270),
                                         make_pair(1, 370)};

    INSERT_PAIRS_AND_VALIDATE(bpt, items);

    bpt.root->flush();
    const auto &found = bpt.find(1);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == items.size());
    for (const auto &item : items)
        CHECK(flatten.count(item.second) > 0);

    bpt.delete_index();
}

TEST_CASE("Crete index, insert partially same + create subtree with internal and search")
{
    BPTreeIndex<int64_t> bpt("user", "uuid");
    bpt.delete_index();
    bpt.create();
    vector<pair<int64_t, int64_t>> ones{make_pair(1, 500), make_pair(1, 400), make_pair(1, 300)};
    vector<pair<int64_t, int64_t>> twos{make_pair(2, 550), make_pair(2, 410)};
    vector<pair<int64_t, int64_t>> zeros{make_pair(0, 1000), make_pair(0, 2000)};

    INSERT_PAIRS_AND_VALIDATE(bpt, ones);
    INSERT_PAIRS_AND_VALIDATE(bpt, twos);
    INSERT_PAIRS_AND_VALIDATE(bpt, zeros);

    bpt.root->flush();
    const auto &found = bpt.find(1);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.size() == ones.size());
    for (const auto &item : ones)
        CHECK(flatten.count(item.second) > 0);

    const auto &found2 = bpt.find(2);
    const auto &flatten2 = bpt.flatten_intervals_to_ptrs(found2);
    CHECK(flatten2.size() == twos.size());
    for (const auto &item : twos)
        CHECK(flatten2.count(item.second) > 0);

    const auto &found3 = bpt.find(0);
    const auto &flatten3 = bpt.flatten_intervals_to_ptrs(found3);
    CHECK(flatten3.size() == zeros.size());
    for (const auto &item : zeros)
        CHECK(flatten3.count(item.second) > 0);

    bpt.delete_index();
}

#endif // is testing
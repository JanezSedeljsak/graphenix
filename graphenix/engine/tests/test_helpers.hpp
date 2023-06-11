inline void INSERT_PAIRS_AND_VALIDATE(BPTreeIndex<int64_t> &bpt, vector<pair<int64_t, int64_t>> &items)
{
    for (const auto &item : items)
        bpt.insert(item.first, item.second);

    for (const auto &item : items)
    {
        const auto &found = bpt.find(item.first);
        const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
        CHECK(flatten.count(item.second) > 0);
    }
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

inline void CHECK_IF_BASIC_INSERT_WORKED(BPTreeIndex<int64_t> &bpt)
{
    CHECK(bpt.root->keys.size() == 7); // 6 existing + 1 inserted
    const auto &found = bpt.find(3);
    const auto &flatten = bpt.flatten_intervals_to_ptrs(found);
    CHECK(flatten.count(256) > 0);
}

inline void CHECK_FOR_012(BPTreeIndex<int64_t> &bpt, vector<pair<int64_t, int64_t>> &zeros,
                          vector<pair<int64_t, int64_t>> &ones, vector<pair<int64_t, int64_t>> &twos)
{
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
}
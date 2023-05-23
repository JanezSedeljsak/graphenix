#ifdef IS_TESTING

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "benchmark.h"
#include "../bptree/bptreenode.hpp"

TEST_CASE("Create bptree node")
{
    BPTreeNode<int64_t> *n = new BPTreeNode<int64_t>(0, 8);
    unique_ptr<BPTreeNode<int64_t>> n_ptr = unique_ptr<BPTreeNode<int64_t>>(n);
    CHECK(n_ptr->is_leaf == true);
    double avg;
    BENCHMARK(n_ptr->flush(), 100, avg);
    // todo: write avg to txt file to get results
}

#endif // is testing
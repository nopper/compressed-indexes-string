#define BOOST_TEST_MODULE iterate_graph

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "ps/graphs/graph_types.hpp"
#include <vector>

using namespace ps::graphs;

template <typename GraphImpl>
void test_iterate_graph()
{
    GraphImpl graph("test_data/simple-test.tsv.gz");

    std::vector<int> values;
    std::vector<int> expected = {0, 1, 0, 2, 0, 4, 0, 5,
                                 1, 2, 1, 3,
                                 2, 4, 2, 5};
    Edge edge;

    while (graph.get_next(edge))
    {
        values.push_back(edge.first);
        values.push_back(edge.second);
    }

    BOOST_REQUIRE_EQUAL_COLLECTIONS(values.begin(), values.end(),
                                    expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(iterate_graph)
{
    test_iterate_graph<gz_graph>();
}

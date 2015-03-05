#define BOOST_TEST_MODULE iterate_edges

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "ps/graphs/graph_types.hpp"
#include <vector>

using namespace ps::graphs;

template <typename GraphImpl>
void test_iterate_edges()
{
    GraphImpl graph("test_data/simple-test.tsv.gz");
    edges_iterator<GraphImpl> iterable(graph);

    std::vector<int> values;

    Edges edges;

    while (iterable.get_next(edges))
    {
        values.push_back(edges.first);

        for (auto it = edges.second.begin(); it != edges.second.end(); ++it) {
            int value = *it;
            values.push_back(value);
        }
    }

    std::vector<int> expected = {0, 1, 2, 4, 5,
                                 1, 2, 3,
                                 2, 4, 5};
    BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(),
                                    values.begin(), values.end());
}

BOOST_AUTO_TEST_CASE(iterate_graph)
{
    test_iterate_edges<gz_graph>();
}

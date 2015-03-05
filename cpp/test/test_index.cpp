#define BOOST_TEST_MODULE index_file

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/tuple/tuple.hpp>
#include <vector>

#include "test/test_generic_sequence.hpp"
#include "test/perftest_common.hpp"

#include "ps/graphs/graph_types.hpp"
#include "ps/graphs/serializer.hpp"
#include "ps/sequences/options.hpp"
#include "ps/indices/index_types.hpp"
#include "ps/indices/neighbors.hpp"


using namespace ps;
using namespace ps::graphs;
using namespace ps::indices;
namespace bfs = boost::filesystem;

template <typename GraphType, typename Index>
void test_index_file(const char *name)
{
    boost::system::error_code ec;
    bfs::path test_root(bfs::unique_path(bfs::temp_directory_path(ec) / "%%%%-%%%%-%%%%"));

    if (!bfs::create_directory(test_root, ec) || ec) {
        cout << "Failed creating " << test_root << ": " << ec.message() << endl;
        BOOST_ERROR("Unable to create temp directory");
        return;
    }

    bfs::path index_path(test_root / "index");

    std::cout << "Testing index using " << name << " encoding" << std::endl;
    std::cout << "Temporary files will be stored in " << index_path.string() << std::endl;

    // TODO: Generate a random graph instead of using this small example
    GraphType graph("test_data/simple-test.tsv.gz");

    {
        TIMEIT("Creating index", 1)
        {
            uint64_t universe = 6;
            sequences::options opts(universe);

            Index index(opts, index_path.c_str());

            graph_serializer<GraphType, Index> serializer(graph, index);
        }
    }

    {
        TIMEIT("Querying index", 1)
        {
            Index index(index_path.c_str());

            {
                std::vector<int> expected = {1, 2, 4, 5};

                Edges edges;
                neighbors(index, 0, edges);

                BOOST_REQUIRE_EQUAL(edges.first, 0);
                BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(),
                                                edges.second.begin(), edges.second.end());
            }

            {
                std::vector<int> expected = {1, 2, 3, 4, 5};
                Edges edges;
                neighbors(index, 0, edges, 2);

                BOOST_REQUIRE_EQUAL(edges.first, 0);
                BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(),
                                                edges.second.begin(), edges.second.end());
            }
        }
    }

    bfs::remove_all(test_root);
}

#define LOOP_BODY(R, DATA, T)                                     \
BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(test_index_file_, T)) {         \
    test_index_file<gz_graph, BOOST_PP_CAT(T, _index)>(BOOST_STRINGIZE(T)); \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SIMPLE_INDEX_TYPES);
#undef LOOP_BODY

template <typename GraphType, typename Index>
void test_topk_index(const char *name)
{
    boost::system::error_code ec;
    bfs::path test_root(bfs::unique_path(bfs::temp_directory_path(ec) / "%%%%-%%%%-%%%%"));

    if (!bfs::create_directory(test_root, ec) || ec) {
        cout << "Failed creating " << test_root << ": " << ec.message() << endl;
        BOOST_ERROR("Unable to create temp directory");
        return;
    }

    bfs::path index_path(test_root / "index");

    std::cout << "Testing index using " << name << " encoding" << std::endl;
    std::cout << "Temporary files will be stored in " << index_path.string() << std::endl;

    // TODO: Generate a random graph instead of using this small example
    GraphType graph("test_data/coverage.tsv.gz");

    {
        TIMEIT("Creating index", 1)
        {
            uint64_t universe = 23;
            sequences::options opts(universe);

            Index index(opts, index_path.c_str(), "test_data/ranking-coverage.tsv.gz");

            graph_serializer<GraphType, Index> serializer(graph, index);
        }
    }

    {
        Index index(index_path.c_str());
        uint64_t seq_offset = 0;

        BOOST_REQUIRE_EQUAL(index.get_offset(4, seq_offset), true);

        auto en = index.sequence_at(seq_offset);

        BOOST_REQUIRE_EQUAL(en.size(), 5);

        {
            std::vector<int> expected = {1, 2, 5, 8, 10};

            Edges edges;
            neighbors(index, 4, edges);

            BOOST_REQUIRE_EQUAL(edges.first, 4);
            BOOST_REQUIRE_EQUAL_COLLECTIONS(expected.begin(), expected.end(),
                                            edges.second.begin(), edges.second.end());
        }

        {
            typename Index::rmq_sequence rmq(4, en, 0);
            index.get_rmq_sequence(4, rmq);

            rmq.rmq(0, rmq.size() - 1);
            BOOST_REQUIRE_EQUAL(rmq.size(), 5);

            while (rmq.next()) {
                std::cout << "Min value so far " << std::get<0>(rmq.value()) << ":" << std::get<1>(rmq.value()) << std::endl;
            }
        }

        {
            typename Index::rmq_sequence rmq(4, en, 0);
            index.get_rmq_sequence(4, rmq);

            rmq.rmq(0, rmq.size() - 1);
            BOOST_REQUIRE_EQUAL(rmq.size(), 5);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 10);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 9);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 5);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 6);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 8);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 3);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 2);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 2);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 1);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 0);

            BOOST_REQUIRE_EQUAL(rmq.next(), false);
        }

        {
            uint64_t seq_offset = 0;

            BOOST_REQUIRE_EQUAL(index.get_offset(10, seq_offset), true);

            auto en = index.sequence_at(seq_offset);

            BOOST_REQUIRE_EQUAL(en.size(), 6);

            typename Index::rmq_sequence rmq(10, en);
            index.get_rmq_sequence(10, rmq);

            rmq.rmq(0, rmq.size() - 1);
            BOOST_REQUIRE_EQUAL(rmq.size(), 6);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 6);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 10);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 4);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 8);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 5);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 6);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 3);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 5);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 2);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 2);

            BOOST_REQUIRE_EQUAL(rmq.next(), true);
            BOOST_REQUIRE_EQUAL(std::get<0>(rmq.value()), 1);
            BOOST_REQUIRE_EQUAL(std::get<1>(rmq.value()), 0);

            BOOST_REQUIRE_EQUAL(rmq.next(), false);
        }
    }

    bfs::remove_all(test_root);
}

#define LOOP_BODY(R, DATA, T)                                               \
BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(test_topk_index_file_, T)) {              \
    test_topk_index<gz_graph, BOOST_PP_CAT(T, _index)>(BOOST_STRINGIZE(T)); \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_TOPK_INDEX_TYPES);
#undef LOOP_BODY

#define BOOST_TEST_MODULE rmq

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "ps/indices/builders/rmq_sequences.hpp"
#include "ps/sequences/options.hpp"
#include <vector>

using namespace ps;
using namespace ps::indices;
namespace bfs = boost::filesystem;

void test_rmq()
{
    // Create temporary directory
    boost::system::error_code ec;
    bfs::path test_root(bfs::unique_path(bfs::temp_directory_path(ec) / "%%%%-%%%%-%%%%"));

    if (!bfs::create_directory(test_root, ec) || ec) {
        cout << "Failed creating " << test_root << ": " << ec.message() << endl;
        BOOST_ERROR("Unable to create temp directory");
        return;
    }
    bfs::path rmq_path(test_root / "rmq");

    // Let's first create the RMQ sequences
    {
        uint64_t universe = 42; // not used
        sequences::options opts(universe);

        topk::rmq_sequences seq(opts, rmq_path.c_str());
        topk::rmq_sequences::builder builder(seq, 6);

        uint64_t cdf_degree = 0;

        {
            std::vector<uint64_t> vec = {1, 2, 3};
            cdf_degree += vec.size();
            builder.append(0, cdf_degree, vec.size(), vec.begin());
        }
        {
            std::vector<uint64_t> vec = {4, 5, 6};
            cdf_degree += vec.size();
            builder.append(1, cdf_degree, vec.size(), vec.begin());
        }
        {
            std::vector<uint64_t> vec = {7, 8, 9};
            cdf_degree += vec.size();
            builder.append(2, cdf_degree, vec.size(), vec.begin());
        }


        builder.commit();
    }

    // Let's first create the RMQ sequences
    {
        topk::rmq_sequences seq(rmq_path.c_str());

        BOOST_REQUIRE_EQUAL(seq.docid().size(), 3);
        BOOST_REQUIRE_EQUAL(seq.degree().size(), 2);
        BOOST_REQUIRE_EQUAL(seq.cartesian_trees().size(), 2);

        std::vector<uint64_t> docid = {0, 0, 1};
        std::vector<uint64_t> degree = {0, 6};

        BOOST_REQUIRE_EQUAL_COLLECTIONS(docid.begin(), docid.end(),
                                        seq.docid().begin(), seq.docid().end());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(degree.begin(), degree.end(),
                                        seq.degree().begin(), seq.degree().end());
    }

    // Remove everything
    bfs::remove_all(test_root);
}

BOOST_AUTO_TEST_CASE(rmq)
{
    test_rmq();
}

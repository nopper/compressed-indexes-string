#define BOOST_TEST_MODULE dicts

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "ps/dicts/dict_types.hpp"
#include <vector>
#include <string>

using namespace std;
using namespace ps::dicts;
namespace bfs = boost::filesystem;

template <typename Dict>
void test_dicts()
{
    // Create temporary directory
    boost::system::error_code ec;
    bfs::path test_root(bfs::unique_path(bfs::temp_directory_path(ec) / "%%%%-%%%%-%%%%"));

    if (!bfs::create_directory(test_root, ec) || ec) {
        cout << "Failed creating " << test_root << ": " << ec.message() << endl;
        BOOST_ERROR("Unable to create temp directory");
        return;
    }

    bfs::path dict_file(test_root / "dict");

    {
        vector<string> strings;
        strings.push_back(string("ciao"));
        strings.push_back(string("miao"));
        strings.push_back(string("micio"));
        strings.push_back(string("mocio"));

        BOOST_REQUIRE_EQUAL(strings.size(), 4);

        Dict::build(strings, dict_file.c_str());
    }

    Dict dict(dict_file.c_str());

    {
        string query("ciao");
        BOOST_REQUIRE_EQUAL(dict.rank(query), 0);
    }

    {
        string query("none");
        BOOST_REQUIRE_EQUAL(dict.rank(query), -1);
    }

    {
        string query;
        dict.select(0, query);
        BOOST_REQUIRE_EQUAL(query, "ciao");
    }

    {
        string query("ciao");
        auto ret = dict.prefix_search(query);

        BOOST_REQUIRE_EQUAL(ret.first, 0);
        BOOST_REQUIRE_EQUAL(ret.second, 1);
    }

    {
        string query("m");
        auto ret = dict.prefix_search(query);

        BOOST_REQUIRE_EQUAL(ret.first, 1);
        BOOST_REQUIRE_EQUAL(ret.second, 4);
    }

    {
        string query("zoo");
        auto ret = dict.prefix_search(query);

        BOOST_REQUIRE_EQUAL(ret.first, -1);
        BOOST_REQUIRE_EQUAL(ret.second, -1);
    }

    // Remove everything
    bfs::remove_all(test_root);
}

BOOST_AUTO_TEST_CASE(dicts)
{
    test_dicts<permuterm_dict>();
    test_dicts<strarray_dict>();
}

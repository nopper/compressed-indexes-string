#define BOOST_TEST_MODULE files

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <iostream>
#include "ps/files.hpp"

using namespace ps::files;
namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_CASE(files)
{
    boost::system::error_code ec;
    bfs::path test_root(bfs::unique_path(bfs::temp_directory_path(ec) / "%%%%-%%%%-%%%%"));

    if (!bfs::create_directory(test_root, ec) || ec) {
        std::cout << "Failed creating " << test_root << ": " << ec.message() << std::endl;
        BOOST_ERROR("Unable to create temp directory");
        return;
    }

    bfs::path tempfile(test_root / "database");

    std::unique_ptr<writable_file> file;
    new_writable_file(tempfile.string(), &file);
    file->append(std::string("ciao mamma :)\n"));
    BOOST_REQUIRE_EQUAL(file->get_file_size(), 14);
    file->close();

    {
        std::unique_ptr<random_access_file> file;
        BOOST_REQUIRE_EQUAL(new_random_access_file(tempfile.string(), &file), true);

        std::string str;
        char buff[256];
        BOOST_REQUIRE_EQUAL(file->read(0, 4, &str, buff), true);
        BOOST_REQUIRE_EQUAL(str.size(), 4);
        BOOST_REQUIRE_EQUAL(str, "ciao");
    }

    bfs::remove_all(test_root);
}

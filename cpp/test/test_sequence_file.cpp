#define BOOST_TEST_MODULE sequence_file

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <vector>

#include "test/test_generic_sequence.hpp"
#include "test/perftest_common.hpp"

#include "ps/sequences/options.hpp"
#include "ps/sequences/sequence_types.hpp"
#include "ps/sequences/sequence_file.hpp"

using namespace ps;
using namespace ps::sequences;
namespace bfs = boost::filesystem;

template <typename Sequence>
void test_sequence_file(const char *name)
{
    boost::system::error_code ec;
    bfs::path test_root(bfs::unique_path(bfs::temp_directory_path(ec) / "%%%%-%%%%-%%%%"));

    if (!bfs::create_directory(test_root, ec) || ec) {
        cout << "Failed creating " << test_root << ": " << ec.message() << endl;
        BOOST_ERROR("Unable to create temp directory");
        return;
    }

    bfs::path index_path(test_root / "sequence");

    std::cout << "Testing sequence_file using " << name << " encoding" << std::endl;
    std::cout << "Temporary files will be stored in " << index_path.string() << std::endl;

    uint64_t num_sequences = 20;
    uint64_t universe = std::numeric_limits<uint64_t>::max() >> 1;
    std::vector<std::vector<uint64_t>> sequences;
    std::vector<uint64_t> offsets;

    for (uint64_t i = 0; i < num_sequences; ++i)
        sequences.push_back(random_sequence(universe, 100));

    {
        TIMEIT("Creating sequence_file", 1)
        {
            options opts(universe);

            sequence_file<Sequence> seqfile(opts, index_path.c_str());
            typename sequence_file<Sequence>::builder seq_builder(seqfile);

            uint64_t num_elements = 0;

            for (auto& sequence: sequences)
            {
                uint64_t offset = seq_builder.append(sequence.size(), sequence.begin());
                num_elements += sequence.size();

                offsets.push_back(offset);
            }

            seq_builder.commit();
        }
    }

    {
        TIMEIT("Reading sequence_file", 1)
        {
            sequence_file<Sequence> seqfile(index_path.c_str());

            uint64_t idx = 0;

            for (auto& sequence: sequences)
            {
                auto en = seqfile.sequence_at(offsets[idx++]);

                for (size_t i = 0; i < en.size(); ++i, en.next())
                {
                    BOOST_REQUIRE_EQUAL(en.docid(), sequence[i]);
                }
            }
        }
    }

    bfs::remove_all(test_root);
}

#define LOOP_BODY(R, DATA, T)                                      \
BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(test_sequence_file_, T)) {       \
    test_sequence_file<BOOST_PP_CAT(T, _seq)>(BOOST_STRINGIZE(T)); \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SEQ_TYPES);
#undef LOOP_BODY


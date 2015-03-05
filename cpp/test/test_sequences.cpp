#define BOOST_TEST_MODULE index

#define BOOST_TEST_DYN_LINK
#include <iostream>
#include <vector>
#include <list>
#include <boost/test/unit_test.hpp>
#include "ps/queues.hpp"
#include "ps/sequences/sequence_types.hpp"
#include "test/test_generic_sequence.hpp"

using namespace ps;
using namespace ps::queues;
using namespace ps::sequences;

template <typename Sequence>
struct build_sequence_job : queues::job {
    build_sequence_job(options opts,
                       const std::vector<uint64_t>& posting)
        : m_opts(opts)
        , m_posting(posting)
    {}

    virtual void prepare(void* user_data)
    {
        Sequence::serialize(m_output, m_opts,
                            m_posting.size(), m_posting.begin());
    }

    virtual void commit(void* user_data)
    {
        Sequence s(m_output.c_str(), m_output.size(), m_opts);

        auto en = s.deserialize_at(0);
        std::vector<uint64_t> input;

        for (size_t i = 0; i < en.size(); ++i, en.next())
            input.push_back(en.docid());

        BOOST_REQUIRE_EQUAL_COLLECTIONS(m_posting.begin(), m_posting.end(),
                                        input.begin(), input.end());
    }

    std::string m_output;
    options m_opts;
    const std::vector<uint64_t>& m_posting;
};

template <typename Sequence>
void test_encode_decode_threaded(const char *name)
{
    int num_indices = 100;
    uint64_t universe = 2000;
    options opts(universe);

    std::string encoded;
    std::vector<uint64_t> offsets;
    std::vector<std::vector<uint64_t>> lists;

    for (int i = 0; i < num_indices; ++i)
    {
        std::vector<uint64_t> current = random_sequence(universe, 100);
        lists.push_back(current);
    }

    ps::queues::queue* q = ps::queues::new_queue();

    for (auto& posting: lists)
    {
        std::shared_ptr<build_sequence_job<Sequence>> jobptr(
            new build_sequence_job<Sequence>(opts, posting)
        );
        q->add_job(jobptr, 1);
    }

    q->complete();
    delete q;
}

template <typename Sequence>
void test_encode_decode_nested(const char * name)
{
    int num_indices = 100;
    uint64_t universe = 2000;
    options opts(universe);

    std::string encoded;
    std::vector<uint64_t> offsets;
    std::vector<std::vector<uint64_t>> lists;

    for (int i = 0; i < num_indices; ++i)
    {
        std::vector<uint64_t> current = random_sequence(universe, 100);
        lists.push_back(current);

        std::string output;
        Sequence::serialize(output, opts,
                            current.size(), current.begin());

        offsets.push_back(encoded.size());
        encoded.append(output);
    }

    Sequence s(encoded.c_str(), encoded.size(), opts);

    for (int i = 0; i < num_indices; ++i)
    {
        auto en = s.deserialize_at(offsets[i]);
        for (size_t j = 0; j < en.size(); ++j, en.next())
            BOOST_REQUIRE_EQUAL(en.docid(), lists[i][j]);
    }

    std::cout << "test_encode_decode_nested(" << name
              << "): " << encoded.size() << " bytes" << std::endl;
}

template <typename Sequence>
void test_encode_decode(const char * name)
{
    uint64_t universe = 2000;
    options opts(universe);
    std::vector<uint64_t> expected = random_sequence(universe, 100);

    std::string output;
    Sequence::serialize(output, opts,
                        expected.size(), expected.begin());
    Sequence s(output.c_str(), output.size(), opts);

    auto en = s.deserialize_at(0);
    for (size_t i = 0; i < en.size(); ++i, en.next())
    {
        BOOST_REQUIRE_EQUAL(en.docid(), expected[i]);
    }

    std::cout << "test_encode_decode(" << name
              << "): " << output.size() << " bytes" << std::endl;
}

#define LOOP_BODY(R, DATA, T)                                                   \
    BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(encode_decode_threaded_, T)) {            \
        test_encode_decode_threaded<BOOST_PP_CAT(T, _seq)>(BOOST_STRINGIZE(T)); \
    }
    BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SEQ_TYPES);
#undef LOOP_BODY

#define LOOP_BODY(R, DATA, T)                                                 \
    BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(encode_decode_nested_, T)) {            \
        test_encode_decode_nested<BOOST_PP_CAT(T, _seq)>(BOOST_STRINGIZE(T)); \
    }
    BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SEQ_TYPES);
#undef LOOP_BODY


#define LOOP_BODY(R, DATA, T)                                          \
    BOOST_AUTO_TEST_CASE(BOOST_PP_CAT(encode_decode_, T)) {            \
        test_encode_decode<BOOST_PP_CAT(T, _seq)>(BOOST_STRINGIZE(T)); \
    }
    BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SEQ_TYPES);
#undef LOOP_BODY

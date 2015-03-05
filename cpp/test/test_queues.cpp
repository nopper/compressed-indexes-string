#define BOOST_TEST_MODULE queues

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include "ps/queues.hpp"

using namespace ps;
using namespace ps::queues;

class sleep_job : public job {
public:
    sleep_job(int sleep_time)
        : m_sleep_time(sleep_time)
    {}

    void prepare(void* user_data)
    {
        {
            boost::lock_guard<boost::mutex> lock(m_mtx);
            std::cout << "Sleeping " << m_sleep_time << " ms in " << boost::this_thread::get_id() << std::endl;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(m_sleep_time));
    }

    void commit(void* user_data)
    {
        std::cout << "Woke up after " << m_sleep_time << std::endl;
    }

private:
    static boost::mutex m_mtx;
    int m_sleep_time;
};

boost::mutex sleep_job::m_mtx{};

void test_blocking_queue()
{
    unordered_single_queue q(1);

    boost::posix_time::ptime start = boost::posix_time::microsec_clock::universal_time();

    q.add_job(std::make_shared<sleep_job>(*new sleep_job(10)), 1);
    q.add_job(std::make_shared<sleep_job>(*new sleep_job(10)), 1);
    q.add_job(std::make_shared<sleep_job>(*new sleep_job(10)), 1);
    q.add_job(std::make_shared<sleep_job>(*new sleep_job(10)), 1);

    // This should wait another event to finish (the minimum latency to pay is 1sec)
    q.add_job(std::make_shared<sleep_job>(*new sleep_job(10)), 1);

    double elapsed_ms = static_cast<double>(
        (boost::posix_time::microsec_clock::universal_time() - start).total_milliseconds()
    );

    q.complete();

    std::cout << "Elapsed " << elapsed_ms << std::endl;

    BOOST_REQUIRE(elapsed_ms >= 10);
    BOOST_REQUIRE(elapsed_ms <= 15);
}

template<class Queue>
void test_queues()
{
    Queue q(0);

    boost::posix_time::ptime start = boost::posix_time::microsec_clock::universal_time();

    q.add_job(std::make_shared<sleep_job>(*new sleep_job(10)), 1);
    q.add_job(std::make_shared<sleep_job>(*new sleep_job(20)), 1);
    q.add_job(std::make_shared<sleep_job>(*new sleep_job(10)), 1);
    q.add_job(std::make_shared<sleep_job>(*new sleep_job(30)), 1);

    q.complete();

    double elapsed_ms = static_cast<double>(
        (boost::posix_time::microsec_clock::universal_time() - start).total_milliseconds()
    );

    std::cout << "Elapsed " << elapsed_ms << std::endl;

    BOOST_REQUIRE(elapsed_ms >= 30);
    BOOST_REQUIRE(elapsed_ms <= 35);
}

BOOST_AUTO_TEST_CASE(iterate_graph)
{
    test_queues<ordered_queue>();
    test_queues<unordered_single_queue>();
    test_queues<unordered_multi_queue>();
    test_blocking_queue();
}

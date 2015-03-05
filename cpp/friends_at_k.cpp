#include "ps/optargs.hpp"
#include "ps/queues.hpp"
#include "ps/graphs/graph_types.hpp"
#include "ps/indices/index_types.hpp"
#include "ps/indices/neighbors.hpp"
#include <mutex>
#include <list>

using namespace ps;
using namespace ps::graphs;
using namespace ps::indices;

template<typename T>
struct less_deque {
    bool operator()(const std::deque<T> *a, const std::deque<T> *b) const {
        if (a->front() == b->front())
            return a < b;
        return a->front() < b->front();
    }
};

template<typename T>
void merge_sorted_lists(T node, std::vector<T> &output, std::vector<std::deque<T> > &input)
{
    std::set<std::deque<T> *, less_deque<T>> pq;

    for (auto it = input.begin(); it != input.end(); it++)
    {
        if (it->empty())
            continue;
        pq.insert(&*it);
    }

    while (!pq.empty()) {
        std::deque<T> *p = *pq.begin();
        pq.erase(pq.begin());

        T x = p->front();
        p->pop_front();

        if (node != x && (output.empty() || x != output.back()))
            output.push_back(x);

        if (!p->empty())
            pq.insert(p);
    }
}


template<typename Index>
class FriendsAtKApp {
public:
    FriendsAtKApp(int k, Index& friends, bool histogram)
        : m_k(k)
        , m_histogram(histogram)
        , m_friends(friends)
    {}

    void complete()
    {
        m_queue.complete();
    }

    void process(int node, double expected_work)
    {
        std::shared_ptr<FriendsAtJob> jobptr(new FriendsAtJob(*this, node));
        m_queue.add_job(jobptr, expected_work);
    }

    struct FriendsAtJob : queues::job {
    public:
        FriendsAtJob(FriendsAtKApp<Index>& parent, int node)
            : m_node(node)
            , m_parent(parent)
        {}

        virtual void prepare(void* user_data)
        {
            assert(m_parent.m_k >=1);

            std::vector<std::deque<int>> adj_lists;

            std::queue<int> curr_queue;
            std::queue<int> next_queue;
            std::unordered_set<int> visited;

            curr_queue.push(m_node);

            int k = m_parent.m_k;

            while (--k >= 0)
            {
                while (!curr_queue.empty())
                {
                    int node = curr_queue.front();

                    visited.insert(node);
                    curr_queue.pop();

                    Edges edges;
                    neighbors(m_parent.m_friends, node, edges);
                    adj_lists.push_back(std::deque<int>(edges.second.begin(), edges.second.end()));

                    for (auto it = edges.second.begin(); it != edges.second.end(); ++it)
                    {
                        int next_node = *it;
                        if (visited.find(next_node) == visited.end() && k >= 0)
                            next_queue.push(next_node);
                    }
                }

                std::swap(curr_queue, next_queue);
            }

            std::vector<int> result;
            merge_sorted_lists<int>(m_node, result, adj_lists);

            std::stringstream ss;

            if (m_parent.m_histogram)
                ss << m_node << '\t' << result.size() << '\n';
            else
            {
                for (auto& v: result)
                    ss << m_node << '\t' << v << '\n';
            }

            m_result = ss.str();
        }

        virtual void commit(void* user_data)
        {
            std::cout << m_result;
        }

    private:
        int m_node;
        FriendsAtKApp<Index>& m_parent;
        std::string m_result;
    };

private:
    int m_k;
    bool m_histogram;
    boost::mutex m_cache_mutex;
    queues::ordered_queue m_queue;
    Index& m_friends;
};

template <typename Index>
void friends_at_k(const char* input_fname,
                  int k,
                  bool histogram)
{
    stdin_graph graph;
    Index index(input_fname);
    FriendsAtKApp<Index> extractor(k, index, histogram);

    progress_logger plog("friends");
    edges_iterator<stdin_graph> iterator(graph);

    Edges edges;

    while (iterator.get_next(edges))
    {
        extractor.process(edges.first, edges.second.size());
        plog.done_item();
    }

    extractor.complete();
    plog.done();
}

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
        ("input,i", po::value<std::string>()->required(), "input index")
        ("index-type,t", po::value<string>()->required(), "Type of index to use")
        ("k", po::value<int>()->default_value(2), "neighbors at k")
        ("histogram,h", po::value<bool>()->default_value(false), "generate histogram only")
    )

    int k = vm["k"].as<int>();
    std::string input_fname = vm["input"].as<std::string>();
    std::string index_type = vm["index-type"].as<std::string>();
    bool histogram = vm["histogram"].as<bool>();

    if (false) {
#define LOOP_BODY(R, DATA, T)                         \
    } else if (index_type == BOOST_PP_STRINGIZE(T)) { \
        friends_at_k<BOOST_PP_CAT(T, _index)>(        \
            input_fname.c_str(),                      \
            k,                                        \
            histogram                                 \
        );

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_INDEX_TYPES);
#undef LOOP_BODY
    } else {
        std::cerr << "ERROR: Unknown index_type " << index_type << std::endl;
        return -1;
    }

    return 0;
}

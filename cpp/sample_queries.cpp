#include "ps/optargs.hpp"
#include "ps/mapping.hpp"
#include "ps/prefix.hpp"
#include "ps/indices/index_types.hpp"
#include "ps/indices/neighbors.hpp"
#include "ps/utils.hpp"
#include "ps/graphs/edges.hpp"

using namespace ps::graphs;
using namespace ps::indices;
using namespace ps::mapping;
using namespace ps::sequences;

template <typename Index>
class query_sampler {
public:
    query_sampler(const char* input_fname,
                  const char* uid_sid_file,
                  const char* uid_str_file,
                  size_t prefix_start,
                  size_t prefix_end,
                  size_t num_queries,
                  size_t n,
                  unsigned seed,
                  bool verification)
        : m_friends(input_fname)
        , m_sid_to_uid(uid_sid_file)
        , m_uid_to_sid(uid_sid_file)
        , m_uid_to_str(uid_str_file)
        , m_prefix_start(prefix_start)
        , m_prefix_end(prefix_end)
        , m_num_queries(num_queries)
        , m_n(n)
        , m_seed(seed)
        , m_verification(verification)
    {}

    void sample_queries_for(int user_id)
    {
        std::vector<int> uids;
        std::vector<std::string> strs;
        get_ids_prefixes_for(user_id, uids, strs);

        if (strs.empty() || uids.empty())
            return;

        std::vector<std::pair<std::string, std::vector<int>>> results_buffer;

        for (size_t i = m_prefix_start; i <= m_prefix_end; ++i)
        {
            std::vector<std::string> prefixes;
            std::vector<double> cdf;
            std::vector<size_t> results;

            ps::prefix::create_prefix_cdf(i, prefixes, cdf, strs.begin(), strs.end());
            ps::prefix::sample_prefixes(m_num_queries, results, prefixes, cdf, true, m_seed);

            for (auto& index: results)
            {
                std::string prefix = prefixes[index];
                std::string prefix_succ = prefix;
                ps::util::successor_of(prefix_succ);

                auto begin = std::lower_bound(strs.begin(), strs.end(), prefix);
                auto end = std::lower_bound(strs.begin(), strs.end(), prefix_succ);

                std::vector<int> results_uids;
                size_t offset = std::distance(strs.begin(), begin);

                for (; begin != end; ++begin)
                {
                    results_uids.push_back(uids[offset]);
                    offset++;
                }

                std::sort(results_uids.begin(), results_uids.end());

                results_buffer.push_back(std::make_pair(
                    prefix, results_uids
                ));
            }
        }

        std::cout << results_buffer.size() << std::endl;

        for (auto& r: results_buffer)
        {
            std::cout << r.first << std::endl;

            if (m_verification)
            {
                std::cout << r.second.size() << std::endl;
                for (auto& v: r.second)
                    std::cout << v << std::endl;
            }
        }
    }

private:
    void get_ids_prefixes_for(int user_id,
                              std::vector<int>& ids,
                              std::vector<std::string>& strs)
    {
        Edges edges;
        int user_sid = 0;
        m_uid_to_sid.get(user_id, user_sid);

        if (!neighbors(m_friends, user_sid, edges, m_n))
            return;

        std::vector<std::pair<int,std::string>> id_str(edges.second.size());

        std::cout << m_verification << std::endl;
        std::cout << user_id << std::endl;

        int offset = 0;

        // We save uid-string mapping in a vector of pairs

        for (auto& sort_id: edges.second)
        {
            // These are sort-ids that need to be translated
            m_sid_to_uid.get(sort_id, id_str[offset].first);
            m_uid_to_str.get(id_str[offset].first, id_str[offset].second);
            offset++;
        }

        // We then sort by lexical ordering
        std::sort(id_str.begin(), id_str.end(),
            [](std::pair<int,std::string> a, std::pair<int,std::string> b) {
                // TODO: Is this lower case?
                return a.second < b.second;
            }
        );

        // And we store this information in two arrays
        //  1) ids that will contain the UIDs of users in order of strings
        //  2) the real strings ordered lexicographically
        ids.resize(id_str.size());
        strs.resize(id_str.size());

        offset = 0;
        for (auto& p: id_str)
        {
            ids[offset] = p.first;
            strs[offset] = p.second;
            offset++;
        }
    }

    Index m_friends;
    SortIdToUserId m_sid_to_uid;
    UserIdToSortId m_uid_to_sid;
    UserIdToString m_uid_to_str;
    size_t m_prefix_start;
    size_t m_prefix_end;
    size_t m_num_queries;
    size_t m_n;
    unsigned m_seed;
    bool m_verification;
};

template <typename Index>
void sample_queries(const char* input_fname,
                    const char* uid_sid_file,
                    const char* uid_str_file,
                    size_t prefix_start,
                    size_t prefix_end,
                    size_t num_queries,
                    size_t n,
                    unsigned seed,
                    bool verification)
{

    query_sampler<Index> sampler(
        input_fname,
        uid_sid_file,
        uid_str_file,
        prefix_start,
        prefix_end,
        num_queries,
        n,
        seed,
        verification
    );

    std::string line;

    while (succinct::util::fast_getline(line))
    {
        int user_id = atoi(line.c_str());
        sampler.sample_queries_for(user_id);
    }
}

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
        ("input,i", po::value<std::string>()->required(), "input index")
        ("id-mapping", po::value<std::string>()->required(), "ID Mapping file")
        ("str-mapping", po::value<std::string>()->required(), "ID String Mapping file")
        ("index-type,t", po::value<std::string>()->required(), "Type of index to use")
        ("prefix-min", po::value<int>()->default_value(1), "Minimum prefix length")
        ("prefix-max", po::value<int>()->default_value(5), "Maximum prefix length")
        ("k,k", po::value<int>()->default_value(1), "Number of queries per prefix length")
        ("n,n", po::value<int>()->default_value(2), "Use N1, N2 or Nn")
        ("seed", po::value<unsigned>()->default_value(42), "Seed number")
        ("verification,v", po::value<bool>()->default_value(true), "Include verification")
    )

    std::string input_fname = vm["input"].as<std::string>();
    std::string index_type = vm["index-type"].as<std::string>();
    std::string id_mapping = vm["id-mapping"].as<std::string>();
    std::string str_mapping = vm["str-mapping"].as<std::string>();

    int prefix_min = vm["prefix-min"].as<int>();
    int prefix_max = vm["prefix-max"].as<int>();
    int k = vm["k"].as<int>();
    int n = vm["n"].as<int>();
    unsigned seed = vm["seed"].as<unsigned>();
    bool verification = vm["verification"].as<bool>();

    if (false) {
#define LOOP_BODY(R, DATA, T)                         \
    } else if (index_type == BOOST_PP_STRINGIZE(T)) { \
        sample_queries<BOOST_PP_CAT(T, _index)>(      \
            input_fname.c_str(),                      \
            id_mapping.c_str(),                       \
            str_mapping.c_str(),                      \
            prefix_min,                               \
            prefix_max,                               \
            k,                                        \
            n,                                        \
            seed,                                     \
            verification                              \
        );

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_INDEX_TYPES);
#undef LOOP_BODY
    } else {
        std::cerr << "ERROR: Unknown index_type " << index_type << std::endl;
        return -1;
    }

    return 0;
}

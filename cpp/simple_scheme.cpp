#include "ps/optargs.hpp"
#include "ps/dicts/dict_types.hpp"
#include "ps/indices/index_types.hpp"
#include "ps/indices/neighbors.hpp"
#include "ps/graphs/edges.hpp"
#include "ps/mapping.hpp"
#include "ps/problems/schemes.hpp"
#include "ps/problems/intersection.hpp"
#include "ps/problems/topk.hpp"
#include <boost/chrono.hpp>
#include <fstream>
#include <random>

using namespace ps;
using namespace ps::dicts;
using namespace ps::graphs;
using namespace ps::indices;
using namespace ps::mapping;
using namespace ps::problems;

namespace bc = boost::chrono;

template<typename DictType>
void execute_prefix_search(const DictType& dictionary,
                           const std::string& query,
                           int& l,
                           int& r)
{
    auto pair = dictionary.prefix_search(query);
    l = pair.first;
    r = pair.second;
}

template<typename Index, typename DictType>
void process_intersection(const Index& index,
                          const DictType& dictionary,
                          const std::vector<uint64_t>& vec_remapping,
                          int iteration,
                          int user_id,
                          int sort_id,
                          const std::string& query,
                          const Scheme scheme,
                          const vector<uint64_t>& ground_truth,
                          bool verification)
{
    std::vector<uint64_t> result;
    uint64_t tt_psearch_usec = 0;
    uint64_t tt_inter_usec = 0;

    int l = -1;
    int r = -1;

    bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
    execute_prefix_search<DictType>(dictionary, query, l, r);
    tt_psearch_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();

    if (l != -1)
    {
        switch (scheme)
        {
        case AsIndex:
            {
                problems::intersection::solver<Index, Schemes::asindex> s(index);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve(sort_id, l, r, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case Hopping:
            {
                problems::intersection::solver<Index, Schemes::hopping> s(index);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve(sort_id, l, r, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case Coverage:
            {
                problems::intersection::solver<Index, Schemes::coverage> s(index);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve(sort_id, l, r, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case BaselineHopping:
            {
                problems::intersection::solver<Index, Schemes::baseline_hopping> s(index);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();

                // We first need to copy in the remapping vector the possible users included in [l;r]
                std::vector<uint64_t> remapping(vec_remapping.begin() + l, vec_remapping.begin() + r);
                std::sort(remapping.begin(), remapping.end());

                s.solve_baseline_hopping(sort_id, remapping, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case BaselineAsIndex:
            {
                problems::intersection::solver<Index, Schemes::baseline_asindex> s(index);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();

                // We first need to copy in the remapping vector the possible users included in [l;r]
                std::vector<uint64_t> remapping(vec_remapping.begin() + l, vec_remapping.begin() + r);
                std::sort(remapping.begin(), remapping.end());

                s.solve_baseline_asindex(sort_id, remapping, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case FastBaselineHopping:
            {
                problems::intersection::solver<Index, Schemes::fast_baseline_hopping> s(index);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve_fast_baseline_hopping(sort_id, l, r, vec_remapping, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case FastBaselineAsIndex:
            {
                problems::intersection::solver<Index, Schemes::fast_baseline_asindex> s(index);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve_fast_baseline_asindex(sort_id, l, r, vec_remapping, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        default:
            break;
        }
    }

    /* ps::logger() << */
    /*     "Query: " << query << " user_id: " << user_id << " " */
    /*     "Prefix search: " << tt_psearch_usec << " usec (l=" << l << ",r=" << r << ") " */
    /*     "Intersection: " << tt_inter_usec << " usec (" << result.size() << " results)" */
    /* << std::endl; */

    if (verification && ground_truth != result)
    {
        std::cerr << "Ground truth and results are different: " << std::endl;
        for (auto& v1: ground_truth)
            std::cerr << v1 << ' ';
        std::cerr << std::endl;
        for (auto& v1: result)
            std::cerr << v1 << ' ';
        std::cerr << std::endl;
        exit(-1);
    }

    std::cout << iteration      << "\t"
              << user_id        << "\t"
              << query          << "\t"
              << l              << "\t"
              << r              << "\t"
              << result.size()  << "\t"
              << tt_psearch_usec<< "\t"
              << tt_inter_usec  << std::endl;
}

template<typename Index, typename DictType>
void process_topk(const Index& index,
                  const DictType& dictionary,
                  const std::vector<uint64_t>& vec_remapping,
                  const std::vector<uint64_t>& ranking,
                  const std::vector<uint64_t>& wand,
                  int iteration,
                  int user_id,
                  int sort_id,
                  const std::string& query,
                  const Scheme scheme,
                  const vector<uint64_t>& ground_truth,
                  bool verification,
                  const int topk)
{
    std::vector<uint64_t> result(topk);
    uint64_t tt_psearch_usec = 0;
    uint64_t tt_inter_usec = 0;

    int l = -1;
    int r = -1;

    bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
    execute_prefix_search<DictType>(dictionary, query, l, r);
    tt_psearch_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();

    if (l != -1)
    {
        switch (scheme)
        {
        case TopkHoppingRMQWAND:
            {
                problems::topk::solver<Index, Schemes::topk_hopping_rmq_wand> s(index, ranking, wand, topk);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve(sort_id, l, r, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case TopkHoppingRMQ:
            {
                problems::topk::solver<Index, Schemes::topk_hopping_rmq> s(index, ranking, wand, topk);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve(sort_id, l, r, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case TopkHoppingWAND:
            {
                problems::topk::solver<Index, Schemes::topk_hopping_wand> s(index, ranking, wand, topk);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve(sort_id, l, r, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        case TopkHopping:
            {
                problems::topk::solver<Index, Schemes::topk_hopping> s(index, ranking, wand, topk);
                bc::high_resolution_clock::time_point t_start = bc::high_resolution_clock::now();
                s.solve(sort_id, l, r, result);
                tt_inter_usec = bc::duration_cast<bc::nanoseconds>(bc::high_resolution_clock::now() - t_start).count();
                break;
            }
        default:
            break;
        }
    }

    /* ps::logger() << */
    /*     "Query: " << query << " user_id: " << user_id << " " */
    /*     "Prefix search: " << tt_psearch_usec << " usec (l=" << l << ",r=" << r << ") " */
    /*     "Intersection: " << tt_inter_usec << " usec (" << result.size() << " results)" */
    /* << std::endl; */

    if (verification && ground_truth != result)
    {
        std::cerr << "Ground truth and results are different: " << std::endl;
        for (auto& v1: ground_truth)
            std::cerr << v1 << ' ';
        std::cerr << std::endl;
        for (auto& v1: result)
            std::cerr << v1 << ' ';
        std::cerr << std::endl;
        exit(-1);
    }

    std::cout << iteration      << "\t"
              << user_id        << "\t"
              << query          << "\t"
              << l              << "\t"
              << r              << "\t"
              << result.size()  << "\t"
              << tt_psearch_usec<< "\t"
              << tt_inter_usec  << std::endl;
}

typedef std::tuple<uint64_t, uint64_t, std::string, std::vector<uint64_t>> query_type;


void read_queries(UserIdToSortId& uid_to_sid, std::vector<uint64_t>& ranking, int topk,
                  const std::string& query_file, std::vector<query_type>& queries,
                  bool include_verification)
{
    std::ifstream queryfile(query_file);

    while (true)
    {
        bool verification;
        int user_id;

        queryfile >> verification;
        queryfile >> user_id;

        int sort_id = 0;
        uid_to_sid.get(user_id, sort_id);

        if (!queryfile)
            break;

        int num_queries;
        queryfile >> num_queries;

        for (int i = 0; i < num_queries; ++i)
        {
            std::string query;
            std::getline(queryfile, query, '\n');
            std::getline(queryfile, query, '\n');

            std::vector<uint64_t> results;

            if (verification)
            {
                int num_results;
                queryfile >> num_results;

                results.resize(num_results);

                for (int j = 0; j < num_results; ++j)
                {
                    int real_id;
                    int tmp = 0;
                    queryfile >> real_id;

                    if (include_verification)
                    {
                        uid_to_sid.get(real_id, tmp);
                        results[j] = tmp;
                    }
                }

                if (topk == 0)
                    std::sort(results.begin(), results.end());
                else
                {
                    std::sort(results.begin(), results.end(), [&ranking](uint64_t a, uint64_t b) {
                        return ranking[a] > ranking[b];
                    });
                    results.resize(std::min(topk, (int)results.size()));
                }
            }

            queries.emplace_back(user_id, sort_id, query, results);
        }
    }
}

template<typename Index, typename DictType>
int prefix_search(const std::string& query_file,
                  const Index& index,
                  const DictType& dictionary,
                  const std::string& scheme,
                  const std::string& id_mapping,
                  const std::string& dict_remapping,
                  const std::string& ranking_file,
                  const std::string& wand_data,
                  const int iterations,
                  const int topk,
                  const bool verification,
                  const int seed)
{
    Scheme s = Scheme::AsIndex;

    if (scheme == "coverage")
    {
        ps::logger() << "Using coverage scheme" << std::endl;
        s = Scheme::Coverage;
    }
    else if (scheme == "topk-hopping")
    {
        ps::logger() << "Using topk-hopping scheme" << std::endl;
        s = Scheme::TopkHopping;
    }
    else if (scheme == "topk-hopping-rmq-wand")
    {
        ps::logger() << "Using topk-hopping-rmq-wand scheme" << std::endl;
        s = Scheme::TopkHoppingRMQWAND;
    }
    else if (scheme == "topk-hopping-rmq")
    {
        ps::logger() << "Using topk-hopping-rmq scheme" << std::endl;
        s = Scheme::TopkHoppingRMQ;
    }
    else if (scheme == "topk-hopping-wand")
    {
        ps::logger() << "Using topk-hopping-wand scheme" << std::endl;
        s = Scheme::TopkHoppingWAND;
    }
    else if (scheme == "hopping")
    {
        ps::logger() << "Using hopping scheme" << std::endl;
        s = Scheme::Hopping;
    }
    else if (scheme == "baseline-hopping")
    {
        ps::logger() << "Using baseline-hopping scheme" << std::endl;
        s = Scheme::BaselineHopping;
    }
    else if (scheme == "baseline-asindex")
    {
        ps::logger() << "Using baseline-asindex scheme" << std::endl;
        s = Scheme::BaselineAsIndex;
    }
    else if (scheme == "fast-baseline-hopping")
    {
        ps::logger() << "Using fast-baseline-hopping scheme" << std::endl;
        s = Scheme::FastBaselineHopping;
    }
    else if (scheme == "fast-baseline-asindex")
    {
        ps::logger() << "Using fast-baseline-asindex scheme" << std::endl;
        s = Scheme::FastBaselineAsIndex;
    }
    else
    {
        ps::logger() << "Using asindex scheme" << std::endl;
    }

    ps::logger() << "Loading UID -> SID translation table ..." << std::endl;
    UserIdToSortId uid_to_sid(id_mapping.c_str());

    std::vector<uint64_t> vec_remapping;

    if (!dict_remapping.empty())
    {
        ps::util::read_vector_from(dict_remapping, vec_remapping);
        ps::logger() << "Dict remapping loaded size()=" << vec_remapping.size() << std::endl;
    }

    std::vector<uint64_t> ranking(index.opts().universe);
    std::vector<uint64_t> wand(index.opts().universe);

    if (topk > 0)
    {
        if (!ranking_file.empty())
        {
            ps::util::read_ranking_from(ranking_file, ranking);
            ps::logger() << "Ranking loaded size()=" << ranking.size() << std::endl;
        }

        if (!wand_data.empty())
        {
            ps::util::read_ranking_from(wand_data, wand);
            ps::logger() << "WAND data loaded size()=" << wand.size() << std::endl;
        }
    }

    std::vector<query_type> queries;
    read_queries(uid_to_sid, ranking, topk, query_file, queries, verification);

    shuffle(queries.begin(), queries.end(), std::default_random_engine(seed));

    for (int i = 0; i < iterations; i++)
    {
        for (const query_type& q: queries)
        {
            if (topk == 0)
                process_intersection<Index, DictType>(
                    index, dictionary, vec_remapping, i,
                    std::get<0>(q), std::get<1>(q), std::get<2>(q), s, std::get<3>(q), verification
                );
            else
                process_topk<Index, DictType>(
                    index, dictionary, vec_remapping, ranking, wand, i,
                    std::get<0>(q), std::get<1>(q), std::get<2>(q), s, std::get<3>(q), verification, topk
                );
        }
    }

    return 0;
}

template<typename Index>
int prefix_search(const std::string query_file,
                  const Index& index,
                  const std::string& dict,
                  const std::string& dict_type,
                  const std::string& scheme,
                  const std::string& id_mapping,
                  const std::string& dict_remapping,
                  const std::string& ranking_file,
                  const std::string& wand_data,
                  const int iterations,
                  const int topk,
                  const bool verification,
                  const int seed)
{
    if (false) {
#define LOOP_BODY(R, DATA, T)                                \
    } else if (dict_type == BOOST_PP_STRINGIZE(T)) {         \
        BOOST_PP_CAT(T, _dict) dictionary(dict.c_str());     \
        return prefix_search<Index, BOOST_PP_CAT(T, _dict)>( \
            query_file,                                      \
            index,                                           \
            dictionary,                                      \
            scheme,                                          \
            id_mapping,                                      \
            dict_remapping,                                  \
            ranking_file,                                    \
            wand_data,                                       \
            iterations,                                      \
            topk,                                            \
            verification,                                    \
            seed                                             \
        );

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_DICT_TYPES);
#undef LOOP_BODY
    } else {
        std::cerr << "ERROR: Unknown dict_type " << dict_type << std::endl;
        return -1;
    }
}

int prefix_search(const std::string& query_file,
                  const std::string& index_fname,
                  const std::string& index_type,
                  const std::string& dict,
                  const std::string& dict_type,
                  const std::string& scheme,
                  const std::string& id_mapping,
                  const std::string& dict_remapping,
                  const std::string& ranking_file,
                  const std::string& wand_data,
                  const int iterations,
                  const int topk,
                  const bool verification,
                  const int seed)
{
    if (false) {
#define LOOP_BODY(R, DATA, T)                               \
    } else if (index_type == BOOST_PP_STRINGIZE(T)) {       \
        BOOST_PP_CAT(T, _index) index(index_fname.c_str()); \
        return prefix_search<BOOST_PP_CAT(T, _index)>(      \
            query_file,                                     \
            index,                                          \
            dict,                                           \
            dict_type,                                      \
            scheme,                                         \
            id_mapping,                                     \
            dict_remapping,                                 \
            ranking_file,                                   \
            wand_data,                                      \
            iterations,                                     \
            topk,                                           \
            verification,                                   \
            seed                                            \
        );

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_INDEX_TYPES);
#undef LOOP_BODY
    } else {
        std::cerr << "ERROR: Unknown index_type " << index_type << std::endl;
        return -1;
    }
}

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
        ("query-file,q", po::value<std::string>()->required(), "File containing queries")
        ("dict-type", po::value<std::string>()->required(), "Type of dict to use")
        ("index-type,t", po::value<std::string>()->required(), "Type of index to use")
        ("input,i", po::value<std::string>()->required(), "Input index")
        ("dictionary,d", po::value<std::string>()->required(), "Dictionary to use")
        ("scheme,s", po::value<std::string>()->default_value("n2asarray"), "Scheme to solve the problem")
        ("id-mapping", po::value<std::string>()->required(), "UserId - SortId mapping")
        ("dict-remapping", po::value<std::string>()->default_value(""), "Dict remapping")
        ("iterations,n", po::value<int>()->default_value(3), "Number of iterations to take")
        ("topk,k", po::value<int>()->default_value(0), "Top k retrieval")
        ("ranking", po::value<std::string>()->default_value(""), "File with ranking scores")
        ("wand", po::value<std::string>()->default_value(""), "WAND auxiliary data")
        ("verification,v", po::value<bool>()->default_value(true), "Verify results")
        ("seed", po::value<int>()->default_value(42), "Seed number")
    )

    return prefix_search(
        vm["query-file"].as<std::string>(),
        vm["input"].as<std::string>(),
        vm["index-type"].as<std::string>(),
        vm["dictionary"].as<std::string>(),
        vm["dict-type"].as<std::string>(),
        vm["scheme"].as<std::string>(),
        vm["id-mapping"].as<std::string>(),
        vm["dict-remapping"].as<std::string>(),
        vm["ranking"].as<std::string>(),
        vm["wand"].as<std::string>(),
        vm["iterations"].as<int>(),
        vm["topk"].as<int>(),
        vm["verification"].as<bool>(),
        vm["seed"].as<int>()
    );
}

#include "ps/optargs.hpp"
#include "ps/graphs/graph_types.hpp"
#include "ps/graphs/serializer.hpp"
#include "ps/indices/index_types.hpp"
#include "ps/utils.hpp"
#include <boost/filesystem.hpp>

using namespace ps;
using namespace ps::graphs;
using namespace ps::indices;
using namespace ps::sequences;

namespace fs = boost::filesystem;

template <typename Index>
int create_index(const std::string& output, int universe)
{
    fs::path path(output.c_str());

    if (!fs::exists(path))
    {
        std::cout << "Saving index to " << output << " with universe=" << universe << std::endl;

        stdin_graph graph;
        options opts(universe);
        Index index(opts, output.c_str());
        graph_serializer<stdin_graph, Index> serializer(graph, index);
    }
    else
    {
        std::cout << "Skipping creation of database. It is already there" << std::endl;
    }

    return 0;
}

template <typename Index>
int create_topk_index(const std::string& output, int universe, const std::string& ranking)
{
    fs::path path(output.c_str());

    if (!fs::exists(path))
    {
        std::cout << "Saving index to " << output << " with universe=" << universe << std::endl;

        stdin_graph graph;
        options opts(universe);
        Index index(opts, output.c_str(), ranking);
        graph_serializer<stdin_graph, Index> serializer(graph, index);
    }
    else
    {
        std::cout << "Skipping creation of database. It is already there" << std::endl;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
        ("index-type,t", po::value<string>()->required(), "Type of index to use")
        ("output,o", po::value<std::string>()->required(), "output database")
        ("universe,u", po::value<int>()->required(), "universe size")
        ("ranking,r", po::value<std::string>()->default_value(""), "ranking")
    )

    int universe = vm["universe"].as<int>();
    std::string index_type = vm["index-type"].as<std::string>();
    std::string output = vm["output"].as<std::string>();
    std::string ranking = vm["ranking"].as<std::string>();

    if (false) {
#define LOOP_BODY(R, DATA, T)                         \
    } else if (index_type == BOOST_PP_STRINGIZE(T)) { \
        return create_index<BOOST_PP_CAT(T, _index)>(output, universe);

    BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SIMPLE_INDEX_TYPES);
#undef LOOP_BODY
#define LOOP_BODY(R, DATA, T)                         \
    } else if (index_type == BOOST_PP_STRINGIZE(T)) { \
        return create_topk_index<BOOST_PP_CAT(T, _index)>(output, universe, ranking);

    BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_TOPK_INDEX_TYPES);
#undef LOOP_BODY
    } else {
        ps::logger() << "ERROR: Unknown index_type " << index_type << std::endl;
    }

    return -1;
}

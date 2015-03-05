#include "ps/optargs.hpp"
#include "ps/graphs/graph_types.hpp"
#include "ps/indices/index_types.hpp"
#include "ps/indices/neighbors.hpp"

using namespace ps;
using namespace ps::graphs;
using namespace ps::indices;

template <typename Index>
int verify_database(const char *input_fname)
{
    stdin_graph graph;
    edges_iterator<stdin_graph> iterator(graph);
    Index index(input_fname);

    std::cout << "Start verification of " << input_fname << std::endl;

    Edges edges1;

    while (iterator.get_next(edges1))
    {
        Edges edges2;
        neighbors(index, edges1.first, edges2);

        if (edges1.second != edges2.second)
        {
            std::cout << "Mismatch found for node " << edges1.first << std::endl;
            std::cout << "Graph contains:    ";

            for (auto it = edges1.second.begin(); it != edges1.second.end(); ++it)
                std::cout << *it << " ";

            std::cout << std::endl;
            std::cout << "Index contains: ";

            for (auto it = edges2.second.begin(); it != edges2.second.end(); ++it)
                std::cout << *it << " ";

            std::cout << std::endl;
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
        ("input,i", po::value<std::string>()->required(), "input index")
        ("index-type,t", po::value<std::string>()->required(), "Type of index to use")
    )

    std::string input_fname = vm["input"].as<std::string>();
    std::string index_type = vm["index-type"].as<std::string>();

    if (false) {
#define LOOP_BODY(R, DATA, T)                            \
    } else if (index_type == BOOST_PP_STRINGIZE(T)) {    \
        return verify_database<BOOST_PP_CAT(T, _index)>( \
            input_fname.c_str()                          \
        );

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_INDEX_TYPES);
#undef LOOP_BODY
    } else {
        cerr << "ERROR: Unknown index_type " << index_type << std::endl;
        return -1;
    }

    return 0;
}

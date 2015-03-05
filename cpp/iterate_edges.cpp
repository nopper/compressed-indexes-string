#include "ps/optargs.hpp"
#include "ps/graphs/graph_types.hpp"

using namespace ps::graphs;

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
    )

    stdin_graph graph;
    edges_iterator<stdin_graph> iterable(graph);

    Edges edges;

    while (iterable.get_next(edges))
    {
    }

    return 0;
}
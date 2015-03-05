#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/cat.hpp>

#include "ps/graphs/edges.hpp"
#include "ps/graphs/edges_iterator.hpp"
#include "ps/graphs/gz_graph.hpp"
#include "ps/graphs/stdin_graph.hpp"

#define PS_GRAPH_TYPES (stdin)(gz)

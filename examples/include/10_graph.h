/*
 * C++ header for a weighted graph used by the path-finding example.
 */
#ifndef EXAMPLES_EXAMPLE10_GRAPH_H
#define EXAMPLES_EXAMPLE10_GRAPH_H

#include <utility>
#include <vector>
/** Type for a nonnegative weighted adjacency list. */
using Graph = std::vector<std::vector<std::pair<int, int>>>;

#endif

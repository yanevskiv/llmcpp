/*
 * C++ file for a generated shortest-path algorithm.
 */

// Example type for a weighted adjacency list.
#include "include/example10_graph.h"

#include <iostream>
#include <vector>

// Find a shortest path through a weighted graph.
__llm__ std::vector<int> shortest_path(const Graph &graph, int source, int destination)
{
    Use Dijkstra's algorithm for nonnegative edge weights. Return the vertex
    sequence from source through destination, inclusive, or an empty vector if
    either vertex is invalid or destination is unreachable.
}

// Run the shortest-path example.
int main()
{
    Graph graph{
        {{1, 7}, {2, 9}, {5, 14}},
        {{0, 7}, {2, 10}, {3, 15}},
        {{0, 9}, {1, 10}, {3, 11}, {5, 2}},
        {{1, 15}, {2, 11}, {4, 6}},
        {{3, 6}, {5, 9}},
        {{0, 14}, {2, 2}, {4, 9}},
    };
    for (int vertex : shortest_path(graph, 0, 4)) {
        std::cout << vertex << ' ';
    }
    std::cout << '\n';
}

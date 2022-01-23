// Interface to the Weighted Graph ADT
// - Vertices are identified by integers between 0 and nV - 1,
//   where nV is the number of vertices in the graph
// - Weights are doubles and must be positive
// - Self-loops are not allowed

// Taken from Lab09 Graph.h, implements a graph ADT

#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>

typedef int Vertex;

typedef struct graph *Graph;

// edges are pairs of vertices (end-points)
typedef struct Edge {
    Vertex v;
    Vertex w;
    double weight;
} Edge;

/**
 * Creates a new instance of a graph
 */
Graph GraphNew(int nV);

/**
 * Frees all memory associated with the given graph
 */
void   GraphFree(Graph g);

/**
 * Returns the number of vertices in the graph
 */
int    GraphNumVertices(Graph g);

/**
 * Inserts  an  edge into a graph. Does nothing if there is already an
 * edge between `e.v` and `e.w`. Returns true if successful, and false
 * if there was already an edge.
  */
bool   GraphInsertEdge(Graph g, Edge e);

/**
 * Removes an edge from a graph. Returns true if successful, and false
 * if the edge did not exist.
 */
bool   GraphRemoveEdge(Graph g, Vertex v, Vertex w);

/**
 * Returns the weight of the edge between `v` and `w` if it exists, or
 * 0.0 otherwise
 */
double GraphIsAdjacent(Graph g, Vertex v, Vertex w);

/**
 * Returns true if the graph contains a cycle, and false otherwise
 */
bool   GraphHasCycle(Graph g);

/**
 * Displays information about the graph
 */
void   GraphShow(Graph g);

// Calculates the W_In value for an edge
double wIn(Graph g, int v, int u);

// Calculates the W_Out value for an edge
double wOut(Graph g, int v, int u);

// Calculates the in degree value for a node
double inDegree(Graph g, int index);

// Calculates the out degree value for a node
double outDegree(Graph g, int index);

// Adjusts the weight of an edge to a double passed in
void adjustWeight(Graph g, int v, int w, double weight);

#endif

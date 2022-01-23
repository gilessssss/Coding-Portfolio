// Implementation of the Weighted Graph ADT
// Uses an adjacency matrix

// Taken from Lab09 Graph.c, implements a graph ADT. Functions added by myself
// have been commented with descriptions in graph.h
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "graph.h"
#include "readData.h"

struct graph {
    int nV;
    double **edges;
    double *pagerank;
};

static bool doHasCycle(Graph g, Vertex v, Vertex prev, bool *visited);
static int  validVertex(Graph g, Vertex v);


Graph GraphNew(int nV) {
    assert(nV > 0);

    Graph g = malloc(sizeof(*g));
    if (g == NULL) {
        fprintf(stderr, "error: out of memory\n");
        exit(EXIT_FAILURE);
    }

    g->nV = nV;
    g->edges = malloc(nV * sizeof(double *));
    if (g->edges == NULL) {
        fprintf(stderr, "error: out of memory\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < nV; i++) {
        g->edges[i] = calloc(nV, sizeof(double));
        if (g->edges[i] == NULL) {
            fprintf(stderr, "error: out of memory\n");
            exit(EXIT_FAILURE);
        }
    }

    return g;
}

void GraphFree(Graph g) {
    for (int i = 0; i < g->nV; i++) {
        free(g->edges[i]);
    }
    free(g->edges);
    free(g);
}

////////////////////////////////////////////////////////////////////////

int GraphNumVertices(Graph g) {
    return g->nV;
}

bool GraphInsertEdge(Graph g, Edge e) {
    assert(validVertex(g, e.v));
    assert(validVertex(g, e.w));
    assert(e.v != e.w);
    assert(e.weight > 0.0);

    if (g->edges[e.v][e.w] == 0.0) {
        g->edges[e.v][e.w] = e.weight;
        return true;
    } else {
        return false;
    }
}

bool GraphRemoveEdge(Graph g, Vertex v, Vertex w) {
    assert(validVertex(g, v));
    assert(validVertex(g, w));

    if (g->edges[v][w] != 0.0) {   // edge e in graph
        g->edges[v][w] = 0.0;
        return true;
    } else {
        return false;
    }
}

double GraphIsAdjacent(Graph g, Vertex v, Vertex w) {
    assert(validVertex(g, v));
    assert(validVertex(g, w));
    
    return g->edges[v][w];
}

bool GraphHasCycle(Graph g) {
    bool *visited = calloc(g->nV, sizeof(bool));
    assert(visited != NULL); // lazy error checking
    
    for (int v = 0; v < g->nV; v++) {
        if (!visited[v] && doHasCycle(g, v, v, visited)) {
            free(visited);
            return true;
        }
    }

    free(visited);
    return false;
}

void GraphShow(Graph g) {
    printf("Number of vertices: %d\n", g->nV);
    for (int v = 0; v < g->nV; v++) {
        for (int w = 0; w < g->nV; w++) {
            if (g->edges[v][w] != 0.0) {
                printf("Edge %d - %d: %lf\n", v, w, g->edges[v][w]);
            }
        }
    }
}

double wIn(Graph g, int v, int u) {
    double numerator = inDegree(g, u);
    double denominator = 0;

    for (int i = 0; i < g->nV; i ++) {
        if (g->edges[v][i] != 0) {
            denominator = denominator + inDegree(g, i);
        }
    }
    return numerator / denominator;
}

double wOut(Graph g, int v, int u) {
    double numerator = outDegree(g, u);
    double denominator = 0;

    for (int i = 0; i < g->nV; i ++) {
        if (g->edges[v][i] != 0) {
            denominator = denominator + outDegree(g, i);
        }
    }
    return numerator / denominator;
}

double inDegree(Graph g, int index) {
    double total = 0;

    for (int i = 0; i < g->nV; i ++) {
        if (g->edges[i][index] != 0) {
            total ++;
        }
    }

    return (total != 0) ? total : 0.5;
}

double outDegree(Graph g, int index) {
    double total = 0;

    for (int i = 0; i < g->nV; i ++) {
        if (g->edges[index][i] != 0) {
            total ++;
        }
    }

    return (total != 0) ? total : 0.5;
}

void adjustWeight(Graph g, int v, int w, double weight) {
    g->edges[v][w] = weight;
    return;
}

static bool doHasCycle(Graph g, Vertex v, Vertex prev, bool *visited) {
    visited[v] = true;
    for (int w = 0; w < g->nV; w++) {
        if (g->edges[v][w] != 0.0) {
            if (!visited[w]) {
                if (doHasCycle(g, w, v, visited)) {
                    return true;
                }
            } else if (w != prev) {
                return true;
            }
        }
    }
    return false;
}
////////////////////////////////////////////////////////////////////////

static int validVertex(Graph g, Vertex v) {
    return v >= 0 && v < g->nV;
}

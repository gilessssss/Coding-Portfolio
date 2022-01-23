// COMP2521 Assignment 2

// Written by: Hugo Giles z5309502
// Date: 10/11/21

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "readData.h"
#include "graph.h"

void addLinks(Graph g, urlList masterList);
void adjustforWIO(Graph g);
int PageRankAlgo(Graph g, double d, double diffPR, int iterations, double *previousResults[]);
void printSortedPageRank(Graph g, urlList masterList, double results[]);


int main(int argc, char *argv[]) {
    // Retrieve parameters passed in
    double d = strtod(argv[1], NULL);
    double diffPR = strtod(argv[2], NULL);
    int maxIterations = atoi(argv[3]);

    // Create a list of all URLs in collection
    urlList collection = getCollection();
    if (urlsInList(collection) == 0) {
        freeURLList(collection);
        return 0;
    }

    // Represent the URLs in a graph and store the Win and Wout values as the
    // weight for each connection between URLs
    Graph g = GraphNew(urlsInList(collection));
    addLinks(g, collection);
    adjustforWIO(g);

    // Setup a 2d array to store all results from each iteration
    double **previousResults = malloc(GraphNumVertices(g) * sizeof(double*)); 
    for (int i = 0; i < GraphNumVertices(g); i ++) {
        previousResults[i] = malloc(maxIterations * sizeof(double));
    }

    // Execute the algorithm storing new result from iterations in new rows
    int finalIteration = PageRankAlgo(g, d, diffPR, maxIterations, previousResults);

    // Retrieve values from the final iteration
    double pageRankResults[GraphNumVertices(g)];
    for (int v = 0; v < GraphNumVertices(g); v ++) {
        pageRankResults[v] = previousResults[v][finalIteration];
    }

    // Print out results
    printSortedPageRank(g, collection, pageRankResults);
    
    // Free all malloc'd memory
    for (int i = 0; i < GraphNumVertices(g); i ++) {
        free(previousResults[i]);
    }
    free(previousResults);
    GraphFree(g);
    freeURLList(collection);

    return 0;
}

// Add all links between urls
void addLinks(Graph g, urlList masterList) {
    URL current = firstURL(masterList);

    // For each URL
    while (current != NULL) {
        int indexFrom = urlIndex(masterList, urlName(current));

        char filename[104];
        strcpy(filename, urlName(current));
        strcat(filename, ".txt");

        // Read in file and fgets until we reach the end of the first section
        FILE * currentFile = fopen(filename, "r");
        char currentStream[1000];
        while (fgets(currentStream, sizeof(currentStream), currentFile) != NULL) {
            if (strcmp(currentStream, "#end Section-1\n")) {
                break;
            }
        }

        // fscanf all urls within section 1 and add an edge between the file's url
        // and the url in the file
        while (fscanf(currentFile, "%s", currentStream) == 1) {
            if (strcmp(currentStream, "#end") == 0) {
                break;
            }
            int indexTo = urlIndex(masterList, currentStream);

            if (indexTo == indexFrom) {
                continue;
            }
            
            Edge newLink;
            newLink.v = indexFrom;
            newLink.w = indexTo;
            newLink.weight = 1;

            GraphInsertEdge(g, newLink);
        }
        fclose(currentFile);
        current = nextURL(current);
    }
}

void adjustforWIO(Graph g) {
    // Add Win * Wout value for all edges
    for (int v = 0; v < GraphNumVertices(g); v ++) {
        for (int w = 0; w < GraphNumVertices(g); w ++) {
            if (GraphIsAdjacent(g, v, w) != 0) {
                double in = wIn(g, v, w);
                double out = wOut(g, v, w);
                adjustWeight(g, v, w, in * out);
            }
        }
    }
    return;
}

// Implementing the given pagerank algorithm
int PageRankAlgo(Graph g, double d, double diffPR, int maxIterations, double *previousResults[]) {
    int N = GraphNumVertices(g);

    // Pseudocode given was essentially translated into C
    for (int i = 0; i < N; i ++) {
        previousResults[i][0] = (1 / (float)N);
    }

    
    int iteration = 0;
    double diff = diffPR;
    while ((iteration < maxIterations) && (diff >= diffPR)) {
        int t = iteration;

        // For each url
        for (int i = 0; i < N; i ++) {
            // Calculate first term of equation
            double firstPart = (1 - d) / (float)N;
            
            // Calculate the sigma sum
            double sigma = 0;
            for (int j = 0; j < N; j ++) {
                double wInOut = GraphIsAdjacent(g, j, i);
                if (wInOut != 0) {
                    sigma += wInOut * previousResults[j][t];
                }
            }
            previousResults[i][t + 1] = firstPart + (d * sigma);
        }

        // Finding new diff
        diff = 0;
        for (int i = 0; i < N; i ++) {
            diff += fabs(previousResults[i][t + 1] - previousResults[i][t]);
        }
        iteration ++;
    }
    return iteration;
}

// Printing results to pageranList.txt in the required format
void printSortedPageRank(Graph g, urlList masterList, double results[]) {
    FILE * file = fopen("pagerankList.txt", "w");
    for (int i = 0; i < GraphNumVertices(g); i ++) {
        
        int currentMaxIndex = 0;
        double currentMax = 0;

        for (int p = 0; p < GraphNumVertices(g); p ++) {
            // Order by pagerank and alphabetical if pageranks are equal
            if (results[p] > currentMax) {
                currentMax = results[p];
                currentMaxIndex = p;
            }
            else if ((strcmp(urlName(nthURL(masterList, p)),
                urlName(nthURL(masterList, currentMaxIndex))) < 0) &&
                (results[p] == currentMax)) {
                currentMax = results[p];
                currentMaxIndex = p;
            }
        }
        results[currentMaxIndex] = 0;
        char *url = urlName(nthURL(masterList, currentMaxIndex));
        double out = outDegree(g, currentMaxIndex);
        fprintf(file, "%s, %.0lf, %.7lf\n", url, out, currentMax);
    }
    fclose(file);
    return;
}

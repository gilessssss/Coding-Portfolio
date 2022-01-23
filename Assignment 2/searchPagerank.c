// COMP2521 Assignment 2

// Written by: Hugo Giles z5309502
// Date: 11/11/21

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "readData.h"
#include "graph.h"

int main(int argc, char *argv[]) {
    // Retrieve a list of all the matched URLs
    urlList matchedURLs =  findMatchedUrls(argv, argc);
    if (urlsInList(matchedURLs) == 0) {
        freeURLList(matchedURLs);
        return 0;
    }
    // Sort this in a stable manner alphabetically, by pagerank and by the matching
    // number in order to fit specification.
    sortListAlphabetically(matchedURLs);
    sortListByPageRank(matchedURLs);
    sortListByMatching(matchedURLs);
    // Output the firts 30 URLs on the list
    URL current = firstURL(matchedURLs);
    for (int i = 0; i < 30; i++) {
        if (current == NULL) {
            break;
        }
        printf("%s\n", urlName(current));
        current = nextURL(current);
    }

    // Free any malloc'd memory
    freeURLList(matchedURLs);
    return 0;
}


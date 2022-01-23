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

struct URL {
    char name[100];
    int keywords;
    float pagerank; // Only used for Part 2 page ranks
    URL next;
};

struct urlList {
    int nURL;
    URL first;
    URL last;
};

// Reads collection and adds all URLs to a urlList
urlList getCollection(void) {
    FILE * collection = fopen("collection.txt", "r");

    if (collection == NULL) {
        return NULL;
    }

    urlList collectionList = malloc(sizeof(*collectionList));
    collectionList->first = NULL;
    collectionList->last = NULL;
    collectionList->nURL = 0;

    char url[100];
    while (fscanf(collection, "%s", url) == 1) {
        URL addURL = malloc(sizeof(*addURL));
        strcpy(addURL->name, url);
        addURL->next = NULL;

        if (collectionList->first == NULL) {
            collectionList->first = addURL;
        }
        else {
            collectionList->last->next = addURL;
        }
        collectionList->last = addURL;
        collectionList->nURL ++;
    }

    fclose(collection);
    return collectionList;
}

// Frees a urlList
void freeURLList(urlList list) {
    if (list->first != NULL) {
        freeURLRecursively(list->first);
    }
    free(list);
    return;
}

// Used to help free a list of urls
void freeURLRecursively(URL currentURL) {
    if (currentURL->next != NULL) {
        freeURLRecursively(currentURL->next);
    }
    free(currentURL);
    return;
}

// Returns the numbers of urls in the list
int urlsInList(urlList list) {
    return list->nURL;
}

// Used to print a list of urls
void printList(urlList list) {
    URL current = list->first;

    while (current != NULL) {
        printf("%s ", current->name);

        printf("%.7lf ", current->pagerank);

        printf("%d\n", current->keywords);



        current = current->next;
    }
    return;
}

// Returns the index of the url in a list
int urlIndex(urlList list, char *url) {
    URL current = list->first;

    for (int i = 0; current != NULL; i ++) {
        if (strcmp(current->name, url) == 0) {
            return i;
        }
        current = current->next;
    }
    return -1;
}

// Returns the first url of a list
URL firstURL(urlList list) {
    return list->first;
}

// Returns the next url in a list
URL nextURL(URL current) {
    return current->next;
}

// Returns the name of a url node
char *urlName(URL current) {
    return current->name;
}

// Returns the pagerank stored with the url
double urlPagerank(URL current) {
    return current->pagerank;
}

// Returns the number of matches a URL has had for searchPagerank
int urlMatches(URL current) {
    return current->keywords;
}

// Returns the nth URL in a list
URL nthURL(urlList list, int n) {
    URL current = list->first;
    for (int i = 0; i < n; i ++) {
        current = current->next;
    }
    return current;
}

// Adds a URL to the list of matched URLs if it comes after a requested search
// term
urlList findMatchedUrls(char *queryWords[], int nQueries) {
    FILE * file = fopen("invertedIndex.txt", "r");

    if (file == NULL) {
        return NULL;
    }

    urlList matchedURLs = malloc(sizeof(*matchedURLs));
    matchedURLs->first = NULL;
    matchedURLs->last = NULL;
    matchedURLs->nURL = 0;

    char currentLine[1000];

    while (fgets(currentLine, sizeof(currentLine), file) != NULL) {
        char * token = strtok(currentLine, " ");
        if (!isURLQueryWord(token, queryWords, nQueries)) {
            continue;
        }
        
        token = strtok(NULL, " ");
        while (strcmp(token, "\n") != 0) {
            addURLtoMatchedList(token, matchedURLs);
            token = strtok(NULL, " ");
        }
    }
    fclose(file);
    return matchedURLs;
}

// Checks whether a word is part of the requested query words
bool isURLQueryWord(char *url, char *queryWords[], int nQueries) {
    for (int i = 1; i < nQueries; i ++) {
        if (strcmp(url, queryWords[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Adds a URL to a matched list or increases its keywords value if already there
void addURLtoMatchedList(char *url, urlList matchedList) {
    int index = urlIndex(matchedList, url);
    if (index != -1) {
        URL current = nthURL(matchedList, index);
        current->keywords ++;
        return;
    }
    URL addURL = malloc(sizeof(*addURL));
    addURL->keywords = 1;
    strcpy(addURL->name, url);
    addURL->next = NULL;

    if (matchedList->first == NULL) {
        matchedList->first = addURL;
        matchedList->last = addURL;
    }
    else {
        matchedList->last->next = addURL;
        matchedList->last = addURL;
    }
    matchedList->nURL ++;
    return;
}

// Sorts a list alphabetically via insertion sort
void sortListAlphabetically(urlList list) {
    urlList sorted = malloc(sizeof(*sorted));
    sorted->first = NULL;
    sorted->last = NULL;
    
    URL current = list->first;

    while (current != NULL) {
        URL next = current->next;
        if ((sorted->first == NULL) || (strcmp(sorted->first->name, current->name)) > 0) {
            current->next = sorted->first;
            sorted->first = current;
            if (sorted->last == NULL) {
                sorted->last = current;
            }
        }
        else {
            URL temp = sorted->first;
            while ((temp->next != NULL) && (strcmp(temp->next->name, current->name) <= 0)) {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
            if (sorted->last == temp) {
                sorted->last = current;
            }
        }
        current = next;
    }
    list->first = sorted->first;
    list->last = sorted->last;
    free(sorted);
    return;
}

// Sorts a list by pagerank via insertion sort
void sortListByPageRank(urlList list) {
    retrievePageranks(list);
    
    urlList sorted = malloc(sizeof(*sorted));
    sorted->first = NULL;
    sorted->last = NULL;

    URL current = list->first;

    while (current != NULL) {
        URL next = current->next;
        if ((sorted->first == NULL) || (sorted->first->pagerank < current->pagerank)) {
            current->next = sorted->first;
            sorted->first = current;
            if (sorted->last == NULL) {
                sorted->last = current;
            }
        }
        else {
            URL temp = sorted->first;
            while ((temp->next != NULL) && (temp->next->pagerank >= current->pagerank)) {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
            if (sorted->last == temp) {
                sorted->last = current;
            }
        }
        current = next;
    }
    list->first = sorted->first;
    list->last = sorted->last;
    free(sorted);
    return;
}

// Retrieves pageranks from pagerankList.txt
void retrievePageranks(urlList list) {
    FILE * file = fopen("pagerankList.txt", "r");
    if (file == NULL) {
        return;
    }

    char url[100];
    char out[100];
    char pageRankS[100];

    while (fscanf(file, "%s %s %s", url, out, pageRankS) == 3) {
        url[strlen(url) - 1] = '\0';

        double pageRank = strtod(pageRankS, NULL);
        int index = urlIndex(list, url);

        if (index == -1) {
            continue;
        }
        URL target = nthURL(list, index);
        target->pagerank = pageRank;
    }
    fclose(file);
    return;
}

// Sorts a list by it's matching quantity via insertion sort
void sortListByMatching(urlList list) {
    urlList sorted = malloc(sizeof(*sorted));
    sorted->first = NULL;
    sorted->last = NULL;

    URL current = list->first;

    while (current != NULL) {
        URL next = current->next;
        if ((sorted->first == NULL) || (sorted->first->keywords < current->keywords)) {
            current->next = sorted->first;
            sorted->first = current;
            if (sorted->last == NULL) {
                sorted->last = current;
            }
        }
        else {
            URL temp = sorted->first;
            while ((temp->next != NULL) && (temp->next->keywords >= current->keywords)) {
                temp = temp->next;
            }
            current->next = temp->next;
            temp->next = current;
            if (sorted->last == temp) {
                sorted->last = current;
            }
        }
        current = next;
    }
    list->first = sorted->first;
    list->last = sorted->last;
    free(sorted);
    return;
}

// Sets all keyword values to a value, used to test
void setAllKey(urlList list, int value) {
    URL current = list->first;
    while (current != NULL) {
        current->keywords = value;
        current = current->next;
    }
    return;
}

// Retrieves URLs from files passed into scaledFootrule
urlList retrieveURLs(char *files[], int nFiles) {
    urlList allURLs = malloc(sizeof(*allURLs));

    allURLs->first = NULL;
    allURLs->last = NULL;
    allURLs->nURL = 0;

    for (int i = 1; i < nFiles; i ++) {
        FILE * file = fopen(files[i], "r");

        char url[104];
        while (fscanf(file, "%s", url) == 1) {
            addURLtoMatchedList(url, allURLs);
        }
        fclose(file);
    }
    return allURLs;
}

// Creates an array ranked lists based on the files passed into scaledFootrule
void setupRankedLists(int *rankedLists[], char *files[], int nFiles, urlList masterList) {
    for (int r = 1; r < nFiles; r ++) {
        char current[104];

        for (int i = 0; i < urlsInList(masterList); i++) {
            FILE * file = fopen(files[r], "r");
            int p = 0;

            while (fscanf(file, "%s", current) == 1) {
                int index = urlIndex(masterList, current);
                rankedLists[r - 1][index] = p + 1;
                p ++;
            }
            fclose(file);
        }
    }
    return;
}

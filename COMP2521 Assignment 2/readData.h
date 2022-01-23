// COMP2521 Assignment 2

// Written by: Hugo Giles z5309502
// Date: 10/11/21

#ifndef READDATA_H
#define READDATA_H

typedef struct urlList *urlList;

typedef struct URL *URL;

urlList getCollection(void);

void freeURLList(urlList list);

void freeURLRecursively(URL currentURL);

int urlsInList(urlList list);

void printList(urlList list);

int urlIndex(urlList list, char *url);

URL firstURL(urlList list);

URL nextURL(URL current);

char *urlName(URL current);

double urlPagerank(URL current);

int urlMatches(URL current);

URL nthURL(urlList list, int n);

urlList findMatchedUrls(char *queryWords[], int nQueries);

bool isURLQueryWord(char *url, char *queryWords[], int nQueries);

void addURLtoMatchedList(char *url, urlList matchedList);

void sortListAlphabetically(urlList list);

void sortListByPageRank(urlList list);

void retrievePageranks(urlList list);

void sortListByMatching(urlList list);

void setAllKey(urlList list, int value);

urlList retrieveURLs(char *files[], int nFiles);

void setupRankedLists(int *rankedLists[], char *files[], int nFiles, urlList masterList);

#endif
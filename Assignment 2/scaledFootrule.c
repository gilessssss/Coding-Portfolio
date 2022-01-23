// COMP2521 Assignment 2

// Written by: Hugo Giles z5309502
// Date: 12/11/21

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "readData.h"
#include "graph.h"
#include "PQ.h"

int sizeOfList(int *rankedLists[], int listIndex, int maxSize);
double footruleForURLInPosition(int url, int position, urlList masterList, int *rankedLists[], int nLists);
double **createMatrix(urlList masterList, int *rankedLists[], int nLists);
double **copyMatrix(int N, double *originalMatrix[]);
void subtractRowMinimum(double *matrix[], int N);
void subtractColMinimum(double *matrix[], int N);
int *smartAlgorithmVTwo(double *matrix[], int N, double *originalMatrix[]);
int optimiseMatrix(double *matrix[], int N, bool *coveredRows, bool *coveredCols);
int MaxNumberOfZeroInRow(double *matrix[], int N, bool *coveredRows, bool *coveredCols);
int MaxNumberOfZeroInCol(double *matrix[], int N, bool *coveredRows, bool *coveredCols);
int zeroesInMatrix(double *matrix[], int N, bool *coveredRows, bool *coveredCols);
void coverMaxRow(double *matrix[], int N, bool *coveredRows, bool *coveredCols);
void coverMaxCol(double *matrix[], int N, bool *coveredRows, bool *coveredCols);
int linesRequired(bool *coveredRows, bool *coveredCols, int N);
double smallestUncovered(double *matrix[], int N, bool *coveredRows, bool *coveredCols);
void adjustmentStep(double *matrix[], int N, bool *coveredRows, bool *coveredCols, double smallest);
void uncover(int N, bool *coveredRows, bool *coveredCols);
int **optimalPaths(int N, double *matrix[]);
int *finalSolve(int N, int *possible[], double *matrix[]);
void clearExtraRow(int N, int *possible[], int i, int p);
void clearExtraCol(int N, int *possible[], int i, int p);
int minAvailableRow(int *possible[], int N);
int minAvailableCol(int *possible[], int N);
void selectFirstOfMinRow(int *possible[], int N);
void selectFirstOfMinCol(int *possible[], int N);
bool isSolutionValid(int N, int *possible[]);
void fixMissingValues(double *matrix[], int *possible[], int *solution, int N);
bool floatsEqual(float floatOne, float floatTwo);

// An Explanation of my "smart" Algorithm
// By finding the "cost" of having a URL in a position I can store all these values
// within a N * N matrix i.e. N urls in N positions. Then we can find the optimal
// solution by finding a unique URL for each position (no URL or position is used twice).
//
// While initially seeming complicated and hard to achieve, I applied the same 
// method I have used previously in some finance questions to minimize the worker
// cost while covering all the jobs and where no worker can complete more than
// one job. I incorporated this technique I had done many times on paper into code.
//
// The technique works by:
// 1. For each row, minusing the minimum value in the row from each cell in the row.
//
// 2. For each column, minusing the minimum value in the column from each cell
// in the column.
//
// 3. Now, cover all zeroes using the minimum amount of rows or columns possible.
//
// 4. If the number of rows and columns used to cover all zeroes is equal to N,
// then we have found the optimal solution amongst the current zeroes. If not,
// subtract the minimum value of the uncovered values from all uncovered values.
// Add this minimum uncovered value to the cells which are covered by both a row
// and column. Then, uncover all rows and columns and go back to step 3.
//
// 5. Once we optimise the matrix, we then transfer our results to another matrix.
// All zeroes from the previous matrix are now 1 and all other values are 0.
// We then find the row that contains the minimum number of 1's and the column
// that contains the minimum number of 1's. From the minimum row/column found
// (whichever row/column has less 1's) we find the first 1 in that row/column. 
// Remove all other 1's within the row and column that contains that "first 1"
// we found (as each solution must have a unique row and column). We continue
// this until we find N unique cells (i.e. one for each row and column) and have
// completed our final solution.
//
// 6. For some large N, usually >10, there can be some solutions found without
// all rows and columns satisfied. In this case we implement a priority queue to
// find the "cheapest" way to complete our final solution.

int main(int argc, char *argv[]) {
    // Retrieve a complete list of all urls referenced by all files passed in
    urlList masterList = retrieveURLs(argv, argc);
    int N = urlsInList(masterList);
    sortListAlphabetically(masterList);

    // Give each URL an index and format the ranked order for each file
    int **rankedLists = malloc((argc - 1) * sizeof(int*)); 
    for (int i = 0; i < argc - 1; i ++) {
        rankedLists[i] = malloc(N * sizeof(int));
        for (int p = 0; p < N; p ++) {
            rankedLists[i][p] = 0;
        }
    }
    setupRankedLists(rankedLists, argv, argc, masterList);

    // Store the 'cost' of putting a url in a position in the list with an N * N
    // where N is the number of URLs
    double **matrix = createMatrix(masterList, rankedLists, argc - 1);
    double **adjustableMatrix = copyMatrix(N, matrix);

    // Using smart algorithm find the optimal order of the final list
    int *finalSolution = smartAlgorithmVTwo(adjustableMatrix, N, matrix);

    // Calculate the total scaled-footrule distance for the optimal solution
    double distance = 0;
    for (int i = 0; i < N; i ++) {
        distance += matrix[finalSolution[i] - 1][i];
    }

    // Print out our results in the required format
    printf("%.6lf\n", distance);
    for (int i = 1; i <= N; i ++) {
        int p = 0;
        while (finalSolution[p] != i) {
            p ++;
        }
        char *url = urlName(nthURL(masterList, p));
        printf("%s\n", url);
    }


    // Free all variables that were malloc'd
    freeURLList(masterList);

    for (int i = 0; i < argc - 1; i ++) {
        free(rankedLists[i]);
    }
    free(rankedLists);

    for (int i = 0; i < N; i ++) {
        free(matrix[i]);
        free(adjustableMatrix[i]);
    }
    free(matrix);
    free(adjustableMatrix);
    free(finalSolution);

    return 0;
}

int *smartAlgorithmVTwo(double *matrix[], int N, double *originalMatrix[]) {
    // Step 1 and 2, minusing minimum values in all rows and then columns
    subtractRowMinimum(matrix, N);
    subtractColMinimum(matrix, N);

    // Representing covered rows and columns in bool arrays
    bool *coveredRows = malloc(N * sizeof(bool));
    bool *coveredCols = malloc(N * sizeof(bool));
    for (int i = 0; i < N; i ++) {
        coveredRows[i] = false;
        coveredCols[i] = false;
    }

    // Step 3 and 4: 
    while (1) {
        int lines = optimiseMatrix(matrix, N, coveredRows, coveredCols);
        if (lines == N) {
            break;
        }
        double smallest = smallestUncovered(matrix, N, coveredRows, coveredCols);
        adjustmentStep(matrix, N, coveredRows, coveredCols, smallest);
        uncover(N, coveredRows, coveredCols);
    }

    // Step 5 and 6: transferring our matrix and then solving
    int **possible = optimalPaths(N, matrix);
    int *solution = finalSolve(N, possible, originalMatrix);

    // Free all memory malloc'd
    free(coveredRows);
    free(coveredCols);

    for (int i = 0; i < N; i ++) {
        free(possible[i]);
    }
    free(possible);
    
    
    return solution;
}

// Find the total distance for a url in a position (i.e. all files are considered)
double footruleForURLInPosition(int url, int position, urlList masterList, int *rankedLists[], int nLists) {
    double totalDistance = 0;
    int n = urlsInList(masterList);
    for (int i = 0; i < nLists; i ++) {
        if (rankedLists[i][position - 1] == 0) {
            continue;
        }
        int cardinality = sizeOfList(rankedLists, i, n);
        
        totalDistance += fabs((rankedLists[i][position - 1] / (float)cardinality) - (url / (float)n));
    }
    return totalDistance;
}

// Create a N * N matrix full with the distances of having a url in a position
double **createMatrix(urlList masterList, int *rankedLists[], int nLists) {
    int N = urlsInList(masterList);

    double **matrix = malloc(N * sizeof(double*)); 
    for (int i = 0; i < N; i ++) {
        matrix[i] = malloc(N * sizeof(double));
    }

    for (int url = 1; url <= N; url ++) {
        for (int position = 1; position <= N; position ++) {
            double resultForPosition =
            footruleForURLInPosition(url, position, masterList, rankedLists, nLists);
            matrix[url - 1][position - 1] = resultForPosition;
        }
    }
    return matrix;
}

// Copy an N *N matrix and return it
double **copyMatrix(int N, double *originalMatrix[]) {
    double **matrix = malloc(N * sizeof(double*)); 
    for (int i = 0; i < N; i ++) {
        matrix[i] = malloc(N * sizeof(double));
    }

    for (int i = 0; i < N; i ++) {
        for (int p = 0; p < N; p ++) {
            matrix[i][p] = originalMatrix[i][p];
        }
    }
    return matrix;
}

// Finds the size of a ranked list (how many values are in the list)
int sizeOfList(int *rankedLists[], int listIndex, int maxSize) {
    int p = 0;
    for (int i = 0; i < maxSize; i ++) {
        if (rankedLists[listIndex][i] != 0) {
            p ++;
        }
    }
    return p;
}

// Subtract the minimum value in a row from the row and do this for all rows
void subtractRowMinimum(double *matrix[], int N) {
    for (int i = 0; i < N; i ++) {
        double rowMin = 999999;
        for (int p = 0; p < N; p ++) {
            double current = matrix[i][p];
            if (current < rowMin) {
                rowMin = current;
            }
        }
        for (int p = 0; p < N; p ++) {
            matrix[i][p] -= rowMin;
        }
    }
    return;
}

// Subtract the minimum value in a column from the column and do this for all columns
void subtractColMinimum(double *matrix[], int N) {
    for (int i = 0; i < N; i ++) {
        double colMin = 999999;
        for (int p = 0; p < N; p ++) {
            double current = matrix[p][i];
            if (current < colMin) {
                colMin = current;
            }
        }
        for (int p = 0; p < N; p ++) {
            matrix[p][i] -= colMin;
        }
    }
    return;
}

// While there are zeroes in the matrix try and find the row or column with
// the largest amount of zeros and cover this, return the number of lines required
// to completely cover the zeroes
int optimiseMatrix(double *matrix[], int N, bool *coveredRows, bool *coveredCols) {
    while (zeroesInMatrix(matrix, N, coveredRows, coveredCols) != 0) {
        int maxRowZeroes = MaxNumberOfZeroInRow(matrix, N, coveredRows, coveredCols);
        int maxColZeroes = MaxNumberOfZeroInCol(matrix, N, coveredRows, coveredCols);
        if (maxRowZeroes > maxColZeroes) {
            coverMaxRow(matrix, N, coveredRows, coveredCols);
        }
        else {
            coverMaxCol(matrix, N, coveredRows, coveredCols);
        }
    }

    return linesRequired(coveredRows, coveredCols, N);
}

// Find the maximum number of zeroes in a row in a matrix.
int MaxNumberOfZeroInRow(double *matrix[], int N, bool *coveredRows, bool *coveredCols) {
    int maxZeroes = 0;
    for (int i = 0; i < N; i ++) {
        int zeroesInRow = 0;
        for (int p = 0; p < N; p ++) {
            if (coveredRows[i] || coveredCols[p]) {
                continue;
            }
            if (floatsEqual(matrix[i][p], 0)) {
                zeroesInRow ++;
            }
        }
        if (zeroesInRow > maxZeroes) {
            maxZeroes = zeroesInRow;
        }
    }
    return maxZeroes;
}

// Find the maximum number of zeroes in a column in a matrix.
int MaxNumberOfZeroInCol(double *matrix[], int N, bool *coveredRows, bool *coveredCols) {
    int maxZeroes = 0;
    for (int i = 0; i < N; i ++) {
        int zeroesInCol = 0;
        for (int p = 0; p < N; p ++) {
            if (coveredRows[p] || coveredCols[i]) {
                continue;
            }
            if (floatsEqual(matrix[p][i], 0)) {
                zeroesInCol ++;
            }
        }
        if (zeroesInCol > maxZeroes) {
            maxZeroes = zeroesInCol;
        }
    }
    return maxZeroes;
}

// Find the number of zeroes not currently covered in a matrix.
int zeroesInMatrix(double *matrix[], int N, bool *coveredRows, bool *coveredCols) {
    int zeroes = 0;
    for (int i = 0; i < N; i ++) {
        for (int p = 0; p < N; p ++) {
            if (coveredRows[i] || coveredCols[p]) {
                continue;
            }
            if (floatsEqual(matrix[i][p], 0)) {
                zeroes ++;
            }
        }
    }
    return zeroes;
}

// Cover the row with the most zeroes in a matrix
void coverMaxRow(double *matrix[], int N, bool *coveredRows, bool *coveredCols) {
    int maxRow = 0;
    int maxZeroes = 0;
    for (int i = 0; i < N; i ++) {
        int zeroesInRow = 0;
        for (int p = 0; p < N; p ++) {
            if (coveredRows[i] || coveredCols[p]) {
                continue;
            }
            if (floatsEqual(matrix[i][p], 0)) {
                zeroesInRow ++;
            }
        }
        if (zeroesInRow > maxZeroes) {
            maxZeroes = zeroesInRow;
            maxRow = i;
        }
    }
    coveredRows[maxRow] = true;
    return;
}

// Cover the column with the most zeroes in a matrix
void coverMaxCol(double *matrix[], int N, bool *coveredRows, bool *coveredCols) {
    int maxCol = 0;
    int maxZeroes = 0;
    for (int i = 0; i < N; i ++) {
        int zeroesInCol = 0;
        for (int p = 0; p < N; p ++) {
            if (coveredRows[p] || coveredCols[i]) {
                continue;
            }
            if (floatsEqual(matrix[p][i], 0)) {
                zeroesInCol ++;
            }
        }
        if (zeroesInCol > maxZeroes) {
            maxZeroes = zeroesInCol;
            maxCol = i;
        }
    }
    coveredCols[maxCol] = true;
    return;
}

// Returns the total number of covered rows and columns
int linesRequired(bool *coveredRows, bool *coveredCols, int N) {
    int count = 0;
    for (int i = 0; i < N; i ++) {
        if (coveredRows[i]) {
            count ++;
        }
        if (coveredCols[i]) {
            count ++;
        }
    }
    return count;
}

// Finds the smallest value that is not covered
double smallestUncovered(double *matrix[], int N, bool *coveredRows, bool *coveredCols) {
    double smallest = 999999;
    for (int i = 0; i < N; i ++) {
        for (int p = 0; p < N; p ++) {
            if (coveredRows[i] || coveredCols[p]) {
                continue;
            }
            if (smallest > matrix[i][p]) {
                smallest = matrix[i][p];
            }
        }
    }
    return smallest;
}

// Subtracts the value from all non-covered cells and adds the value to all 
// cells that are covered by both row and column
void adjustmentStep(double *matrix[], int N, bool *coveredRows, bool *coveredCols, double smallest) {
    // subtract from all uncovered
    for (int i = 0; i < N; i ++) {
        for (int p = 0; p < N; p ++) {
            if (coveredRows[i] || coveredCols[p]) {
                continue;
            }
            matrix[i][p] -= smallest;
        }
    }

    // Add to doubly covered elements
    for (int i = 0; i < N; i ++) {
        for (int p = 0; p < N; p ++) {
            if (!coveredCols[p] || !coveredRows[i]) {
                continue;
            }
            matrix[i][p] += smallest;
        }
    }
    return;
}

// Uncovers all rows and columns
void uncover(int N, bool *coveredRows, bool *coveredCols) {
    for (int i = 0; i < N; i ++) {
        coveredRows[i] = false;
        coveredCols[i] = false;
    }
    return;
}

// Returns a matrix with all the possible values that may be used in the final
// solution represent as 1 and everything else as 0
int **optimalPaths(int N, double *matrix[]) {
    int **possible = malloc(N * sizeof(int*)); 
    for (int i = 0; i < N; i ++) {
        possible[i] = malloc(N * sizeof(int));
        for (int p = 0; p < N; p ++) {
            possible[i][p] = 0;
        }
    }

    for (int i = 0; i < N; i ++) {
        for (int p = 0; p < N; p ++) {
            if (floatsEqual(matrix[i][p], 0)) {
                possible[i][p] = 1;
            }
        }
    }
    return possible;
}

// From an optimised matrix completes step 5 in order to find the optimal solution
// required. Any missing values are then filled.
int *finalSolve(int N, int *possible[], double *matrix[]) {
    // Find the final solution outlined in step 5
    int found = 0;
    while (found < N) {
        int minRow = minAvailableRow(possible, N);
        int minCol = minAvailableCol(possible, N);

        if (minRow <= minCol) {
            selectFirstOfMinRow(possible, N);
            found ++;
        }
        else {
            selectFirstOfMinCol(possible, N);
            found ++;
        }
    }

    int *finalSolution = malloc(N * sizeof(int));
    for (int p = 0; p < N; p ++) {
        for (int i = 0; i < N; i ++) {
            if (possible[i][p] == 2) {
                finalSolution[p] = i + 1;
                possible[i][p] = i + 1;
                break;
            }
        }
    }

    // If this solution is valid return it
    if (isSolutionValid(N, possible)) {
        return finalSolution;
    }

    // else fix missing values before returning
    fixMissingValues(matrix, possible, finalSolution, N);

    return finalSolution;
}

// Clear all values in a row (set them to 0) that aren't our selected values
void clearExtraRow(int N, int *possible[], int i, int p) {
    for (int r = 0; r < N; r++) {
        if (r == p) {
            continue;
        }
        if (possible[i][r] == 1) {
            possible[i][r] = 0;
        }
    }
    return;
}

// Clear all values in a column (set them to 0) that aren't our selected values
void clearExtraCol(int N, int *possible[], int i, int p) {
    for (int r = 0; r < N; r++) {
        if (r == i) {
            continue;
        }
        if (possible[r][p] == 1) {
            possible[r][p] = 0;
        }
    }
    return;
}

// Find the minimum number of 1's in a row in a matrix
int minAvailableRow(int *possible[], int N) {
    int minAvailable = 999999;
    for (int i = 0; i < N; i ++) {
        int availableInRow = 0;
        for (int p = 0; p < N; p ++) {
            if (possible[i][p] == 1) {
                availableInRow ++;
            }
        }
        if ((minAvailable > availableInRow) && (availableInRow != 0)) {
            minAvailable = availableInRow;
        }
    }
    return minAvailable;
}

// Find the minimum number of 1's in a column in a matrix
int minAvailableCol(int *possible[], int N) {
    int minAvailable = 999999;
    for (int i = 0; i < N; i ++) {
        int availableInCol = 0;
        for (int p = 0; p < N; p ++) {
            if (possible[p][i] == 1) {
                availableInCol ++;
            }
        }
        if ((minAvailable > availableInCol) && (availableInCol != 0)) {
            minAvailable = availableInCol;
        }
    }
    return minAvailable;
}

// Selects the first 1 in a row and clears all other 1s with the same row or 
// column
void selectFirstOfMinRow(int *possible[], int N) {
    int row = 0;
    int minAvailable = 999999;
    for (int i = 0; i < N; i ++) {
        int availableInRow = 0;
        for (int p = 0; p < N; p ++) {
            if (possible[i][p] == 1) {
                availableInRow ++;
            }
        }
        if ((minAvailable > availableInRow) && (availableInRow != 0)) {
            minAvailable = availableInRow;
            row = i;
        }
    }

    for (int p = 0; p < N; p ++) {
        if (possible[row][p] == 1) {
            possible[row][p] = 2;
            clearExtraRow(N, possible, row, p);
            clearExtraCol(N, possible, row, p);
            break;
        }
    }
    return;
}

// Selects the first 1 in a column and clears all other 1s with the same row or 
// column
void selectFirstOfMinCol(int *possible[], int N) {
    int col = 0;
    int minAvailable = 999999;
    for (int i = 0; i < N; i ++) {
        int availableInCol = 0;
        for (int p = 0; p < N; p ++) {
            if (possible[p][i] == 1) {
                availableInCol ++;
            }
        }
        if ((minAvailable > availableInCol) && (availableInCol != 0)) {
            minAvailable = availableInCol;
            col = i;
        }
    }

    for (int i = 0; i < N; i ++) {
        if (possible[i][col] == 1) {
            possible[i][col] = 2;
            clearExtraRow(N, possible, i, col);
            clearExtraCol(N, possible, i, col);
            break;
        }
    }
    return;
}

// Returns whether a solution is currently valid by counting the selected values
// and seeing if they're equal to N
bool isSolutionValid(int N, int *possible[]) {
    int count = 0;
    for (int p = 0; p < N; p ++) {
        for (int i = 0; i < N; i ++) { 
            if (possible[i][p] != 0) {
                count ++;
            }
        }
    }
    return (N == count) ? true : false;
}

// Fixes all missing values in a solution by implementing a priority queue
// and selecting the combination which will add the least distance
void fixMissingValues(double *matrix[], int *possible[], int *solution, int N) {
    bool missingURLs[N];
    bool missingPositions[N];

    int missingTotal = N;

    // Retrieves missing URLs
    for (int i = 0; i < N; i ++) {
        bool url = true;
        for (int p = 0; p < N; p ++) {
            if (possible[i][p] != 0) {
                missingTotal --;
                url = false;
                break;
            }
        }
        missingURLs[i] = url;
    }

    // Retrieves missing positions
    for (int i = 0; i < N; i ++) {
        bool position = true;
        for (int p = 0; p < N; p ++) {
            if (possible[p][i] != 0) {
                position = false;
                break;
            }
        }
        missingPositions[i] = position;
    }

    // Adds all URL position combinations to add a priority queue
    PQ missingPossibilities = PQNew();

    for (int i = 0; i < N; i ++) {
        for (int p = 0; p < N; p ++) {
            if (missingURLs[i] && missingPositions[p]) {
                Edge current;
                current.v = i + 1;
                current.w = p + 1;
                current.weight = matrix[i][p];
                PQInsert(missingPossibilities, current);
            }
        }
    }

    // Extracting from the priority queue to find the "cheapest" way to compleete
    // the solution
    for (int i = 0; i < missingTotal; i ++) {
        while (1) {
            Edge current = PQExtract(missingPossibilities);
            if (missingURLs[current.v - 1] && missingPositions[current.w - 1]) {
                solution[current.w - 1] = current.v;
                possible[current.v - 1][current.w - 1] = current.v;
                missingURLs[current.v - 1] = false;
                missingPositions[current.w - 1] = false;
                break;
            }
        }
    }
    PQFree(missingPossibilities);
    return;
}

// As there can be rounding errors within calculations we must use a precision 
// factor when comparing for equality
bool floatsEqual(float floatOne, float floatTwo) {
    float precision = 0.000001;
    if (((floatOne - precision) < floatTwo) && ((floatOne + precision) > floatTwo)) {
        return true;
    }
    return false;
}

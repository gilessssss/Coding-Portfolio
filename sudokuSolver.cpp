// A function which solves a classic sudoku puzzle represented as a 
// 9 * 9 array
// Hugo Giles 6/3/22

#include <iostream>
#define N 9
using namespace std;

int sudoku[N][N] = {
    {0,1,0,0,0,0,0,2,0},
    {0,0,0,0,0,9,0,0,0},
    {4,0,0,7,5,0,6,0,0},
    {0,0,2,9,3,0,0,6,0},
    {0,0,0,0,0,4,9,0,0},
    {3,0,0,0,0,8,0,0,0},
    {0,0,4,0,0,0,0,0,5},
    {5,0,0,3,6,0,7,0,0},
    {0,0,0,0,8,0,0,0,0}
};


bool inCol(int col, int value) {
    for (int i = 0; i < N; i ++) {
        if (sudoku[i][col] == value) {
            return true;
        }
    }
    return false;
}

bool inRow(int row, int value) {
    for (int i = 0; i < N; i ++) {
        if (sudoku[row][i] == value) {
            return true;
        }
    }
    return false;
}

bool inBox(int row, int col, int value) {
    int rowStart = (row / 3) * 3;
    int colStart = (col / 3) * 3;

    for (int i = rowStart; i < rowStart + 3; i ++) {
        for (int p = colStart; p < colStart + 3; p ++) {
            if (sudoku[i][p] == value) {
                return true;
            }
        }
    }
    return false;
}

bool findMissing(int &row, int &col) {
    for (row = 0; row < N; row ++) {
        for (col = 0; col < N; col ++) {
            if (sudoku[row][col] == 0) {
                return true;
            }
        }
    }
    return false;
}

bool isValid(int row, int col, int value) {
    if (inCol(col, value)) {
        return false;
    }

    if (inRow(row, value)) {
        return false;
    }

    if (inBox(row, col, value)) {
        return false;
    }
    return true;
}

bool solve() {
    int row;
    int col;

    if (!findMissing(row, col)) {
        return true;
    }

    for (int i = 1; i < 10; i ++) {
        if (isValid(row, col, i)) {
            sudoku[row][col] = i;
            
            if (solve()) {
                return true;
            }
            else {
                sudoku[row][col] = 0;
            }
        }
    }
    return false;
}

void printSudoku() {
    for (int i = 0; i < N; i ++) {
        for (int p = 0; p < N; p ++) {
            cout << sudoku[i][p] << " ";
            if ((p + 1) % 3 == 0) {
                cout << "  ";
            }
        }
        cout << "\n";
        if ((i + 1) % 3 == 0) {
            cout << "\n";
        }
    }
    return;
}


int main() {
    if (solve()) {
        printSudoku();
    }
    else {
        cout << "F in the chat\n";
    }
    return 0;
}

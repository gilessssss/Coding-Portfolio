// Implementation of the File System ADT
// COMP2521 Assignment 1

// Written by: Hugo Giles z5309502
// Date: 16/10/21

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FileType.h"
#include "Fs.h"
#include "utility.h"

#define DIRECTORY 0
#define REGULAR_FILE 1
#define ROOT 2

// For deleteProtocol
#define ONLY_EMPTY_DIR 0
#define ONLY_FILE 1
#define EVERYTHING 2

// Bool
#define FALSE 0
#define TRUE 1


struct FsRep {
    char name[PATH_MAX + 1];
    char cwdAddress[PATH_MAX + 1];
    char content[PATH_MAX + 1];
    int fileType;
    int error;
    Fs cwd;
    Fs firstChild;
    Fs nextSibling;
    Fs parent;
};

// Helper Functions
static void freeNodesRecursively(Fs fs);
static Fs moveToDir(Fs fs, char *path);
static void dirErrorHandling(int errorCode, char *path);
static void fileErrorHandling(int errorCode, char *path);
static char* canonicPath(char* path);
static void recursiveTreePrint(Fs fs, int level);
static Fs moveToFile(Fs fs, char *path);
static int delete(Fs fs, char *path, int deleteProtocol);
static Fs moveTo(Fs fs, char *path);
static void copyProcess(Fs fs, bool recursive, char *src[], char *dest);
static void cpMultiple(Fs fs, bool recursive, char *src[], char *dest);
static Fs cloneTree(Fs fs, Fs parent, int root);
static Fs cloneNode(Fs fs);
static void insertFileAtLocation(Fs fs, char *path, Fs node);
static void moveProcess(Fs fs, char *src[], char *dest);
static void mvMultiple(Fs fs, char *src[], char *dest);
static Fs unlinkNode(Fs fs, char *path);


// Creates a new root directory
Fs FsNew(void) {
    // Allocates memory to the new node and set it up as default
    Fs root = malloc(sizeof(*root));
    if (root == NULL) {
        fprintf(stderr, "Insufficient memory!\n");
        exit(EXIT_FAILURE);
    }

    strcpy(root->name, "/");
    strcpy(root->cwdAddress, "/");
    strcpy(root->content, "");
    root->fileType = ROOT;
    root->error = 0;
    root->cwd = root;
    root->firstChild = NULL;
    root->nextSibling = NULL;
    root->parent = root;

    return root;
}

// Puts the canonic path of the CWD into the cwd array
void FsGetCwd(Fs fs, char cwd[PATH_MAX + 1]) {
    strcpy(cwd, fs->cwdAddress);
    return;
}

// Frees all memory associated with the given Fs
void FsFree(Fs fs) {
    if (fs->firstChild != NULL) {
        freeNodesRecursively(fs->firstChild);
    }
    free(fs);
    return;
}

// Creates a directory at the given path
void FsMkdir(Fs fs, char *path) {
    // Separates the path to parent directory and the new directory name
    char adjustedPath[PATH_MAX + 1] = {0};
    strcpy(adjustedPath, path);

    int i = 0;
    while ((i < strlen(adjustedPath)) &&
        (adjustedPath[strlen(adjustedPath) - 1 - i] != '/')) {
        i ++;
    }
    int length = i;
    char filename[50] = {0};

    i = 0;
    while (i < length) {
        filename[length - 1 - i] = adjustedPath[strlen(adjustedPath) - 1];
        adjustedPath[strlen(adjustedPath) - 1] = 0;
        i ++;
    }
    if (strlen(adjustedPath) != 0) {
        adjustedPath[strlen(adjustedPath) - 1] = 0;
    }

    if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0) ||
        (strcmp(filename, "") == 0)) {
        dirErrorHandling(1, path);
        return;
    }

    // Allocate memory for new directory
    Fs newDirectory = malloc(sizeof(*newDirectory));
    if (newDirectory == NULL) {
        fprintf(stderr, "Insufficient memory!\n");
        exit(EXIT_FAILURE);
    }

    strcpy(newDirectory->name, filename);
    strcpy(newDirectory->content, "");
    newDirectory->fileType = DIRECTORY;
    newDirectory->error = 0;
    newDirectory->firstChild = NULL;
    newDirectory->nextSibling = NULL;
    newDirectory->parent = NULL;

    // Move to parent directory and deal with errors
    Fs destination = moveToDir(fs, adjustedPath);

    int status = destination->error;
    fs->error = 0;

    if (status == 1) {
        dirErrorHandling(1, path);
        free(newDirectory);
        return;
    }
    else if ((status == 2) || (status == 4)) {
        dirErrorHandling(2, path);
        free(newDirectory);
        return;
    }
    else if (status == 3) {
        dirErrorHandling(3, path);
        free(newDirectory);
        return;
    }   

    if (destination->firstChild == NULL) {
        destination->firstChild = newDirectory;
        newDirectory->parent = destination;
        return;
    }
    
    destination = destination->firstChild;

    // Loop through and place new directory in ASCII order
    while (destination != NULL) {
        if (strcmp(destination->name, filename) == 0) {
            dirErrorHandling(1, path);
            free(newDirectory);
            return;
        }
        else if (strcmp(destination->name, newDirectory->name) > 0) {
            newDirectory->nextSibling = destination;
            newDirectory->parent = destination->parent;

            newDirectory->parent->firstChild = newDirectory;
            return;
        }
        else if (destination->nextSibling == NULL) {
            destination->nextSibling = newDirectory;
            newDirectory->parent = destination->parent;
            return;
        }
        else if (strcmp(destination->nextSibling->name, newDirectory->name) > 0) {
            newDirectory->nextSibling = destination->nextSibling;
            destination->nextSibling = newDirectory;
            newDirectory->parent = destination->parent;

            return;
        }
        destination = destination->nextSibling;
    }
}

// Creates a file at the given path
void FsMkfile(Fs fs, char *path) {
    // Separates the path to parent directory and the new file name
    char adjustedPath[PATH_MAX + 1] = {0};
    strcpy(adjustedPath, path);

    int i = 0;
    while ((i < strlen(adjustedPath)) &&
        (adjustedPath[strlen(adjustedPath) - 1 - i] != '/')) {
        i ++;
    }
    int length = i;
    char filename[50] = {0};

    i = 0;
    while (i < length) {
        filename[length - 1 - i] = adjustedPath[strlen(adjustedPath) - 1];
        adjustedPath[strlen(adjustedPath) - 1] = 0;
        i ++;
    }
    if (strlen(adjustedPath) != 0) {
        adjustedPath[strlen(adjustedPath) - 1] = 0;
    }

    if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0) ||
        (strcmp(filename, "") == 0)) {
        fileErrorHandling(1, path);
        return;
    }
    
    // Allocate memory for new file
    Fs newFile = malloc(sizeof(*newFile));
    if (newFile == NULL) {
        fprintf(stderr, "Insufficient memory!\n");
        exit(EXIT_FAILURE);
    }

    strcpy(newFile->name, filename);
    strcpy(newFile->content, "");
    newFile->fileType = REGULAR_FILE;
    newFile->error = 0;
    newFile->firstChild = NULL;
    newFile->nextSibling = NULL;
    newFile->parent = NULL;

    // Move to parent directory and deal with errors
    Fs destination = moveToDir(fs, adjustedPath);

    int status = destination->error;
    fs->error = 0;

    if (status == 1) {
        fileErrorHandling(1, path);
        free(newFile);
        return;
    }
    else if ((status == 2) || (status == 4)) {
        fileErrorHandling(2, path);
        free(newFile);
        return;
    }
    else if (status == 3) {
        fileErrorHandling(3, path);
        free(newFile);
        return;
    }   

    if (destination->firstChild == NULL) {
        destination->firstChild = newFile;
        newFile->parent = destination;
        return;
    }

    destination = destination->firstChild;

    // Loop through and place new file in ASCII order
    while (destination != NULL) {
        if (strcmp(destination->name, filename) == 0) {
            fileErrorHandling(1, path);
            free(newFile);
            return;
        }
        else if (strcmp(destination->name, newFile->name) > 0) {
            newFile->nextSibling = destination;
            newFile->parent = destination->parent;

            newFile->parent->firstChild = newFile;
            return;
        }
        else if (destination->nextSibling == NULL) {
            destination->nextSibling = newFile;
            newFile->parent = destination->parent;
            return;
        }
        else if (strcmp(destination->nextSibling->name, newFile->name) > 0) {
            newFile->nextSibling = destination->nextSibling;
            destination->nextSibling = newFile;
            newFile->parent = destination->parent;

            return;
        }
        destination = destination->nextSibling;
    }
}

// Changes the working directory to path
void FsCd(Fs fs, char *path) {
    // If path is NULL cwd becomes the root
    if (path == NULL) {
        fs->cwd = fs;
        strcpy(fs->cwdAddress, "/");
        return;
    }
    // Move to path and handle any errors
    Fs newCd = moveToDir(fs, path);

    int status = newCd->error;
    fs->error = 0;

    if ((status == 2) || (status == 4)) {
        printf("cd: '%s': Not a directory\n", path);
        return;
    }
    else if (status == 3) {
        printf("cd: '%s': No such file or directory\n", path);
        return;
    }
    fs->cwd = newCd;

    // Combines path with old cwd in order to get an absolute path to the new cwd
    char passablePath[PATH_MAX + 1] = {0};

    int i = 0;
    int p = 0;
    if (path[0] != '/') {
        while (fs->cwdAddress[i] != '\0') {
            passablePath[i] = fs->cwdAddress[i];
            i ++;
        }
        passablePath[i ++] = '/';
    }
    while (path[p] != '\0') {
        passablePath[i ++] = path[p ++];
    }

    // Finds the canonic cwd and saves it
    char *canonicCwd = canonicPath(passablePath);
    strcpy(fs->cwdAddress, canonicCwd);
    free(canonicCwd);

    return;
}

// Prints all files and directories at a path
void FsLs(Fs fs, char *path) {
    // Finds the folder to ls from
    Fs requestedDir;
    if (path == NULL) {
        requestedDir = fs->cwd;
    }
    else {
        requestedDir = moveToDir(fs, path);
    }

    // Handles error if folder can't be found
    int status = requestedDir->error;
    fs->error = 0;

    if (status == 2) {
        printf("ls: cannot access '%s': Not a directory\n", path);
        return;
    }
    else if (status == 3) {
        printf("ls: cannot access '%s': No such file or directory\n", path);
        return;
    }

    if (status == 4) {
        printf("%s\n", path);
        return;
    }


    // Prints all files within the parent directory
    if (requestedDir->firstChild == NULL) {
        return;
    }

    Fs current = requestedDir->firstChild;
    while (1) {
        printf("%s\n", current->name);
        if (current->nextSibling == NULL) {
            return;
        }
        current = current->nextSibling;
    }
}

// Prints the canonical path of the cwd
void FsPwd(Fs fs) {
    printf("%s\n", fs->cwdAddress);
    return;
}

// Prints the file system
void FsTree(Fs fs, char *path) {
    // Initialises tree print recursion from path or root if path is NULL
    if (path == NULL) {
        printf("/\n");
        recursiveTreePrint(fs, 0);
    }
    else {
        Fs printFromHere = moveToDir(fs, path);
        int status = printFromHere->error;
        fs->error = 0;

        if ((status == 2) || (status == 4)) {
            printf("tree: '%s': Not a directory\n", path);
            return;
        }
        else if (status == 3) {
            printf("tree: '%s': No such file or directory\n", path);
            return;
        }
        printf("%s\n", path);
        recursiveTreePrint(printFromHere, 0);
    }
    return;
}

// Sets the content of a file at path
void FsPut(Fs fs, char *path, char *content) {
    // Moves to the file given by path and handles any errors
    Fs requestedFile = moveToFile(fs, path);

    int status = requestedFile->error;
    fs->error = 0;

    if (status == 1) {
        printf("put: '%s': Is a directory\n", path);
        return;
    }
    else if (status == 2) {
        printf("put: '%s': Not a directory\n", path);
        return;
    }
    else if (status == 3) {
        printf("put: '%s': No such file or directory\n", path);
        return;
    }

    // Change the content at path
    char *localContent = strdup(content);

    strcpy(requestedFile->content, localContent);
    free(localContent);
    return;
}

// Prints the content of a file at path
void FsCat(Fs fs, char *path) {
    // Moves to requested file at path and deals with any errors
    Fs requestedFile = moveToFile(fs, path);

    int status = requestedFile->error;
    fs->error = 0;

    if (status == 1) {
        printf("cat: '%s': Is a directory\n", path);
        return;
    }
    else if (status == 2) {
        printf("cat: '%s': Not a directory\n", path);
        return;
    }
    else if (status == 3) {
        printf("cat: '%s': No such file or directory\n", path);
        return;
    }

    // Prints content
    printf("%s", requestedFile->content);
    return;
}

// Deletes an empty directory at path
void FsDldir(Fs fs, char *path) {
    // Deletes directory or handles errors
    int status = delete(fs, path, ONLY_EMPTY_DIR);

    if (status == 0) {
        return;
    }
    else if (status == 1) {
        printf("dldir: failed to remove '%s': Directory not empty\n", path);
        return;
    }
    else if (status == 2) {
        printf("dldir: failed to remove '%s': Not a directory\n", path);
        return;
    }
    else if (status == 3) {
        printf("dldir: failed to remove '%s': No such file or directory\n", path);
        return;
    }
    return;
}

// Deletes the file at path
void FsDl(Fs fs, bool recursive, char *path) {
    int status;

    // Deletes path or handles any errors returned
    if (recursive == TRUE) {
        status = delete(fs, path, EVERYTHING);
    }
    else {
        status = delete(fs, path, ONLY_FILE);
    }
    if (status == 0) {
        return;
    }
    else if (status == 1) {
        printf("dl: cannot remove '%s': Is a directory\n", path);
        return;
    }
    else if (status == 2) {
        printf("dl: cannot remove '%s': Not a directory\n", path);
        return;
    }
    else if (status == 3) {
        printf("dl: cannot remove '%s': No such file or directory\n", path);
        return;
    }
    return;
}

// Copies file(s) or directory(ies) and pastes it at dest
void FsCp(Fs fs, bool recursive, char *src[], char *dest) {
    int srcSize = 0;

    // Starts copy process based on whether single or multiple input
    for (int i = 0; src[i] != NULL; i++) {
        srcSize ++;
    }

    if (srcSize == 1) {
        copyProcess(fs, recursive, src, dest);
    }
    else {
        cpMultiple(fs, recursive, src, dest);
    }
    return;
}

// Moves file(s) or directory(ies) to dest
void FsMv(Fs fs, char *src[], char *dest) {
    int srcSize = 0;

    // Starts move process based on whether single or multiple input
    for (int i = 0; src[i] != NULL; i++) {
        srcSize ++;
    }

    if (srcSize == 1) {
        moveProcess(fs, src, dest);
    }
    else {
        mvMultiple(fs, src, dest);
    }
    return;
}

// Helper Functions

// Frees a node and all its siblings and children recursively
static void freeNodesRecursively(Fs fs) {
    // Frees child, then sibling then itself recursively
    if (fs->firstChild != NULL) {
        freeNodesRecursively(fs->firstChild);
    } 
    if (fs->nextSibling != NULL) {
        freeNodesRecursively(fs->nextSibling);
    }
    free(fs);
    return;
}

// Returns a pointer to path directory
static Fs moveToDir(Fs fs, char *path) {
    // Deal with relative paths being passed in by going to cwd
    char temp_path[PATH_MAX + 1];

    strcpy(temp_path, path);

    Fs current = fs;

    if (path[0] != '/') {
        current = current->cwd;
    }

    char *separator = "/";
    char *token;

    token = strtok(temp_path, separator);

    // Navigate to directory and return any error if applicable
    while (token != NULL) {
        // Check for special cases
        if ((strcmp(token, ".") == 0) || (strcmp(token, "") == 0)) {
            token = strtok(NULL, separator);
            continue;
        }
        else if (strcmp(token, "..") == 0) {
            current = current->parent;
            token = strtok(NULL, separator);
            continue;
        }
        if (current->firstChild == NULL) {
            fs->error = 3;
            return fs;
        }
        else {
            current = current->firstChild;
        }

        // Loop through to find suitable directory
        while (current != NULL) {
            if (strcmp(current->name, token) == 0) {
                if (current->fileType != DIRECTORY) {
                    token = strtok(NULL, separator);
                    if (token != NULL) {
                        fs->error = 2;
                        return fs;
                    }
                    else {
                        fs->error = 4;
                        return fs;
                    }
                }
                break;
            }
            else {
                current = current->nextSibling;
            }
        }
        if (current == NULL) {
            fs->error = 3;
            return fs;
        }
        token = strtok(NULL, separator);
    }
    // Upon finishing return a pointer to the directory
    return current;
}

// Handles errors for making directories
static void dirErrorHandling(int errorCode, char *path) {
    // Based on errorcode print the applicable error message
    if (errorCode == 1) {
        printf("mkdir: cannot create directory '%s': File exists\n",
        path);
        return;
    }
    else if (errorCode == 2) {
        printf("mkdir: cannot create directory '%s': Not a directory\n",
        path);
        return;
    }
    else if (errorCode == 3) {
        printf("mkdir: cannot create directory '%s': No such file or directory\n",
        path);
        return;
    }
    return;
}

// Handles errors for making files
static void fileErrorHandling(int errorCode, char *path) {
    // Starts copy process based on whether single or multiple input
    if (errorCode == 1) {
        printf("mkfile: cannot create file '%s': File exists\n",
        path);
        return;
    }
    else if (errorCode == 2) {
        printf("mkfile: cannot create file '%s': Not a directory\n",
        path);
        return;
    }
    else if (errorCode == 3) {
        printf("mkfile: cannot create file '%s': No such file or directory\n",
        path);
        return;
    }
    return;
}

// When given a path, returns the canonic path
static char* canonicPath(char* path) {
    char *adjustedPath = malloc(sizeof(char) * (strlen(path) + 1));
    int i = 0;

    adjustedPath[i] = *path++;

    while (*path) {
        // Store one directory name in stack
        while ((*path) && (*path != '/')) {
            adjustedPath[i + 1] = *path++;
            i ++;
        }
        // Deal with special cases of "." and ".."
        if ((adjustedPath[i] == '.') && (adjustedPath[i - 1] == '/')) {
            // Don't add "./" to new path
            i --;
            if (*path == '/') {
                path ++;
            }
        }
        else if ((adjustedPath[i] == '.') && (adjustedPath[i - 1] == '.') &&
            (adjustedPath[i - 2] == '/')) {
            // If "../" track back to previous / and delete previous directory call
            if (i == 2) {
                i = 0;
            }
            else {
                i -= 3;
                while (adjustedPath[i] != '/') {
                    i --;
                }
            }
            if (*path == '/') {
                path ++;
            }
        }
        else {
            // if valid directory add to new path
            if (*path == '/') {
                if (adjustedPath[i] == '/') {
                    path ++;
                }
                else {
                    adjustedPath[i + 1] = *path++;
                    i ++;
                }
            }
        }
    }
    
    // Delete any unwanted characters
    if ((i > 0) && (adjustedPath[i] == '/')) {
        i --;
    }
    adjustedPath[i + 1] = 0;

    return adjustedPath;
}

// Prints a node and all its siblings and children recursively
static void recursiveTreePrint(Fs fs, int level) {
    // Print whitespace as required by spec
    for (int i = 0; i < level; i++) {
        printf("    ");
    }

    // Print file name before calling recursively for sibling and child
    if (level != 0) {
        printf("%s\n", fs->name);
    }
    if (fs->firstChild != NULL) {
        recursiveTreePrint(fs->firstChild, level + 1);
    }
    if (fs->nextSibling != NULL) {
        recursiveTreePrint(fs->nextSibling, level);
    }
    return;
}

// Returns a pointer to path file
static Fs moveToFile(Fs fs, char *path) {
    // Checks for special case
    if (strcmp(path, "/") == 0) {
        fs->error = 1;
        return fs;
    }

    // Separates path to parent directory and filename
    char temp_path[PATH_MAX + 1];

    strcpy(temp_path, path);

    Fs current = fs;

    if (path[0] != '/') {
        current = current->cwd;
    }

    int i = 0;
    while ((i < strlen(temp_path)) &&
        (temp_path[strlen(temp_path) - 1 - i] != '/')) {
        i ++;
    }
    int length = i;
    char filename[50] = {0};

    i = 0;
    while (i < length) {
        filename[length - 1 - i] = temp_path[strlen(temp_path) - 1];
        temp_path[strlen(temp_path) - 1] = 0;
        i ++;
    }

    if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0) ||
        (strcmp(filename, "/") == 0)) {
        fs->error = 1;
        return fs;
    }

    char *separator = "/";
    char *token;

    token = strtok(temp_path, separator);

    // Loop to find parent directory or return applicable error code
    while (token != NULL) {
        // Check special cases
        if ((strcmp(token, ".") == 0) || (strcmp(token, "") == 0)) {
            token = strtok(NULL, separator);
            continue;
        }
        else if (strcmp(token, "..") == 0) {
            current = current->parent;
            token = strtok(NULL, separator);
            continue;
        }
        if (current->firstChild == NULL) {
            fs->error = 3;
            return fs;
        }
        else {
            current = current->firstChild;
        }
        // Loop until directory is found
        while (current != NULL) {
            if (strcmp(current->name, token) == 0) {
                if (current->fileType != DIRECTORY) {
                    fs->error = 2;
                    return fs;
                }
                break;
            }
            else {
                current = current->nextSibling;
            }
        }
        if (current == NULL) {
            fs->error = 3;
            return fs;
        }
        token = strtok(NULL, separator);
    }

    if (current->firstChild == NULL) {
        fs->error = 3;
        return fs;
    }

    // Find file within parent directory
    current = current->firstChild;
    while (current != NULL) {
        if (strcmp(current->name, filename) == 0) {
            if (current->fileType != REGULAR_FILE) {
                fs->error = 1;
                return fs;
            }
            return current;
        }
        else {
            current = current->nextSibling;
        }
    }
    fs->error = 3;
    return fs;
}

// Deletes a file/dir at path
static int delete(Fs fs, char *path, int deleteProtocol) {
    // Check special case of root
    if (strcmp(path, "/") == 0) {
        return 1;
    }

    // Separate path into parent directory path and file/dir name
    char temp_path[PATH_MAX + 1];

    strcpy(temp_path, path);

    Fs current = fs;

    if (path[0] != '/') {
        current = current->cwd;
    }

    int i = 0;
    while ((i < strlen(temp_path)) &&
        (temp_path[strlen(temp_path) - 1 - i] != '/')) {
        i ++;
    }
    int length = i;
    char dirName[50] = {0};

    i = 0;
    while (i < length) {
        dirName[length - 1 - i] = temp_path[strlen(temp_path) - 1];
        temp_path[strlen(temp_path) - 1] = 0;
        i ++;
    }

    if ((strcmp(dirName, ".") == 0) || (strcmp(dirName, "..") == 0) ||
        (strcmp(dirName, "/") == 0)) {
        return 1;
    }

    char *separator = "/";
    char *token;

    token = strtok(temp_path, separator);

    // Loop through to find parent directory
    while (token != NULL) {
        // Deals with special cases
        if ((strcmp(token, ".") == 0) || (strcmp(token, "") == 0)) {
            token = strtok(NULL, separator);
            continue;
        }
        else if (strcmp(token, "..") == 0) {
            current = current->parent;
            token = strtok(NULL, separator);
            continue;
        }
        if (current->firstChild == NULL) {
            return 3;
        }
        else {
            current = current->firstChild;
        }
        // Finds directory
        while (current != NULL) {
            if (strcmp(current->name, token) == 0) {
                if (current->fileType != DIRECTORY) {
                    return 2;
                }
                break;
            }
            else {
                current = current->nextSibling;
            }
        }
        if (current == NULL) {
            return 3;
        }
        token = strtok(NULL, separator);
    }

    if (current->firstChild == NULL) {
        return 3;
    }

    // Based on delete protocol (what we can delete), find file/dir and delete
    // or return error if permission is not available/invalid path
    current = current->firstChild;
    while (current != NULL) {
        if (strcmp(current->name, dirName) == 0) {
            // Unlink all paths before freeing node recursively
            if (deleteProtocol == ONLY_EMPTY_DIR) {
                if (current->fileType != DIRECTORY) {
                    return 2;
                }
                else if (current->firstChild != NULL) {
                    return 1;
                }
                current->parent->firstChild = current->nextSibling;
                free(current);
            }
            else if (deleteProtocol == ONLY_FILE) {
                if (current->fileType != REGULAR_FILE) {
                    return 1;
                }
                current->parent->firstChild = current->nextSibling;
                free(current);
            }
            else {
                current->parent->firstChild = current->nextSibling;
                if (current->firstChild != NULL) {
                    freeNodesRecursively(current->firstChild);
                }
                free(current);
            }
            return 0;
        }
        else if (current->nextSibling == NULL) {
            return 3;
        }
        else if (strcmp(current->nextSibling->name, dirName) == 0) {
            // Unlink all paths before freeing node recursively
            if (deleteProtocol == ONLY_EMPTY_DIR) {
                if (current->nextSibling->fileType != DIRECTORY) {
                    return 2;
                }
                else if (current->nextSibling->firstChild != NULL) {
                    return 1;
                }
                Fs temp = current->nextSibling;
                current->nextSibling = temp->nextSibling;
                free(temp);
            }
            else if (deleteProtocol == ONLY_FILE) {
                if (current->nextSibling->fileType != REGULAR_FILE) {
                    return 1;
                }
                Fs temp = current->nextSibling;
                current->nextSibling = temp->nextSibling;
                free(temp);
            }
            else {
                Fs temp = current->nextSibling;
                current->nextSibling = temp->nextSibling;
                if (temp->firstChild != NULL) {
                    freeNodesRecursively(temp->firstChild);
                }
                free(temp);
            }
            return 0;
        }
        else {
            current = current->nextSibling;
        }
    }
    return 0;
}

// Returns a pointer to path, not as safe as moveToDir and moveToFile as it
// doesn't handle all errors
static Fs moveTo(Fs fs, char *path) {
    // Take in path and deal and move pointer to cwd if relative path
    char temp_path[PATH_MAX + 1];

    strcpy(temp_path, path);

    Fs current = fs;

    if (path[0] != '/') {
        current = current->cwd;
    }

    char *separator = "/";
    char *token;

    token = strtok(temp_path, separator);

    // Loop through recursively to find requested file/dir
    while (token != NULL) {
        if ((strcmp(token, ".") == 0) || (strcmp(token, "") == 0)) {
            token = strtok(NULL, separator);
            continue;
        }
        else if (strcmp(token, "..") == 0) {
            current = current->parent;
            token = strtok(NULL, separator);
            continue;
        }
        if (current->firstChild == NULL) {
            // file doesnt exist
            fs->error = 1;
            return fs;
        }
        current = current->firstChild;
        while (current != NULL) {
            if (strcmp(current->name, token) == 0) {
                break;
            }
            if (current->nextSibling == NULL) {
                // file doesnt exist
                fs->error = 1;
                return fs;
            }
            current = current->nextSibling;
        }
        token = strtok(NULL, separator);
    }
    return current;
}

// Copies a file or directory to dest
static void copyProcess(Fs fs, bool recursive, char *src[], char *dest) {
    Fs destination = moveTo(fs, dest);
    Fs source = moveTo(fs, src[0]);
    int status = destination->error;
    fs->error = 0;

    // Dest doesn't exist and we're copying in file
    if ((status != 0) && (source->fileType == REGULAR_FILE)) {
        FsMkfile(fs, dest);
        FsPut(fs, dest, source->content);
        return;
    }

    // Dest doesn't exist and we're copying in folder
    if ((status != 0) && (source->fileType != DIRECTORY) &&
        (recursive == TRUE)) {
        // Find path for parent direcotry and travel there
        char adjustedPath[PATH_MAX + 1] = {0};
        strcpy(adjustedPath, dest);

        int i = 0;
        while ((i < strlen(adjustedPath)) &&
            (adjustedPath[strlen(adjustedPath) - 1 - i] != '/')) {
            i ++;
        }
        int length = i;
        char filename[50] = {0};

        i = 0;
        while (i < length) {
            filename[length - 1 - i] = adjustedPath[strlen(adjustedPath) - 1];
            adjustedPath[strlen(adjustedPath) - 1] = 0;
            i ++;
        }
        if (strlen(adjustedPath) != 0) {
            adjustedPath[strlen(adjustedPath) - 1] = 0;
        }

        // Copy file requested and insert at parent directory of dest
        Fs payload = cloneTree(source, NULL, 1);
        strcpy(payload->name, filename);

        insertFileAtLocation(fs, adjustedPath, payload);
        return;
    }

    // If dest file already exists, just copy content
    if (destination->fileType == REGULAR_FILE) {
        FsPut(fs, dest, source->content);
        return;
    }

    // If dest folder exists copy file to the folder
    if ((destination->fileType != REGULAR_FILE) && (source->fileType == REGULAR_FILE)) {
        char adjustedPath[PATH_MAX + 1] = {0};
        strcpy(adjustedPath, dest);
        strcat(adjustedPath, "/");
        strcat(adjustedPath, source->name);

        // Check for duplicates and delete if necessary
        Fs checkForDuplicate = moveTo(fs, adjustedPath);
        if (checkForDuplicate->error != 1) {
            FsDl(fs, true, adjustedPath);
        }
        fs->error = 0;

        FsMkfile(fs, adjustedPath);
        FsPut(fs, adjustedPath, source->content);
        return;
    }

    // If dest folder exists copy folder into destination
    if ((destination->fileType != REGULAR_FILE) && (source->fileType == DIRECTORY) &&
        (recursive == TRUE)) {
        char adjustedPath[PATH_MAX + 1] = {0};
        strcpy(adjustedPath, dest);
        strcat(adjustedPath, "/");
        strcat(adjustedPath, source->name);

        // Check for duplicates and delete if necessary
        Fs checkForDuplicate = moveTo(fs, adjustedPath);
        if (checkForDuplicate->error != 1) {
            FsDl(fs, true, adjustedPath);
        }
        fs->error = 0;

        // Copy src and insert at destination
        Fs payload = cloneTree(source, destination, 1);
        insertFileAtLocation(fs, dest, payload);
        return;
    }
    return;
}

// Deals with multiple files/directories to copy
static void cpMultiple(Fs fs, bool recursive, char *src[], char *dest) {
    // Loop through src and pass into the copyProcess
    for (int i = 0; src[i] != NULL; i++) {
        char *path = strdup(src[i]);

        char *pass[] = {path, NULL};

        copyProcess(fs, recursive, pass, dest);
        free(path);
    }
    return;
}

// Clones a tree recursively
static Fs cloneTree(Fs fs, Fs parent, int root) {
    // Clone the current node
    if (fs == NULL) {
        return NULL;
    }
    Fs newNode = cloneNode(fs);

    // Link current node to siblings and children recursively
    newNode->parent = parent;
    if (fs->firstChild != NULL) {
        newNode->firstChild = cloneTree(fs->firstChild, newNode, 0);
    }
    if (!root) {
        if (fs->nextSibling != NULL) {
            newNode->nextSibling = cloneTree(fs->nextSibling, newNode->parent, 0);
        }
    }
    return newNode;
}

// Clones a node
static Fs cloneNode(Fs fs) {
    // Allocate memory for new node and copy name, content and file type
    Fs newNode = malloc(sizeof(*newNode));
    if (newNode == NULL) {
        fprintf(stderr, "Insufficient memory!\n");
        exit(EXIT_FAILURE);
    }

    strcpy(newNode->name, fs->name);
    strcpy(newNode->content, fs->content);
    newNode->fileType = fs->fileType;
    newNode->error = 0;
    newNode->cwd = fs->cwd;
    newNode->firstChild = NULL;
    newNode->nextSibling = NULL;
    newNode->parent = NULL;

    return newNode;
}

// Inserts a file/directory at a location
static void insertFileAtLocation(Fs fs, char *path, Fs node) {
    // Move to parent directory
    Fs parent = moveToDir(fs, path);

    // If parent has no children insert here
    if (parent->firstChild == NULL) {
        parent->firstChild = node;
        node->parent = parent;
        return;
    }

    // Otherwise loop through to insert file in ASCII order
    Fs current = parent->firstChild;
    while (current != NULL) {
        if (strcmp(current->name, node->name) > 0) {
            node->nextSibling = current;
            node->parent = current->parent;

            node->parent->firstChild = node;
            return;
        }
        else if (current->nextSibling == NULL) {
            current->nextSibling = node;
            node->parent = current->parent;
            return;
        }
        else if (strcmp(current->nextSibling->name, node->name) > 0) {
            node->nextSibling = current->nextSibling;
            current->nextSibling = node;
            node->parent = current->parent;

            return;
        }
        current = current->nextSibling;
    }
}

// Moves a file or directory to dest
static void moveProcess(Fs fs, char *src[], char *dest) {
    Fs destination = moveTo(fs, dest);
    Fs source = moveTo(fs, src[0]);
    int status = destination->error;
    fs->error = 0;

    // If dest doesn't exist
    if (status != 0) {
        // Find parent directory of dest
        char adjustedPath[PATH_MAX + 1] = {0};
        strcpy(adjustedPath, dest);

        int i = 0;
        while ((i < strlen(adjustedPath)) &&
            (adjustedPath[strlen(adjustedPath) - 1 - i] != '/')) {
            i ++;
        }
        int length = i;
        char filename[50] = {0};

        i = 0;
        while (i < length) {
            filename[length - 1 - i] = adjustedPath[strlen(adjustedPath) - 1];
            adjustedPath[strlen(adjustedPath) - 1] = 0;
            i ++;
        }
        if (strlen(adjustedPath) != 0) {
            adjustedPath[strlen(adjustedPath) - 1] = 0;
        }
        
        // Unlink src from original location
        Fs payload = unlinkNode(fs, src[0]);

        // insert src at parent directory of dest
        insertFileAtLocation(fs, adjustedPath, payload);
        strcpy(payload->name, filename);
        return;
    }

    // If file exists, delete the file and call moveProcess again
    if (destination->fileType == REGULAR_FILE) {
        FsDl(fs, true, dest);
        moveProcess(fs, src, dest);
        return;
    }

    // If dest is folder unlink node and insert into the folder
    if (destination->fileType != REGULAR_FILE) {
        Fs payload = unlinkNode(fs, src[0]);

        char adjustedPath[PATH_MAX + 1] = {0};
        strcpy(adjustedPath, dest);
        strcat(adjustedPath, "/");
        strcat(adjustedPath, source->name);

        Fs checkForDuplicate = moveTo(fs, adjustedPath);
        if (checkForDuplicate->error != 1) {
            FsDl(fs, true, adjustedPath);
        }
        fs->error = 0;

        insertFileAtLocation(fs, dest, payload);
        return;
    }
    return;
}

// Deals with multiple files/directories to move
static void mvMultiple(Fs fs, char *src[], char *dest) {
    // Loop through src and call moveProcess for each new src file/directory
    for (int i = 0; src[i] != NULL; i++) {
        char *path = strdup(src[i]);

        char *pass[] = {path, NULL};

        moveProcess(fs, pass, dest);
        free(path);
    }
    return;
}

// Unlinks a file/directory from the file system and returns a pointer to it
static Fs unlinkNode(Fs fs, char *path) {
    // Navigate to parent directory, by splitting path into parent path and 
    // file/dir name
    char adjustedPath[PATH_MAX + 1] = {0};
    strcpy(adjustedPath, path);

    int i = 0;
    while ((i < strlen(adjustedPath)) &&
        (adjustedPath[strlen(adjustedPath) - 1 - i] != '/')) {
        i ++;
    }
    int length = i;
    char filename[50] = {0};

    i = 0;
    while (i < length) {
        filename[length - 1 - i] = adjustedPath[strlen(adjustedPath) - 1];
        adjustedPath[strlen(adjustedPath) - 1] = 0;
        i ++;
    }

    if (strlen(adjustedPath) != 0) {
        adjustedPath[strlen(adjustedPath) - 1] = 0;
    }
    
    Fs parent = moveTo(fs, adjustedPath);

    Fs current = parent->firstChild;

    // Loop through to find requested file/directory
    // unlink and return it
    while (current != NULL) {
        if (strcmp(current->name, filename) == 0) {
            parent->firstChild = current->nextSibling;
            current->parent = NULL;
            current->nextSibling = NULL;
            return current;
        }
        if (strcmp(current->nextSibling->name, filename) == 0) {
            Fs temp = current->nextSibling;
            current->nextSibling = temp->nextSibling;
            temp->parent = NULL;
            temp->nextSibling = NULL;
            return temp;
        }
        current = current->nextSibling;
    }
    return fs;
}

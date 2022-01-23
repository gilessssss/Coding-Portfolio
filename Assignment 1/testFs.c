// Main program for testing the File System ADT

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Fs.h"

int main(void) {
    Fs fs = FsNew();

	FsMkfile(fs, "init");
	FsMkdir(fs, "bin");
	FsMkdir(fs, "etc");
	FsMkdir(fs, "home");
	FsMkdir(fs, "tmp");
	FsMkfile(fs, "bin/ls");
	FsMkfile(fs, "bin/mv");
	FsMkfile(fs, "etc/passwd");
	FsMkfile(fs, "tmp/tmp.123");
	FsMkfile(fs, "tmp/tmp.456");
	FsMkdir(fs, "etc/ssh");
	FsMkdir(fs, "home/jas");
	FsMkdir(fs, "tmp/tmpdir");
	FsTree(fs, NULL);
	FsFree(fs);
}


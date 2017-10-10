#ifndef STATUS_H
#define STATUS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

struct directory {
	char **contents;
	int file_count;
};

/* prints a directory's content to stdout
 * receives the directory path as argument */
void print_dir_contents(const char* dir);

/* prints a directory's content to specified file
 * receives the directory path as argument */
void fprint_dir_contents(const char* dir, FILE* file);

/* returns an array containing the names of
 * the directory's contents receives the 
 * directory path as argument */
struct directory *get_dir_contents(const char* dir);

#endif /* STATUS_H defined */

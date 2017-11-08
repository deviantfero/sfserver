#ifndef STATUS_H
#define STATUS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "utils.h"
#include "comms.h"


struct directory {
	struct file_status **files;
	int file_count;
};

struct server_status {
	const char *dir;
	struct directory *current_dir;
	int client_count;
	int downloads;
	int uploads;
	int pid;
};

struct file_status {
	char *name;
	int dcount;
};

/* receives a server_status struct and prints it's directories
 * and stats on how many times they've been downloaded into a
 * file specified by the [file] argument */
void fprint_dir_status(FILE* file, struct server_status *status);

/* receives a server_status struct and prints it's directories
 * and stats on how many times they've been downloaded into a
 * string specified by the [str] argument */
char *sprint_dir_status(struct server_status *status);

/* receives a server_status struct and prints it's contents
 * into FILE* specified by file param */
void fprint_status(FILE *file, struct server_status *status);

/* receives a server_status struct and prints it's current status
 * string specified by the [str] argument */
char *sprint_status(struct server_status *status);

/* returns an array containing the names of
 * the directory's contents receives the 
 * directory path as argument */
struct directory *get_dir_contents(const char* dir);

#endif /* STATUS_H defined */

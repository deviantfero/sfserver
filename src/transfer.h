#ifndef TRANSFER_H
#define TRANSFER_H

#define TRANSFER_CHAR '#'
#define DEFAULT_DIR "."

#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "utils.h"
#include "comms.h"


enum method {
	PIPES,
	QUEUE,
	SOCKETS
};

int upload_file(const char *pipe_name, char *src, char *file_name, int chunksize, enum method *m);
int send_pipe_file(const char *pipe_name, int src_fd, int chunksize, size_t file_size);
int receive_pipe_file(const char *pipe_name, int piped, int chunksize, size_t filesize);
void fprogress_bar(FILE *file, off_t file_size, size_t transfered);

#endif

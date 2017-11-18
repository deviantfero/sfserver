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
#include <sys/un.h>
#include <stddef.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include "utils.h"
#include "comms.h"
#include "encrypt.h"



char *get_method_name(enum method m);
int upload_file(const char *pipe_name, char *src, char *file_name, struct options *opt);
int send_pipe_file(const char *pipe_name, int src_fd, struct options *opt, size_t file_size);
int send_sock_file(const char *sock_name, int src_fd, struct options *opt, size_t filesize);
int receive_pipe_file(const char *pipe_name, int piped, struct options *opt, size_t filesize);
int receive_sock_file(const char *sock_name, int dst_fd, struct options *opt, size_t filesize);
int make_named_sock(const char *sock_name, bool recv);
void fprogress_bar(FILE *file, off_t file_size, size_t transfered);

#endif

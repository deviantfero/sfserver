#ifndef TRANSFER_H
#define TRANSFER_H

#define TRANSFER_CHAR '#'
#define DEFAULT_DIR "."
#define PRIORITY 5u

#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <stddef.h>
#include <mqueue.h>
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
ssize_t upload_file(const char *pipe_name, char *src, char *file_name, struct options *opt);
ssize_t send_pipe_file(const char *pipe_name, int src_fd, struct options *opt, size_t file_size);
ssize_t send_sock_file(const char *sock_name, int src_fd, struct options *opt, size_t filesize);
ssize_t send_queue_file(const char *queue, int src_fd, struct options *opt, size_t file_size);
ssize_t receive_pipe_file(const char *pipe_name, int piped, struct options *opt, size_t filesize);
ssize_t receive_sock_file(const char *sock_name, int dst_fd, struct options *opt, size_t filesize);
ssize_t receive_queue_file(const char *queue, int src_fd, struct options *opt, size_t file_size);
int make_named_sock(const char *sock_name, bool recv);
void fprogress_bar(FILE *file, off_t file_size, size_t transfered);

#endif

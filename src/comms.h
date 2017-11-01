#ifndef COMMS_H

#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define COMMS_H
#define SIGNAL 0
#define SENDER 1
#define MAX_BUFFER 1024
#define MSG_LS     "ls"
#define MSG_STATUS "status"
#define MSG_UPLD   "upload"
#define MSG_DOWNLD "download"
#define MSG_EXIT   "bye"
#define MSG_ARRIVE "hello"

/* messages should follow format "what;pid" */
void send_message(const char *pipe_name, char *msg, bool do_unlink);
char **wait_message(const char *pipe_name);

#endif /* ifndef COMMS_H */

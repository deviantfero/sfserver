#ifndef COMMS_H

#define COMMS_H

#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "utils.h"

#define SIGNAL 0
#define SENDER 1
#define WAIT_TIME 1000
#define NO_TIMEOUT 0
#define MSG_TIMEOUT "TIMEOUT"
#define DFT_TRIES 10 // default number of tries
#define MAX_BUFFER 4096
#define MAX_TOKENS 10
#define MSG_ENCRYPT   "encrypt"
#define MSG_CHUNKSIZE "chunksize"
#define MSG_METHOD    "method"
#define MSG_LS        "ls"
#define MSG_STATUS    "status"
#define MSG_DONE      "done"
#define MSG_UPLD      "upload"
#define MSG_DOWNLD    "download"
#define MSG_EXIT      "bye"
#define MSG_ARRIVE    "hello"

enum method {
	PIPES,
	QUEUE,
	SOCKETS
};

struct options {
	int chunksize;
	int pid;
	enum method m;
	bool encrypt;
	bool compress;
};

/* messages should follow format "what;pid" */
void send_message(const char *pipe_name, char *msg, bool do_unlink);
char **wait_message(const char *pipe_name, int tries);

#endif /* ifndef COMMS_H */

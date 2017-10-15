#include "comms.h"

/* opens pipe in Write only mode, 
 * this should freeze the program 
 * until message has been read */
void send_message(const char *pipe_name, char *msg, bool do_unlink) {
	mkfifo(pipe_name, 0666);

	int fifod = open(pipe_name, O_WRONLY);
	char *processed_message = malloc(MAX_BUFFER);

	snprintf(processed_message, MAX_BUFFER, "%s;%d\n", msg, getpid());
	write(fifod, processed_message, MAX_BUFFER);
	close(fifod);

	if(do_unlink) unlink(pipe_name);
}

/* variant of the wait_message found in sfc, it differs
 * in that it process the message and returns a char[][2]
 * strcuture that will be passed onto the handling thread
 * for that client */
char **wait_message(const char *pipe_name) {
	char *msg_buffer = malloc(MAX_BUFFER);
	char **msg = malloc(sizeof(char*) * 2);
	char *token;

	if(msg_buffer == NULL || msg == NULL) {
		return NULL;
	}

	int fifod = open(pipe_name, O_RDONLY);
	read(fifod, msg_buffer, MAX_BUFFER);

	for(int i = 0; (token = strsep(&msg_buffer, ";")) != NULL && i < 2; i++) {
		msg[i] = malloc(sizeof(strlen(token)));
		if(msg[i] == NULL) {
			return NULL;
		}
		strcpy(msg[i], token);
	}

	close(fifod);
	free(msg_buffer);
	return msg;
}


void *client_handler(void *client) {
	char*** real_client = (char***)client;
	puts("hello thread world");
	return client;
}

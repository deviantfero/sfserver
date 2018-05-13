#include "comms.h"

/* opens pipe in Write only mode, 
 * this should freeze the program 
 * until message has been read */
void send_message(const char *pipe_name, char *msg, bool do_unlink) {
	mkfifo(pipe_name, 0666);

	int fifod = open(pipe_name, O_WRONLY);
	int pm_size = buffer_size("%0*d%s", 7, getpid(), msg);
	char *processed_message = malloc(pm_size);

	snprintf(processed_message, pm_size, "%0*d%s", 7, getpid(), msg);
	write(fifod, processed_message, pm_size);
	close(fifod);

	free(processed_message);
	if(do_unlink) unlink(pipe_name);
}

char **wait_message(const char *pipe_name, int tries) {
	char *msg_buffer = malloc(1);
	char byte = 0;
	char **msg = malloc(sizeof(char*) * MAX_TOKENS);
	int count = 0;

	if(msg_buffer == NULL || msg == NULL) {
		return NULL;
	}

	mkfifo(pipe_name, 0666);
	int fifod = open(pipe_name, O_RDONLY);
	int err = 0; // read return value

	while((err = read(fifod, &byte, 1)) == 1) {
		msg_buffer = realloc(msg_buffer, count  + 1);
		msg_buffer[count] = byte;
		count++;
	}

	if(err == -1 && tries > 0) {
		usleep(WAIT_TIME);
		close(fifod);
		return wait_message(pipe_name, tries - 1);
	} else if(tries == 0){
		msg[SIGNAL] = NULL;
		msg[SENDER] = NULL;
		close(fifod);
		return msg;
	}


	msg[SENDER] = malloc(count);
	msg[SIGNAL] = malloc(count);
	snprintf(msg[SENDER], 8, "%s", msg_buffer);
	snprintf(msg[SIGNAL], count - 7, "%s", msg_buffer + 7);

	free(msg_buffer);
	close(fifod);
	return msg;
}


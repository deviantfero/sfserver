#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "comms.h"
#include "consts.h"
#include "logger.h"
#include "status.h"

#define WAIT_TIME  1

void *client_handler(void*);
struct server_status *status;
pthread_mutex_t cc_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
	const char *fifo_path = "/tmp/sfs";
	int opt;
	status = (struct server_status*) malloc(sizeof(struct server_status));
	status->dir = DEFAULT_DIR;

	while((opt = getopt(argc, argv, "D:")) != -1) {
		switch(opt) {
			case 'D': 
				if((status->dir = realpath(optarg, NULL)) == NULL) {
					fprintf(stderr, "%s: No such path or directory.\n", argv[0]);
					exit(2);
				}
				break;
			case '?':
				if(opt == 'D') {
					fprintf(stderr, "-%c requires an argument.\n", opt);
				}
				break;
			default:
				if((status->dir = realpath(DEFAULT_DIR, NULL)) == NULL) {
					fprintf(stderr, "%s: No such path or directory.\n", argv[0]);
					exit(2);
				}
				break;
		}
	}

	/* initializing server_status */
	status->pid = getpid();
	status->client_count = 0;
	status->current_dir = get_dir_contents(status->dir);
	fprint_status(stdout, status);

	char** msg = malloc(sizeof(char*) * 2);

	// initializing pointer, good practice (?)
	pthread_t* client_threads = malloc(sizeof(pthread_t)); 

	for(int i = 0;;) {
		sleep(WAIT_TIME);
		msg = wait_message(fifo_path);

		if(msg[SIGNAL] != NULL && msg[SENDER] != NULL) {

			/* alloc enough space for one more pthread */
			client_threads = realloc(client_threads, sizeof(pthread_t) * (i + 1));
			if(pthread_create(&client_threads[i], NULL, client_handler, msg)) {
				fprintf(stderr, "Error attending client\n");
			}
			
			if(pthread_detach(client_threads[i])) {
				fprintf(stderr, "Error joining client\n");
				exit(2);
			}
			i++;
		}
	}

	free(msg);
	free(status);
	return 0;
}

void *client_handler(void *param_msg) {
	char** msg = (char**)param_msg;
	char* cpipe_name = malloc(sizeof(MAX_BUFFER));

	if(cpipe_name == NULL) {
		fprintf(stderr, "Error: in allocating cpipe_name\n");
		pthread_exit(NULL);
	}

	pthread_mutex_lock(&cc_mutex);
	status->client_count++;
	fprint_status(stdout, status);
	pthread_mutex_unlock(&cc_mutex);

	/* we read where the client writes, thus we read from sfc(pid)w */
	snprintf(cpipe_name, MAX_BUFFER, "/tmp/sfc%sw", msg[SENDER]);

	while(1) {
		sleep(WAIT_TIME);
		msg = wait_message(cpipe_name);
		/* handle messages */
		if(strncmp(msg[SIGNAL], MSG_LS, sizeof(MSG_LS)) == 0) {
			fprintf(stdout, "handling ls signal (%s)\n", msg[SENDER]);
		} else if(strncmp(msg[SIGNAL], MSG_EXIT, sizeof(MSG_LS)) == 0) {
			fprintf(stdout, "closing thread for (%s)...\n", msg[SENDER]);

			pthread_mutex_lock(&cc_mutex);
			status->client_count--;
			fprint_status(stdout, status);
			pthread_mutex_unlock(&cc_mutex);

			break;
		}
	}

	free(cpipe_name);
	pthread_exit(msg);
}

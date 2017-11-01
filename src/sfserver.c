#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "consts.h"
#include "logger.h"
#include "status.h"
#include "comms.h"

#define WAIT_TIME  1

void *client_handler(void*);

int main(int argc, char *argv[]) {
	const char *dir = DEFAULT_DIR;
	const char *fifo_path = "/tmp/sfs";
	int opt;
	struct directory *current_dir;

	while((opt = getopt(argc, argv, "D:")) != -1) {
		switch(opt) {
			case 'D': 
				if((dir = realpath(optarg, NULL)) == NULL) {
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
				if((dir = realpath(DEFAULT_DIR, NULL)) == NULL) {
					fprintf(stderr, "%s: No such path or directory.\n", argv[0]);
					exit(2);
				}
				break;
		}
	}

	current_dir = get_dir_contents(dir);
	fprintf(stdout, "[pid: %d][dir: %s]\nwaiting clients...\n", getpid(), dir);

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

	return 0;
}

void *client_handler(void *param_msg) {
	char** msg = (char**)param_msg;
	char* cpipe_name = malloc(sizeof(MAX_BUFFER));

	if(cpipe_name == NULL) {
		fprintf(stderr, "Error: in allocating cpipe_name\n");
		return NULL;
	}

	/* we read where the client writes, thus we read from sfc(pid)w */
	snprintf(cpipe_name, MAX_BUFFER, "/tmp/sfc%sw", msg[SENDER]);
	fprintf(stdout, "started thread for (%s)...\n", msg[SENDER]);

	while(1) {
		sleep(WAIT_TIME);
		msg = wait_message(cpipe_name);
		/* handle messages */
		if(strncmp(msg[SIGNAL], MSG_LS, sizeof(MSG_LS)) == 0) {
			fprintf(stdout, "handling ls signal (%s)\n", msg[SENDER]);
		} else if(strncmp(msg[SIGNAL], MSG_EXIT, sizeof(MSG_LS)) == 0) {
			fprintf(stdout, "closing thread for (%s)...\n", msg[SENDER]);
			break;
		}
	}

	free(cpipe_name);
	pthread_exit(msg);
}

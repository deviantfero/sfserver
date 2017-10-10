#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "consts.h"
#include "logger.h"
#include "status.h"

#define MAX_BUFFER 4096
#define WAIT_TIME  1

int main(int argc, char *argv[]) {
	const char *dir = DEFAULT_DIR;
	const char *fifo_path = "/tmp/fifo";
	bool debug = true;
	char *msg_buffer = malloc(MAX_BUFFER);
	int opt, fifod;
	struct directory *current_dir;

	while((opt = getopt(argc, argv, "sD:")) != -1) {
		switch(opt) {
			case 'D': 
				if((dir = realpath(optarg, NULL)) == NULL) {
					fprintf(stderr, "%s: No such path or directory.\n", argv[0]);
					exit(2);
				}
				break;
			case 's': debug = false; break;
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

	fprintf(stdout, "[pid: %d][dir: %s]\nwaiting clients...\n", getpid(), dir);

	for(;;) {
		sleep(WAIT_TIME);
		fifod = open(fifo_path, O_RDONLY);
		if((read(fifod, msg_buffer, MAX_BUFFER)) != EOF)
			printf("%s", msg_buffer);
		close(fifod);
	}
	return 0;
}

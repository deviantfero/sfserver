#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./consts.h"
#include "./logger.h"
#include "./status.h"

int main(int argc, char *argv[]) {
	const char *dir = DEFAULT_DIR;
	bool debug = true;
	int opt;

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
				if(opt == 'D')
					fprintf(stderr, "-%c requires an argument.\n", opt);
				break;
			default:
				dir = realpath(DEFAULT_DIR, NULL);
				break;
		}
	}
	fprintf(stdout, "I'm waiting at pid: %d, serving: %s\n", getpid(), dir);
	output_dir_contents(dir);
	return 0;
}

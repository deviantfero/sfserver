#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>

#include "comms.h"
#include "status.h"
#include "transfer.h"

void *client_handler(void*);
struct server_status *status;
pthread_mutex_t cc_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
	const char *fifo_path = "/tmp/sfs";
	int opt;
	status = (struct server_status*) malloc(sizeof(struct server_status));
	status->current_dir = DEFAULT_DIR;
	status->downloads = 0;
	status->uploads = 0;

	while((opt = getopt(argc, argv, "D:")) != -1) {
		switch(opt) {
			case 'D': 
				if((status->current_dir = realpath(optarg, NULL)) == NULL) {
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
				if((status->current_dir = realpath(DEFAULT_DIR, NULL)) == NULL) {
					fprintf(stderr, "%s: No such path or directory.\n", argv[0]);
					exit(2);
				}
				break;
		}
	}

	/* initializing server_status */
	status->pid = getpid();
	status->client_count = 0;
	status->dir = get_dir_contents(status->current_dir);
	fprint_status(stdout, status);

	char** msg = malloc(sizeof(char*) * 2);

	pthread_t* client_threads = malloc(sizeof(pthread_t)); 

	for(int i = 0;;) {
		msg = wait_message(fifo_path, DFT_TRIES);

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
			send_message(fifo_path, (char*)status->dir, true);
			i++;
		}
	}

	free(msg);
	free(status);
	return 0;
}

void *client_handler(void *param_msg) {
	char** msg = (char**)param_msg;
	size_t name_len = 0;
	char* rpipe_name;
	char* wpipe_name;

	pthread_mutex_lock(&cc_mutex);
	status->client_count++;
	fprint_status(stdout, status);
	pthread_mutex_unlock(&cc_mutex);

	/* same length is true for both pipe's names */
	name_len = buffer_size("/tmp/sfc%dw", atoi(msg[SENDER]));

	rpipe_name = malloc(name_len);
	wpipe_name = malloc(name_len);

	if(rpipe_name == NULL || wpipe_name == NULL) {
		fprintf(stdout, "error allocating either rpipe or wpipe names\n");
		pthread_exit(msg);
	}

	/* we read where the client writes, thus we read from sfc(pid)w */
	/* we wirte where the client reads, thus we write in sfc(pid)r */
	snprintf(wpipe_name, name_len, "/tmp/sfc%dr", atoi(msg[SENDER]));
	snprintf(rpipe_name, name_len, "/tmp/sfc%dw", atoi(msg[SENDER]));

	while(1) {
		msg = wait_message(rpipe_name, DFT_TRIES);

		if(msg[SIGNAL] != NULL && msg[SENDER] != NULL) {
			if(strncmp(msg[SIGNAL], MSG_LS, sizeof(MSG_LS)) == 0) {
				fprintf(stdout, "handling ls signal (%s)\n", msg[SENDER]);
				char* files_msg = sprint_dir_status(status);
				send_message(wpipe_name, files_msg, true);
				free(files_msg);
			} else if(strncmp(msg[SIGNAL], MSG_STATUS, sizeof(MSG_STATUS)) == 0){
				fprintf(stdout, "handling status signal (%s)\n", msg[SENDER]);
				char *status_msg = sprint_status(status);
				send_message(wpipe_name, status_msg, true);
				free(status_msg);
			} else if(strncmp(msg[SIGNAL], MSG_UPLD, sizeof(MSG_EXIT)) == 0) {
				int total = 0, fnamesize, nfd; 
				char *dst_path;
				/* receive chunksize */
				msg = wait_message(rpipe_name, DFT_TRIES);
				int chunksize = atoi(msg[SIGNAL]);
				/* receive filesize */
				msg = wait_message(rpipe_name, DFT_TRIES);
				int filesize = atoi(msg[SIGNAL]);
				/* receive filename */
				msg = wait_message(rpipe_name, DFT_TRIES);
				fprintf(stdout, "receiving %s from (%s)...\n", msg[SIGNAL], msg[SENDER]);

				/* here goes select function for choosing method of receiving */
				fnamesize = buffer_size("%s/%s", status->current_dir, msg[SIGNAL]);
				dst_path = malloc(fnamesize);
				snprintf(dst_path, fnamesize, "%s/%s", status->current_dir, msg[SIGNAL]);
				puts(dst_path);
				nfd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

				while((total += receive_pipe_file(rpipe_name, nfd, chunksize, filesize)) < filesize);

				fprintf(stdout, "done! transfered %d bytes from (%s)\n", total, msg[SENDER]);
				pthread_mutex_lock(&cc_mutex);
				status->uploads++;
				status->dir = get_dir_contents(status->current_dir);
				fprint_status(stdout, status);
				pthread_mutex_unlock(&cc_mutex);

			
			} else if(strncmp(msg[SIGNAL], MSG_DOWNLD, sizeof(MSG_DOWNLD)) == 0) {
				int req_file;
				fprintf(stdout, "handling download signal (%s)\n", msg[SENDER]);
				char* files_msg = sprint_dir_status(status);
				send_message(wpipe_name, files_msg, true);

				int msg_size = buffer_size("%d", status->dir->file_count);
				char* str_file_count = malloc(msg_size);
				snprintf(str_file_count, msg_size, "%d", status->dir->file_count);
				send_message(wpipe_name, str_file_count, true);

				msg = wait_message(rpipe_name, DFT_TRIES);
				req_file = atoi(msg[SIGNAL]);

				fprintf(stdout, "%s - wants file: %s\n", msg[SENDER], status->dir->files[req_file]->name);
				upload_file(wpipe_name, status->dir->files[req_file]->name, 0, NULL);
				fprintf(stdout, "done! (%s)...\n", msg[SENDER]);

				pthread_mutex_lock(&cc_mutex);
				status->downloads++;
				status->dir->files[req_file]->dcount++;
				fprint_status(stdout, status);
				fprint_dir_status(stdout, status);
				pthread_mutex_unlock(&cc_mutex);

				free(files_msg);
				free(str_file_count);
			} else if(strncmp(msg[SIGNAL], MSG_EXIT, sizeof(MSG_EXIT)) == 0) {
				fprintf(stdout, "closing thread for (%s)...\n", msg[SENDER]);

				pthread_mutex_lock(&cc_mutex);
				status->client_count--;
				fprint_status(stdout, status);
				pthread_mutex_unlock(&cc_mutex);
				break;
			}
		}
	}

	free(wpipe_name);
	free(rpipe_name);
	pthread_exit(msg);
}

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
	char *rpipe_name;
	char *wpipe_name;
	char *csock_name;
	struct options *client = malloc(sizeof(struct options));

	client->m = PIPES;
	client->encrypt   = false;
	client->compress  = false;
	client->chunksize = 0;
	client->pid = atoi(msg[SENDER]);

	pthread_mutex_lock(&cc_mutex);
	status->client_count++;
	fprintf(stdout, "(%d) joined successfully!\n", client->pid);
	fprint_status(stdout, status);
	pthread_mutex_unlock(&cc_mutex);

	/* same length is true for both pipe's names */
	name_len = buffer_size("/tmp/sfc%dw", client->pid);

	rpipe_name = malloc(name_len);
	wpipe_name = malloc(name_len);
	csock_name = malloc(name_len);

	if(rpipe_name == NULL || wpipe_name == NULL) {
		fprintf(stdout, "error allocating either rpipe or wpipe names\n");
		pthread_exit(msg);
	}

	/* we read where the client writes, thus we read from sfc(pid)w */
	/* we wirte where the client reads, thus we write in sfc(pid)r */
	snprintf(wpipe_name, name_len, "/tmp/sfc%dr", client->pid);
	snprintf(rpipe_name, name_len, "/tmp/sfc%dw", client->pid);
	snprintf(csock_name, name_len, "/tmp/ssfc%d", client->pid);

	while(1) {
		msg = wait_message(rpipe_name, DFT_TRIES);

		if(msg[SIGNAL] != NULL && msg[SENDER] != NULL) {
			if(strncmp(msg[SIGNAL], MSG_LS, sizeof(MSG_LS)) == 0) {
				/* send server directory status to client */
				fprintf(stdout, "handling ls signal (%s)\n", msg[SENDER]);
				char* files_msg = sprint_dir_status(status);
				send_message(wpipe_name, files_msg, true);
				free(files_msg);
			} else if(strncmp(msg[SIGNAL], MSG_STATUS, sizeof(MSG_STATUS)) == 0){
				/* send server status to client */
				fprintf(stdout, "handling status signal (%s)\n", msg[SENDER]);
				char *status_msg = sprint_status(status);
				send_message(wpipe_name, status_msg, true);
				free(status_msg);
			} else if(strncmp(msg[SIGNAL], MSG_METHOD, sizeof(MSG_METHOD)) == 0) {
				/* update method for client */
				msg = wait_message(rpipe_name, DFT_TRIES);
				client->m   = atoi(msg[SIGNAL]);
				fprintf(stdout, "handling method signal (%s), new method (%s)\n", msg[SENDER], get_method_name(client->m));
			} else if(strncmp(msg[SIGNAL], MSG_CHUNKSIZE, sizeof(MSG_CHUNKSIZE)) == 0) {
				/* update chunk size for client */
				msg = wait_message(rpipe_name, DFT_TRIES);
				client->chunksize = atoi(msg[SIGNAL]);
				fprintf(stdout, "handling chunksize signal (%s), new chunksize (%d)\n", msg[SENDER], client->chunksize);
			} else if(strncmp(msg[SIGNAL], MSG_ENCRYPT, sizeof(MSG_ENCRYPT)) == 0) {
				/* turn on/off encryption */
				client->encrypt = !client->encrypt;
				fprintf(stdout, "handling encrypt signal (%s), encryption (%s)\n", msg[SENDER], client->encrypt ? "on" : "off");
			} else if(strncmp(msg[SIGNAL], MSG_UPLD, sizeof(MSG_EXIT)) == 0) {
				int total = 0, fnamesize, nfd; 
				char *dst_path;
				/* receive filesize */
				msg = wait_message(rpipe_name, DFT_TRIES);
				int filesize = atoi(msg[SIGNAL]);
				/* receive filename */
				msg = wait_message(rpipe_name, DFT_TRIES);
				fprintf(stdout, "receiving %s from (%s) with (%s)...\n", msg[SIGNAL], msg[SENDER], get_method_name(client->m));

				/* here goes select function for choosing method of receiving */
				fnamesize = buffer_size("%s/%s", status->current_dir, msg[SIGNAL]);
				dst_path = malloc(fnamesize);
				snprintf(dst_path, fnamesize, "%s/%s", status->current_dir, msg[SIGNAL]);
				puts(dst_path);
				nfd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

				switch(client->m) {
					case PIPES: 
						while((total += receive_pipe_file(rpipe_name, nfd, client, filesize)) < filesize);
						break;
					case SOCKETS:
						while((total += receive_sock_file(csock_name, nfd, client, filesize)) < filesize);
						break;
					default:
						break;
				}

				/* update status */
				fprintf(stdout, "done! transfered %d bytes from (%s | %s)\n", total, msg[SENDER], dst_path);
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

				/* receive number of file requested */
				msg = wait_message(rpipe_name, DFT_TRIES);
				req_file = atoi(msg[SIGNAL]);

				/* concatenate full path of file */
				int fnamesize = buffer_size("%s/%s", status->current_dir, status->dir->files[req_file]->name);
				char* src_path = malloc(fnamesize);
				snprintf(src_path, fnamesize, "%s/%s", status->current_dir, status->dir->files[req_file]->name);
				fprintf(stdout, "%s - wants file: %s\n", msg[SENDER], src_path);

				upload_file(wpipe_name, src_path, status->dir->files[req_file]->name, client);
				fprintf(stdout, "done! (%s)...\n", msg[SENDER]);

				/* update status */
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
			} else {
				fprintf(stdout, "unknown message [%s] by %s\n", msg[SIGNAL], msg[SENDER]);
			}
		}
	}

	free(wpipe_name);
	free(rpipe_name);
	pthread_exit(msg);
}

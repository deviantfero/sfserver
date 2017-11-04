#include "status.h"

void fprint_dir_status(FILE *file, struct server_status *s) {
	for(int i = 0; i < s->current_dir->file_count; i ++) {
		fprintf(file, "[%d]%30.30s %4s%d\n", i + 1, s->current_dir->files[i]->name, " ", s->current_dir->files[i]->dcount);
	}
}

char *sprint_dir_status(char *str, struct server_status *s) {
	size_t file_str_len = 0;
	char *file_str;
	str = malloc(1);

	for(int i = 0; i < s->current_dir->file_count; i ++) {
		/* get size of resulting string */
		file_str_len = buffer_size("[%d]%30.30s %4s%d\n",
								i + 1,
								s->current_dir->files[i]->name, " ", 
								s->current_dir->files[i]->dcount);

		file_str = malloc(file_str_len);

		snprintf(file_str, file_str_len, "[%d]%30.30s %4s%d\n", 
				i + 1, 
				s->current_dir->files[i]->name, " ", 
				s->current_dir->files[i]->dcount);

		str = realloc(str, strlen(str) + file_str_len);
		str = strncat(str, file_str, strlen(str) + file_str_len);
	}
	return str;
}

void fprint_status(FILE *file, struct server_status *s) {
	fprintf(file, "[pid: %d][dir: %s][clients: %d]"
			"[downloads: %d][uploads: %d]\n", s->pid, s->dir, s->client_count, s->downloads, s->uploads);
}

char *sprint_status(char *str, struct server_status *s) {
	int status_str_len = buffer_size("[pid: %d][dir: %s][clients: %d]"
				"[downloads: %d][uploads: %d]\n", s->pid, s->dir, s->client_count, s->downloads, s->uploads);
	str = malloc(status_str_len);
	snprintf(str, status_str_len, "[pid: %d][dir: %s][clients: %d]"
				"[downloads: %d][uploads: %d]\n", s->pid, s->dir, s->client_count, s->downloads, s->uploads);
	return str;
}


struct directory *get_dir_contents(const char *dir) {
	struct dirent *dir_contents;
	struct directory *current_dir = malloc(sizeof(struct directory));
	current_dir->files = malloc(sizeof(struct file_status*));

	if(current_dir == NULL) {
		return NULL;
	}

	DIR *fd = opendir(dir);

	for(int i = 0; (dir_contents = readdir(fd)) != NULL;){
		if(dir_contents->d_type == DT_REG) {
			current_dir->files = realloc(current_dir->files, sizeof(struct file_status*) * (i + 1));
			current_dir->files[i] = malloc(sizeof(struct file_status));
			current_dir->files[i]->name = dir_contents->d_name;
			current_dir->files[i]->dcount = 0;
			current_dir->file_count = i + 1;

			if(current_dir->files[i] == NULL) {
				printf("null at %d\n fetching files", i);
			}

			i++;
		}

	}

	return current_dir;
}

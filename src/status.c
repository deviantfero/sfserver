#include "status.h"

void print_dir_contents(const char *dir) {
	DIR *fd = opendir(dir);
	struct dirent *dir_contents;
	while((dir_contents = readdir(fd)) != NULL) {
		if(dir_contents->d_type == DT_DIR || dir_contents->d_type == DT_REG) {
			fprintf(stdout, "%s\n", dir_contents->d_name);
		}
	}
}

void fprint_dir_contents(FILE *file, const char *dir) {
	DIR *fd = opendir(dir);
	struct dirent *dir_contents;
	while((dir_contents = readdir(fd)) != NULL) {
		if(dir_contents->d_type == DT_DIR || dir_contents->d_type == DT_REG) {
			fprintf(file, "%s\n", dir_contents->d_name);
		}
	}
}

void fprint_status(FILE *file, struct server_status *s) {
	fprintf(file, "[pid: %d][dir: %s][clients: %d]\n", s->pid, s->dir, s->client_count);
}

struct directory *get_dir_contents(const char *dir) {
	struct dirent *dir_contents;
	struct directory *current_dir = malloc(sizeof(struct directory));
	current_dir->contents = malloc(sizeof(char*));

	if(current_dir == NULL) {
		return NULL;
	}

	DIR *fd = opendir(dir);

	for(int i = 0; (dir_contents = readdir(fd)) != NULL;){
		if(dir_contents->d_type == DT_DIR || dir_contents->d_type == DT_REG) {
			current_dir->contents = realloc(current_dir->contents, sizeof(char*) * (i + 1));
			current_dir->contents[i] = dir_contents->d_name;
			current_dir->file_count = i + 1;

			if(current_dir->contents[i] == NULL) {
				printf("null at %d\n", i);
			}

			i++;
		}

	}

	return current_dir;
}

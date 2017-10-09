#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

void output_dir_contents(const char* dir) {
	DIR *fd = opendir(dir);
	struct dirent *dir_contents;
	while((dir_contents = readdir(fd)) != NULL) {
		if(dir_contents->d_type == DT_DIR || dir_contents->d_type == DT_REG)
			fprintf(stdout, "%s\n", dir_contents->d_name);
	}
}

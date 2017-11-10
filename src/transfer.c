#include "transfer.h"

int receive_pipe_file(const char *pipe_name, int piped, int chunksize, size_t filesize) {
	int fifod = open(pipe_name, O_RDONLY), err;
	size_t count = 0;
	char byte[chunksize + 1];
	memset(byte, 0, chunksize + 1);

	while((err = read(fifod, byte, chunksize)) > 0 && count < filesize) {
		/* making sure not to insert trash at the end of file */
		chunksize = ((size_t)(filesize - count) > (size_t)chunksize) ? 
					(size_t)chunksize : (size_t)(filesize - count);

		if(err != -1 && write(piped, byte, chunksize) != -1) {
			count += err;
		}
		memset(byte, 0, chunksize + 1);
	}
	return count;
}

int send_pipe_file(const char *pipe_name, int src_fd, int chunksize, size_t filesize) {
	mkfifo(pipe_name, 0666);
	int fifod = open(pipe_name, O_WRONLY), transfered = 0, chunk = 0, wchunk = 0;
	char byte[chunksize + 1];

	for(int i = 0; (size_t)transfered < filesize; i++) {
		if((chunk = read(src_fd, byte, chunksize)) == -1) fprintf(stderr, "error reading a byte");
		wchunk = write(fifod, byte, chunksize);
		if(wchunk != -1)
			transfered += chunk;
		fprogress_bar(stdout, filesize, transfered);
	}

	close(src_fd);
	close(fifod);
	/* unlink(pipe_name); */

	return transfered;
}


void fprogress_bar(FILE *file, off_t file_size, size_t transfered) {
	float percentage = ((float)100/file_size) * transfered;
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

	int isize = buffer_size("\r%3.2f%% %d bytes", percentage, transfered);
	int barsize = size.ws_col - isize;

	char *progress_str = malloc(size.ws_col - isize);
	float progress_chars = ((float)(barsize)/100) * percentage;


	for(int i = 0; i < progress_chars - 1; i++) {
		progress_str[i] = TRANSFER_CHAR;
	}

	fprintf(file, "\r%3.2f%% %ld bytes [%-*s]", percentage, transfered, barsize - 1, progress_str);
	fflush(file);
}

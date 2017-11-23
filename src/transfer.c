#include "transfer.h"

char* get_method_name(enum method m) {
	switch(m) {
		case PIPES:   return "pipes";
		case QUEUE:   return "queue";
		case SOCKETS: return "sockets";
		default:      return "unknown";
	}
}

ssize_t upload_file(const char *pipe_name, 
				   char *src, 
				   char *file_name,
				   struct options *opt) {

	int status;
	int src_fd;
	off_t filesize;
	char fs[MAX_BUFFER];
	char sn[MAX_BUFFER];
	char qn[MAX_BUFFER];
	/* enum method method_value = PIPES; */

	if(opt->compress) {
		int extsize = buffer_size("%s", file_name) + 3;
		char *tmp_buffer = malloc(extsize);
		snprintf(tmp_buffer, extsize, "%s.gz", file_name);
		file_name = tmp_buffer;
		src = deflate_file(src);
	}

	src_fd = open(src, O_RDONLY);
	filesize = lseek(src_fd, 0, SEEK_END);
	lseek(src_fd, (off_t) 0, SEEK_SET);

	snprintf(fs, MAX_BUFFER, "%ld", filesize);
	snprintf(sn, MAX_BUFFER, "/tmp/ssfc%d", opt->pid);
	snprintf(qn, MAX_BUFFER, "/qsfc%d", opt->pid);

	send_message(pipe_name, fs, true);
	send_message(pipe_name, file_name, true);

	switch(opt->m) {
		case PIPES: 
			status = send_pipe_file(pipe_name, src_fd, opt, filesize) == filesize;
			break;
		case SOCKETS: 
			status = send_sock_file(sn, src_fd, opt, filesize) == filesize;
			break;
		case QUEUE: 
			status = send_queue_file(qn, src_fd, opt, filesize) == filesize;
			break;
		default: return 0;
	}
	
	/* delete the .gz if compress mode on */
	if(opt->compress) unlink(src);
	return status;
}

ssize_t receive_pipe_file(const char *pipe_name, int piped, struct options *opt, size_t filesize) {
	ssize_t chunksize = opt->chunksize == 0 ? MAX_BUFFER : opt->chunksize;
	int fifod = open(pipe_name, O_RDONLY); 
	ssize_t err, wchunk, count = 0;
	char byte[chunksize + 1];
	memset(byte, 0, chunksize + 1);

	while((err = read(fifod, byte, chunksize)) > 0 && (size_t)count < filesize) {
		/* making sure not to insert trash at the end of file */
		chunksize = ((size_t)(filesize - count) > (size_t)chunksize) ? 
					(size_t)chunksize : (size_t)(filesize - count);

		if(opt->encrypt) encrypt(byte, KEY, err);

		if(err != -1 && (wchunk = write(piped, byte, chunksize)) != -1) {
			count += wchunk;
		}
		fprogress_bar(stdout, filesize, count);
		memset(byte, 0, chunksize + 1);
	}
	return count;
}

ssize_t receive_queue_file(const char *queue, int dst_fd, struct options *opt, size_t filesize) {
	ssize_t chunksize = opt->chunksize == 0 ? MAX_BUFFER : opt->chunksize;
	char buffer[chunksize + 1];
	mqd_t msg_queue = mq_open(queue, O_RDONLY);
	ssize_t err = 0, wchunk = 0, count = 0;

	while((err = mq_receive(msg_queue, buffer, chunksize, NULL)) > 0) {
		/* making sure not to insert trash at the end of file */
		chunksize = ((size_t)(filesize - count) > (size_t)chunksize) ? 
					(size_t)chunksize : (size_t)(filesize - count);

		if(opt->encrypt) encrypt(buffer, KEY, err);

		if(err != -1 && (wchunk = write(dst_fd, buffer, chunksize)) != -1) {
			count += wchunk;
		} else {
			fprintf(stderr, "failed to receive from buffer: %s\n", strerror(errno));
			break;
		}
		fprogress_bar(stdout, filesize, count);
		memset(buffer, 0, chunksize + 1);

		/* weird bug */
		if((ssize_t)filesize - count == 0) {
			mq_unlink(queue);
			return count;
		}
	}
	mq_unlink(queue);
	return count;
}

ssize_t receive_sock_file(const char *sock_name, int dst_fd, struct options *opt, size_t filesize) {
	ssize_t chunksize = opt->chunksize == 0 ? MAX_BUFFER : opt->chunksize;
	int csock = make_named_sock(sock_name, true);
	char buffer[chunksize + 1];
	ssize_t err = 0, wchunk = 0, count = 0;

	while((err = read(csock, buffer, chunksize)) > 0 && (size_t)count < filesize) {
		/* making sure not to insert trash at the end of file */
		chunksize = ((size_t)(filesize - count) > (size_t)chunksize) ? 
					(size_t)chunksize : (size_t)(filesize - count);

		if(opt->encrypt) encrypt(buffer, KEY, err);

		if(err != -1 && (wchunk = write(dst_fd, buffer, chunksize)) != -1) {
			count += wchunk;
		}
		fprogress_bar(stdout, filesize, count);
		memset(buffer, 0, chunksize + 1);
	}

	close(csock);
	unlink(sock_name);

	return count;
}

ssize_t send_pipe_file(const char *pipe_name, int src_fd, struct options *opt, size_t filesize) {
	mkfifo(pipe_name, 0666);
	ssize_t chunksize = opt->chunksize == 0 ? MAX_BUFFER : opt->chunksize;
	int fifod = open(pipe_name, O_WRONLY); 
	ssize_t transfered = 0, chunk = 0, wchunk = 0;
	char byte[chunksize + 1];

	for(int i = 0; (size_t)transfered < filesize; i++) {
		if((ssize_t)(chunk = read(src_fd, byte, chunksize)) == -1) fprintf(stderr, "error reading a byte");

		if(opt->encrypt) encrypt(byte, KEY, chunk);

		wchunk = write(fifod, byte, chunksize);
		if(wchunk != -1)
			transfered += chunk;
	}

	close(src_fd);
	close(fifod);

	return transfered;
}

ssize_t send_queue_file(const char *queue, int src_fd, struct options *opt, size_t filesize) {
	int chunksize = opt->chunksize == 0 ? MAX_BUFFER : opt->chunksize;
	struct mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = chunksize;
	mqd_t msg_queue = mq_open(queue, O_WRONLY | O_CREAT, 0666, &attr);
	ssize_t transfered = 0, chunk = 0, wchunk = 0;
	char buffer[chunksize + 1];

	if(msg_queue < 0) {
		fprintf(stderr, "failed to open queue: %s\n", strerror(errno));
		return 0;
	}

	for(int i = 0; (size_t)transfered < filesize; i++) {
		if((chunk = read(src_fd, buffer, chunksize)) == -1) fprintf(stderr, "error reading a buffer");

		if(opt->encrypt) encrypt(buffer, KEY, chunk);

		wchunk = mq_send(msg_queue, buffer, chunksize, PRIORITY);
		if(wchunk == 0)
			transfered += chunk;
		else if(wchunk == -1)
			fprintf(stderr, "failed to write queue: %s\n", strerror(errno));
		fprogress_bar(stdout, filesize, transfered);
	}

	close(src_fd);
	return transfered;
}

ssize_t send_sock_file(const char *sock_name, int src_fd, struct options *opt, size_t filesize) {
	ssize_t chunksize = opt->chunksize == 0 ? MAX_BUFFER : opt->chunksize;
	int csock = make_named_sock(sock_name, false), ssock;
	char buffer[chunksize + 1];
	ssize_t transfered = 0, chunk = 0, wchunk = 0;

	if(listen(csock, 10) < 0) {
		fprintf(stderr, "failed to listen %s\n", strerror(errno));
		return 0;
	}

	ssock = accept(csock, (struct sockaddr*)NULL, NULL);
	for(int i = 0; (size_t)transfered < filesize; i++) {
		if((chunk = read(src_fd, buffer, chunksize)) == -1) fprintf(stderr, "error reading a byte");

		if(opt->encrypt) encrypt(buffer, KEY, chunk);

		wchunk = write(ssock, buffer, chunksize);
		if(wchunk != -1)
			transfered += chunk;
		fprogress_bar(stdout, filesize, transfered);
	}
	close(ssock);

	return transfered;
}

void fprogress_bar(FILE *file, off_t file_size, size_t transfered) {
	float percentage = ((float)100/file_size) * transfered;
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

	int isize = buffer_size("\r%3.2f%% %d bytes ", percentage, transfered);
	int barsize = size.ws_col - isize;

	char *progress_str = malloc(size.ws_col - isize);
	memset(progress_str, 0, size.ws_col - isize);
	float progress_chars = ((float)(barsize)/100) * percentage;


	for(int i = 0; i < progress_chars - 1; i++) {
		progress_str[i] = TRANSFER_CHAR;
	}

	fprintf(file, "\r%3.2f%% %ld bytes [%-*s]", percentage, transfered, barsize - 1, progress_str);
	fflush(file);
}

int make_named_sock(const char *sock_name, bool recv){
	struct sockaddr_un name;
	int sock;
	size_t size;

	if((sock = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "socket not created\n");
		return 0;
	}

	name.sun_family = AF_LOCAL;
	strncpy(name.sun_path, sock_name, sizeof(name.sun_path));
	name.sun_path[sizeof(name.sun_path) - 1] = '\0';

	size = SUN_LEN(&name);
	size = (offsetof (struct sockaddr_un, sun_path) + strlen(name.sun_path));

	int status = recv ? connect(sock, (struct sockaddr *)&name, size) : 
						bind(sock, (struct sockaddr *)&name, size);
	if(status < 0) {
		fprintf(stderr, "socket connect|bind fail: %s\n", strerror(errno));
		return 0;
	}

	return sock;
}

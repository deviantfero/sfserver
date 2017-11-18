#ifndef H_ENCRYPT

#define H_ENCRYPT
#define KEY "secretkey"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

char *encrypt(char *message, char *key);

#endif

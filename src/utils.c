#include "utils.h"

/* https://stackoverflow.com/questions/3919995/determining-sprintf-buffer-size-whats-the-standard 
 * original by: Regis Portales */

int buffer_size(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(NULL, 0, format, args);
    va_end(args);
    return result + 1; // safe byte for \0
}

void encrypt(char *message, char * key, ssize_t chunksize) {
    size_t keylen = buffer_size("%s", key);
    for(ssize_t i = 0; i < chunksize; i++) {
        message[i] = message[i] ^ key[i % keylen];
    }
}

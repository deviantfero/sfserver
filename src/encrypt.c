#include "encrypt.h"

/* original by: Tim
 * https://stackoverflow.com/questions/8072868/simple-xor-algorithm
 */

char *encrypt(char *message, char * key) {
    size_t messagelen = strlen(message);
    size_t keylen = strlen(key);

    char * encrypted = malloc(messagelen+1);

    for(size_t i = 0; i < messagelen; i++) {
        encrypted[i] = message[i] ^ key[i % keylen];
    }
    encrypted[messagelen] = '\0';

    return encrypted;
}

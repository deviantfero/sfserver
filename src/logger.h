#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/* by defining LOGGER_H we avoid the double definition of the prototype when we include this file
 * in sfserver.c, because it will already be defined the first time we include this header file in
 * logger.c, so the preprocessor skips this definition alltogether the second time arround */

void logger(const char* tag, const char* message);

#endif

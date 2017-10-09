#include <stdio.h>
#include "logger.h"

void logger(const char* tag, const char* message) {
	fprintf(stderr, "%s :: %s\n", tag, message);
}

#include <stdio.h>

#define MAX_BUFFER_SIZE 2

void writeBuffer(FILE *source, FILE *destination, int offset, size_t size);

void moveBytesBack(FILE *file, long destination, long sourceStart);
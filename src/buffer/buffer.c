#include "buffer.h"

void moveBytesBack(FILE *file, long destination, long sourceStart) {
    long fileSize, bytesToMove, bytesRead, bytesRemaining;
    int bytesToRead = MAX_BUFFER_SIZE;
    char buffer[MAX_BUFFER_SIZE];

    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);

    bytesRead = 0;
    bytesToMove = fileSize - sourceStart;

    while (bytesRead < bytesToMove) {
        bytesRemaining = bytesToMove - bytesRead;

        if (bytesRemaining < MAX_BUFFER_SIZE) {
            bytesToRead = bytesRemaining;
        }

        fseek(file, sourceStart + bytesRead, SEEK_SET);
        fread(buffer, bytesToRead, 1, file);
        fseek(file, destination + bytesRead, SEEK_SET);
        fwrite(buffer, bytesToRead, 1, file);
        bytesRead += bytesToRead;
    }
}

void writeBuffer(FILE *source, FILE *destination, int offset, size_t size) {
    char buffer[MAX_BUFFER_SIZE];
    int bytesToRead = MAX_BUFFER_SIZE;

    /**
     * Caso a posicao atual no arquivo + o tamanho maximo do buffer seja maior
     * que o tamanho do arquivo, entao o numero de bytes não é o maximo
     * possivel, e sim a diferença entre o tamanho do arquivo e a posicao atual.
     */
    if (offset + MAX_BUFFER_SIZE > size) {
        bytesToRead = size - offset;
    }

    fread(buffer, bytesToRead, 1, source);
    fwrite(buffer, bytesToRead, 1, destination);
}
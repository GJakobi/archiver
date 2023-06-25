#include "../files-list/files-list.h"

#define HEADER_SIZE 2 * sizeof(long)

typedef enum {
    NONE,
    INSERT,
    LIST,
    EXTRACT,
    REMOVE,
    APPEND,
    HELP,
    MOVE
} COMMAND;

/**
 * Pega as informações guardadas nos dois primeiros bytes: o local em que a
 * area de diretorio inicia e quantos arquivos estao guardados.
 */
void getHeaderData(int archiveFileExists, FILE *archiveFile,
                   long *directoryAreaStart, long *numberOfFilesStored);

/**
 * Recebe o nome do arquivo, o argc e argv, e o optind, que indica o indice do
 * proximo argumento a ser lido, nesse caso, os arquivos restantes a serem
 * inseridos.
 */
void insertFilesIntoArchive(char *archiveFileName, int argc, char *argv[],
                            int optind, int command);

/**
 * Lista os arquivos contido no archive em formato similar ao "tar -tvf"
 */
void listFilesFromArchive(char *archiveFileName);

/**
 * Remove os arquivos especificados do archive.
 */
void removeFilesFromArchive(char *archiveFilename, int argc, char *argv[],
                            int optind);

/**
 * Move o membro indicado na linha de comando para imediatamente depois do
 * membro target existente em archive.
 */
void moveFilesFromArchive(char *target, int argc, char *argv[], int optind);

void printHelp();

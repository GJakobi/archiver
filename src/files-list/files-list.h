#include <stdio.h>
#include <sys/types.h>

#define MAX_NAME_SIZE 1024

/**
 *  nome (sem espaços), UID (user ID), permissões, tamanho, data de modificação,
 * ordem no arquivo e localização.
 */
typedef struct FileInfo {
    char name[MAX_NAME_SIZE];
    unsigned long location;
    uid_t userId;
    mode_t permissions;
    off_t size;
    time_t lastModificationTime;
    int order;
    struct FileInfo *next;
} FileInfo;

typedef struct FilesList {
    FileInfo *head;
    long size;
} FilesList;

/**
 * Lê as informações de um arquivo e as guarda em uma struct FileInfo, que será
salva na área do diretorio.
*/
FileInfo *createFileInfo(char *fileName, int order, int location);

/*
Cria uma lista encadeada de FileInfo, que guarda informações sobre todos os
arquivos contidos no archive */
FilesList *createFilesList();

/**
 * Insere um FileInfo na lista de arquivos.
 */
void insertFileIntoFilesList(FilesList *filesList, FileInfo *fileInfo);

/**
 * Escreve a lista de arquivos no archive (considera que o ponteiro esta no
 * final do arquivo).
 */
void writeFilesListToDirectory(FilesList *filesList, FILE *archiveFile);

FilesList *createFilesListFromArchive(FILE *archiveFile,
                                      int numberOfFilesStored,
                                      long directoryAreaStart);

void printFilesList(FilesList *filesList);

FilesList *destroyFilesList(FilesList *filesList);

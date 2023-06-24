#include <stdio.h>
#include <sys/types.h>

#define MAX_NAME_SIZE 1024

/**
 *  Struct que define os conteúdos de um membro da area de diretório.
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

/**
 * Lista de arquivos que estão na area de diretório
 */
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

/**
 * Retorna um FileInfo da área de diretório a partir do seu nome.
 */
FileInfo *findFileInfo(FilesList *filesList, char *filename);

void printFilesList(FilesList *filesList);

FilesList *destroyFilesList(FilesList *filesList);

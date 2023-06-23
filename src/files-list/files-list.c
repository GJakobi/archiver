#include "files-list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Caso o nome do arquivo contenha um diretório, transforma em um caminho
 * relativo Ex: xy/dir/arq.txt -> ./xy/dir/arq.txt
 */
char *prepareFileName(char *fileName) {
    size_t newPathLength =
        strlen(fileName) + 3;  // "./" + null terminator no pior caso
    char *newPath = malloc(newPathLength * sizeof(char));
    if (!newPath) {
        fprintf(stderr, "Error: could not allocate memory for file name %s.\n",
                fileName);
        exit(1);
    }

    if (fileName[0] == '/') {
        strncpy(newPath, ".", newPathLength);
        strncat(newPath, fileName, newPathLength - 1);
    } else if (strncmp(fileName, "./", 2) == 0) {
        strncpy(newPath, fileName, newPathLength);
    } else {
        strncpy(newPath, "./", newPathLength);
        strncat(newPath, fileName, newPathLength - 2);
    }

    return newPath;
}

FileInfo *createFileInfo(char *fileName, int order, int location) {
    FileInfo *fileInfo;
    struct stat fileInfoStat;
    int readFileStat;

    fileInfo = malloc(sizeof(FileInfo));
    if (!fileInfo) {
        fprintf(stderr, "Error: could not allocate memory for file info.\n");
        exit(1);
    }

    readFileStat = stat(fileName, &fileInfoStat);
    if (readFileStat == -1) {
        fprintf(stderr, "Error: could not read file %s.\n", fileName);
        exit(1);
    }

    strcpy(fileInfo->name, prepareFileName(fileName));
    fileInfo->location = location;
    fileInfo->userId = fileInfoStat.st_uid;
    fileInfo->permissions = fileInfoStat.st_mode;
    fileInfo->size = fileInfoStat.st_size;
    fileInfo->lastModificationTime = fileInfoStat.st_mtime;
    fileInfo->order = order;

    return fileInfo;
}

FilesList *createFilesList() {
    FilesList *filesList;

    filesList = malloc(sizeof(FilesList));
    if (!filesList) {
        fprintf(stderr, "Error: could not allocate memory for files list.\n");
        exit(1);
    }

    filesList->head = NULL;
    filesList->size = 0;

    return filesList;
}

void insertFileIntoFilesList(FilesList *filesList, FileInfo *fileInfo) {
    FileInfo *currentFileInfo;

    if (!filesList->head) {
        filesList->head = fileInfo;
        filesList->size++;
        return;
    }

    currentFileInfo = filesList->head;
    while (currentFileInfo->next) {
        currentFileInfo = currentFileInfo->next;
    }

    currentFileInfo->next = fileInfo;
    filesList->size++;

    return;
}

void writeFilesListToDirectory(FilesList *filesList, FILE *archiveFile) {
    FileInfo *currentFileInfo;

    currentFileInfo = filesList->head;
    while (currentFileInfo) {
        fwrite(currentFileInfo, sizeof(FileInfo), 1, archiveFile);
        currentFileInfo = currentFileInfo->next;
    }
}

FilesList *createFilesListFromArchive(FILE *archiveFile, int numOfFilesStored,
                                      long dirStart) {
    FileInfo *fileInfo;
    long i;
    FilesList *filesList = createFilesList();

    fseek(archiveFile, dirStart, SEEK_SET);
    for (i = 0; i < numOfFilesStored; i++) {
        fileInfo = malloc(sizeof(FileInfo));
        if (!fileInfo) {
            fprintf(stderr,
                    "Error: could not allocate memory for file info.\n");
            exit(1);
        }
        fread(fileInfo, sizeof(FileInfo), 1, archiveFile);
        fileInfo->next = NULL;  // o valor do ponteiro nao eh o mesmo apos ler
        insertFileIntoFilesList(filesList, fileInfo);
    }

    return filesList;
}

FilesList *destroyFilesList(FilesList *filesList) {
    FileInfo *currentFileInfo, *nextFileInfo;

    currentFileInfo = filesList->head;
    while (currentFileInfo) {
        nextFileInfo = currentFileInfo->next;
        free(currentFileInfo);
        currentFileInfo = nextFileInfo;
    }

    free(filesList);

    return NULL;
}

void printFilesList(FilesList *filesList) {
    FileInfo *currentFileInfo;

    currentFileInfo = filesList->head;
    while (currentFileInfo) {
        printf("Name: %s\n", currentFileInfo->name);
        printf("Location: %d\n", currentFileInfo->location);
        printf("User ID: %d\n", currentFileInfo->userId);
        printf("Permissions: %d\n", currentFileInfo->permissions);
        printf("Size: %ld\n", currentFileInfo->size);
        printf("Last modification time: %ld\n",
               currentFileInfo->lastModificationTime);
        printf("Order: %d\n", currentFileInfo->order);
        printf("\n");

        currentFileInfo = currentFileInfo->next;
    }
}
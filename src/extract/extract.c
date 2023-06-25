#include "extract.h"

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../archive-manipulation/archive-manipulation.h"
#include "../buffer/buffer.h"
#include "../files-list/files-list.h"

void createFolder(char *folderName) {
    if (mkdir(folderName, 0777) == -1 && errno != EEXIST) {
        perror("Error");
        fprintf(stderr, "could not create folder %s\n", folderName);
        exit(1);
    }
}

void createFoldersFromFilename(char *filename) {
    char *filenameCopy = malloc(strlen(filename) + 1);
    strcpy(filenameCopy, filename);

    char *folderName = strtok(dirname(filenameCopy), "/");
    char currentPath[MAX_NAME_SIZE] = "";

    while (folderName) {
        strcat(currentPath, folderName);

        if (strlen(currentPath) != 1) {
            createFolder(currentPath);
        }

        strcat(currentPath, "/");

        folderName = strtok(NULL, "/");
    }

    free(filenameCopy);
}

void extractOneFile(FilesList *filesList, FILE *archiveFile, char *filename) {
    FileInfo *fileInfo;
    FILE *extractedFile;
    int i;

    fileInfo = findFileInfo(filesList, filename);
    if (!fileInfo) {
        fprintf(stderr, "Error: file %s not found in archive.\n", filename);
        exit(1);
    }

    createFoldersFromFilename(fileInfo->name);

    extractedFile = fopen(fileInfo->name, "w");
    if (!extractedFile) {
        fprintf(stderr, "Error: could not create file %s.\n", fileInfo->name);
        exit(1);
    }

    // TODO: give permissions to file

    fseek(archiveFile, fileInfo->location, SEEK_SET);
    for (i = 0; i < fileInfo->size; i += MAX_BUFFER_SIZE) {
        writeBuffer(archiveFile, extractedFile, i, fileInfo->size);
    }
}

void extractAllFiles(FilesList *filesList, FILE *archiveFile) {
    FileInfo *fileInfo;

    fileInfo = filesList->head;

    while (fileInfo) {
        extractOneFile(filesList, archiveFile, fileInfo->name);
        fileInfo = fileInfo->next;
    }
}

void extractFilesFromArchive(char *archiveFilename, int argc, char *argv[],
                             int optind) {
    FILE *archiveFile;
    FilesList *filesList;
    size_t i;
    long directoryAreaStart, numberOfFilesStored;

    archiveFile = fopen(archiveFilename, "r");
    if (!archiveFile) {
        fprintf(stderr, "Error: could not open archive file %s.\n",
                archiveFilename);
        exit(1);
    }

    getHeaderData(1, archiveFile, &directoryAreaStart, &numberOfFilesStored);

    filesList = createFilesListFromArchive(archiveFile, numberOfFilesStored,
                                           directoryAreaStart);

    /**
     * Caso não tenha sido passado nenhum arquivo específico, devemos extrair
     * todos os arquivos
     */
    if (optind == argc) {
        extractAllFiles(filesList, archiveFile);
        fclose(archiveFile);
        return;
    }

    for (i = optind; i < argc; i++) {
        extractOneFile(filesList, archiveFile, argv[i]);
    }
    fclose(archiveFile);
}

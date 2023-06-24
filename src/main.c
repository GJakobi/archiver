#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "./files-list/files-list.h"

#define MAX_BUFFER_SIZE 1024
#define HEADER_SIZE 2 * sizeof(long)

typedef enum { NONE, INSERT, LIST, EXTRACT } COMMAND;

/**
 * Pega as informações guardadas nos dois primeiros bytes: o local em que a
 * area de diretorio inicia e quantos arquivos estao guardados.
 */
void getHeaderData(int archiveFileExists, FILE *archiveFile,
                   long *directoryAreaStart, long *numberOfFilesStored) {
    if (!archiveFileExists) {
        // Quando o arquivo não existe, a area do diretorio começa após os dois
        // primeiros bytes iniciais (chamados de header).
        *directoryAreaStart = HEADER_SIZE;
        fwrite(directoryAreaStart, sizeof(long), 1, archiveFile);
        fwrite(numberOfFilesStored, sizeof(long), 1, archiveFile);
    } else {
        fread(directoryAreaStart, sizeof(long), 1, archiveFile);
        fread(numberOfFilesStored, sizeof(long), 1, archiveFile);
    }
}

void updateHeaderData(FILE *archiveFile, long *directoryAreaStart,
                      long *numberOfFilesStored) {
    fseek(archiveFile, 0, SEEK_SET);
    fwrite(directoryAreaStart, sizeof(long), 1, archiveFile);
    fwrite(numberOfFilesStored, sizeof(long), 1, archiveFile);
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

/**
 * Recebe o nome do arquivo, o argc e argv, e o optind, que indica o indice do
 * proximo argumento a ser lido, nesse caso, os arquivos restantes a serem
 * inseridos.
 */
void insertFilesIntoArchive(char *archiveFileName, int argc, char *argv[],
                            int optind) {
    FILE *archiveFile, *fileToInsert;
    int archiveFileExists, i, j;
    long directoryAreaStart, numberOfFilesStored = 0;
    FileInfo *fileInfo;
    FilesList *filesList;

    archiveFileExists = access(archiveFileName, F_OK) != -1;

    archiveFile = fopen(archiveFileName, archiveFileExists ? "r+" : "w+");
    if (!archiveFile) {
        fprintf(stderr, "Error: could not open archive file %s.\n",
                archiveFileName);
        exit(1);
    }

    getHeaderData(archiveFileExists, archiveFile, &directoryAreaStart,
                  &numberOfFilesStored);

    if (archiveFileExists) {
        filesList = createFilesListFromArchive(archiveFile, numberOfFilesStored,
                                               directoryAreaStart);
        fseek(archiveFile, directoryAreaStart, SEEK_SET);
    } else {
        filesList = createFilesList();
    }

    for (i = optind; i < argc; i++) {
        fileToInsert = fopen(argv[i], "r");
        if (!fileToInsert) {
            fprintf(stderr, "Error: could not open file %s.\n", argv[i]);
            exit(1);
        }

        fileInfo =
            createFileInfo(argv[i], numberOfFilesStored, directoryAreaStart);

        insertFileIntoFilesList(filesList, fileInfo);

        for (j = 0; j < fileInfo->size; j += MAX_BUFFER_SIZE) {
            writeBuffer(fileToInsert, archiveFile, j, fileInfo->size);
        }

        // atualiza o inicio da area do diretorio
        directoryAreaStart += fileInfo->size;
        numberOfFilesStored++;
        fclose(fileToInsert);
    }

    writeFilesListToDirectory(filesList, archiveFile);

    updateHeaderData(archiveFile, &directoryAreaStart, &numberOfFilesStored);

    fclose(archiveFile);
}

void listFilesFromArchive(char *archiveFileName) {
    FILE *archiveFile;
    FilesList *filesList;
    long directoryAreaStart, numberOfFilesStored;

    archiveFile = fopen(archiveFileName, "r");
    if (!archiveFile) {
        fprintf(stderr, "Error: could not open archive file %s.\n",
                archiveFileName);
        exit(1);
    }

    getHeaderData(1, archiveFile, &directoryAreaStart, &numberOfFilesStored);

    filesList = createFilesListFromArchive(archiveFile, numberOfFilesStored,
                                           directoryAreaStart);

    printFilesList(filesList);

    fclose(archiveFile);
}

void createFolder(char *folderName) {
    if (mkdir(folderName, 0777) == -1 && errno != EEXIST) {
        perror("Error");
        fprintf(stderr, "could not create folder %s\n", folderName);
        exit(1);
    }
}

void createFoldersFromFilename(char *filename) {
    char *folderName = strtok(dirname(filename), "/");
    char currentPath[MAX_NAME_SIZE] = "";

    while (folderName) {
        strcat(currentPath, folderName);

        if (strlen(currentPath) != 1) {
            createFolder(currentPath);
        }

        strcat(currentPath, "/");

        folderName = strtok(NULL, "/");
    }
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

    fseek(archiveFile, fileInfo->location, SEEK_SET);
    for (i = 0; i < fileInfo->size; i += MAX_BUFFER_SIZE) {
        writeBuffer(archiveFile, extractedFile, i, fileInfo->size);
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

    for (i = optind; i < argc; i++) {
        extractOneFile(filesList, archiveFile, argv[i]);
    }

    fclose(archiveFile);
}

void testeREAD() {
    FILE *bin = fopen("backup.vpp", "rb");
    FileInfo *fileInfo = malloc(sizeof(FileInfo));
    char content[1025];

    int i;

    long directoryAreaStart, numOfFilesStored;
    fread(&directoryAreaStart, sizeof(long), 1, bin);
    printf("directoryAreaStart: %ld\n", directoryAreaStart);
    fread(&numOfFilesStored, sizeof(long), 1, bin);
    printf("numOfFilesStored: %ld\n", numOfFilesStored);

    for (i = 0; i < numOfFilesStored; i++) {
        fread(content, 1, 3, bin);
        printf("content %d: %s\n", i + 1, content);
    }

    for (i = 0; i < numOfFilesStored; i++) {
        printf("--- READING FILE %d ---\n", i + 1);

        fread(fileInfo, sizeof(FileInfo), 1, bin);
        printf("name: %s\n", fileInfo->name);
        printf("userId: %u\n", fileInfo->userId);
        printf("permissions: %hu\n", fileInfo->permissions);
        printf("size: %lld\n", fileInfo->size);
        printf("lastModificationTime: %ld\n", fileInfo->lastModificationTime);
        printf("order: %d\n", fileInfo->order);
        printf("location: %lu\n", fileInfo->location);
    }
}

int main(int argc, char *argv[]) {
    int option;
    char *archiveFileName = NULL;
    COMMAND command = NONE;

    // testeREAD();
    // return 1;

    while ((option = getopt(argc, argv, "i:c:x:")) != -1) {
        switch (option) {
            case 'i':
                archiveFileName = optarg;
                command = INSERT;
                break;
            case 'c':
                archiveFileName = optarg;
                command = LIST;
                break;
            case 'x':
                archiveFileName = optarg;
                command = EXTRACT;
                break;
            default:
                fprintf(stderr, "Error: invalid option used.\n");
                exit(1);
        }
    }

    switch (command) {
        case INSERT:
            insertFilesIntoArchive(archiveFileName, argc, argv, optind);
            break;
        case LIST:
            listFilesFromArchive(archiveFileName);
            break;
        case EXTRACT:
            extractFilesFromArchive(archiveFileName, argc, argv, optind);
            break;
        default:
            break;
    }

    return 0;
}
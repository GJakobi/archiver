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

#define MAX_BUFFER_SIZE 2
#define HEADER_SIZE 2 * sizeof(long)

typedef enum { NONE, INSERT, LIST, EXTRACT, REMOVE, APPEND, HELP } COMMAND;

void updateHeaderData(FILE *archiveFile, long *directoryAreaStart,
                      long *numberOfFilesStored) {
    fseek(archiveFile, 0, SEEK_SET);
    fwrite(directoryAreaStart, sizeof(long), 1, archiveFile);
    fwrite(numberOfFilesStored, sizeof(long), 1, archiveFile);
}

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

void removeOneFile(FILE *archiveFile, char *filename, char *archiveFilename,
                   long *dirAreaStart, long *numFilesStored,
                   FilesList *filesList) {
    /* Bytes que sobram depois de deletar o arquivo (sem
    contar com os bytes da area de diretorio) */
    int byterAfterDelete;
    FileInfo *fileInfo;

    fileInfo = findFileInfo(filesList, filename);
    if (!fileInfo) {
        fprintf(stderr, "Error: file %s not found in archive.\n", filename);
        exit(1);
    }

    byterAfterDelete = *dirAreaStart - fileInfo->size;

    moveBytesBack(archiveFile, fileInfo->location,
                  fileInfo->location + fileInfo->size);

    *dirAreaStart -= fileInfo->size;
    *numFilesStored -= 1;

    updateFileInfoAfterDelete(fileInfo);

    removeFileFromFilesList(filesList, fileInfo->name);

    truncate(archiveFilename, byterAfterDelete);
}

/**
 * Atualiza a área de diretorio e o header do arquivo.
 */
void updateArchiveAfterRemoval(FILE *archiveFile, FilesList *filesList,
                               long directoryAreaStart,
                               long numberOfFilesStored) {
    fseek(archiveFile, directoryAreaStart, SEEK_SET);
    writeFilesListToDirectory(filesList, archiveFile);
    updateHeaderData(archiveFile, &directoryAreaStart, &numberOfFilesStored);
}

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
 * Determina se um arquivo deve ser substituido ou não quando ele ja existe no
 * archive.
 */
int shouldUpdateFile(FileInfo *fileInfo, int command) {
    struct stat fileInfoStat;
    int readFileStat;

    readFileStat = stat(fileInfo->name, &fileInfoStat);
    if (readFileStat == -1) {
        fprintf(stderr, "Error: could not stat file %s.\n", fileInfo->name);
        exit(1);
    }

    if (command != APPEND ||
        fileInfoStat.st_mtime > fileInfo->lastModificationTime) {
        return 1;
    }

    return 0;
}

/**
 * Recebe o nome do arquivo, o argc e argv, e o optind, que indica o indice do
 * proximo argumento a ser lido, nesse caso, os arquivos restantes a serem
 * inseridos.
 */
void insertFilesIntoArchive(char *archiveFileName, int argc, char *argv[],
                            int optind, int command) {
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
        fileInfo = findFileInfo(filesList, argv[i]);
        if (fileInfo) {
            if (shouldUpdateFile(fileInfo, command)) {
                removeOneFile(archiveFile, fileInfo->name, archiveFileName,
                              &directoryAreaStart, &numberOfFilesStored,
                              filesList);
                updateArchiveAfterRemoval(archiveFile, filesList,
                                          directoryAreaStart,
                                          numberOfFilesStored);
                fseek(archiveFile, directoryAreaStart, SEEK_SET);
            } else {
                continue;
            }
        }

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

void removeFilesFromArchive(char *archiveFilename, int argc, char *argv[],
                            int optind) {
    FILE *archiveFile;
    FilesList *filesList;
    long directoryAreaStart, numberOfFilesStored;
    size_t i;

    archiveFile = fopen(archiveFilename, "r+");
    if (!archiveFile) {
        fprintf(stderr, "Error: could not open archive file %s.\n",
                archiveFilename);
        exit(1);
    }

    getHeaderData(1, archiveFile, &directoryAreaStart, &numberOfFilesStored);

    filesList = createFilesListFromArchive(archiveFile, numberOfFilesStored,
                                           directoryAreaStart);

    for (i = optind; i < argc; i++) {
        removeOneFile(archiveFile, argv[i], archiveFilename,
                      &directoryAreaStart, &numberOfFilesStored, filesList);
    }

    updateArchiveAfterRemoval(archiveFile, filesList, directoryAreaStart,
                              numberOfFilesStored);

    fclose(archiveFile);
}

void printHelp() {
    printf("Execucao: vina++ <opção> <archive> [membro1 membro2 ...]\n");

    printf("Opcoes:\n");
    printf(
        "-i: insere/acrescenta um ou mais membros ao archive. Caso o membro "
        "já exista no archive, ele deve ser substituído. Novos membros são "
        "inseridos respeitando a ordem da linha de comando, ao final do "
        "archive.\n");
    printf(
        "-a : mesmo comportamento da opção -i, mas a substituição de um membro "
        "existente ocorre APENAS caso o parâmetro seja mais recente que o "
        "arquivado.\n");
    printf(
        "-m target: move o membro indicado na linha de comando para "
        "imediatamente depois do membro target existente em archive.\n");
    printf(
        "-x: extrai os membros indicados de archive. Se os membros não forem "
        "indicados, todos devem ser extraídos.\n");
    printf("-r: remove os membros indicados de archive;\n");
    printf(
        "-c: lista o conteúdo de archive em ordem, incluindo as propriedades "
        "de cada membro (nome, UID, permissões, tamanho e data de modificação) "
        "e sua ordem no arquivo.\n");
}

int main(int argc, char *argv[]) {
    int option;
    char *archiveFileName = NULL;
    COMMAND command = NONE;

    while ((option = getopt(argc, argv, "i:c:x:r:a:h")) != -1) {
        switch (option) {
            case 'a':
                archiveFileName = optarg;
                command = APPEND;
                break;
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
            case 'r':
                archiveFileName = optarg;
                command = REMOVE;
                break;
            case 'h':
                command = HELP;
                break;
            default:
                fprintf(stderr, "Error: invalid option used.\n");
                exit(1);
        }
    }

    switch (command) {
        case APPEND:
        case INSERT:
            insertFilesIntoArchive(archiveFileName, argc, argv, optind,
                                   command);
            break;
        case LIST:
            listFilesFromArchive(archiveFileName);
            break;
        case EXTRACT:
            extractFilesFromArchive(archiveFileName, argc, argv, optind);
            break;
        case REMOVE:
            removeFilesFromArchive(archiveFileName, argc, argv, optind);
            break;
        case HELP:
            printHelp();
            break;
        default:
            break;
    }

    return 0;
}
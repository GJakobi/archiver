#include "archive-manipulation.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../buffer/buffer.h"
#include "../files-list/files-list.h"

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

/**
 * Remove apenas um arquivo de um archive.
 */
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
 * Atualiza a área de diretorio e o header do arquivo. Necessita ser chamada
 * após remover uma quantidade qualquer de arquivos.
 */
void updateArchiveAfterRemoval(FILE *archiveFile, FilesList *filesList,
                               long directoryAreaStart,
                               long numberOfFilesStored) {
    fseek(archiveFile, directoryAreaStart, SEEK_SET);
    writeFilesListToDirectory(filesList, archiveFile);
    updateHeaderData(archiveFile, &directoryAreaStart, &numberOfFilesStored);
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

void moveFilesFromArchive(char *target, int argc, char *argv[], int optind) {
    FILE *archiveFile;
    FileInfo *targetFileInfo, *fileInfoToBeMoved;
    FilesList *filesList;
    long directoryAreaStart, numberOfFilesStored;

    if (argc - optind != 2) {
        fprintf(stderr, "Error: wrong number of arguments.\n");
        exit(1);
    }

    archiveFile = fopen(argv[optind], "r+");
    if (!archiveFile) {
        fprintf(stderr, "Error: could not open archive file %s.\n",
                argv[optind]);
        exit(1);
    }

    getHeaderData(1, archiveFile, &directoryAreaStart, &numberOfFilesStored);

    filesList = createFilesListFromArchive(archiveFile, numberOfFilesStored,
                                           directoryAreaStart);

    targetFileInfo = findFileInfo(filesList, target);
    if (!targetFileInfo) {
        fprintf(stderr, "Error: target file %s not found in archive.\n",
                target);
        exit(1);
    }

    fileInfoToBeMoved = findFileInfo(filesList, argv[optind + 1]);
    if (!fileInfoToBeMoved) {
        fprintf(stderr, "Error: file %s not found in archive.\n",
                argv[optind + 1]);
        exit(1);
    }

    moveFileInfo(filesList, targetFileInfo, fileInfoToBeMoved);

    fseek(archiveFile, directoryAreaStart, SEEK_SET);
    writeFilesListToDirectory(filesList, archiveFile);

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

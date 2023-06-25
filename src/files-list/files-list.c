#include "files-list.h"

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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
    char *preparedFileName;

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

    preparedFileName = prepareFileName(fileName);

    strcpy(fileInfo->name, preparedFileName);
    fileInfo->location = location;
    fileInfo->userId = fileInfoStat.st_uid;
    fileInfo->permissions = fileInfoStat.st_mode;
    fileInfo->size = fileInfoStat.st_size;
    fileInfo->lastModificationTime = fileInfoStat.st_mtime;
    fileInfo->order = order;
    fileInfo->next = NULL;

    free(preparedFileName);

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
    FileInfo *currentFileInfo = NULL;

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

void printFilePermissions(mode_t permissions) {
    char formattedPermissions[11];

    formattedPermissions[0] = (S_ISDIR(permissions)) ? 'd' : '-';
    formattedPermissions[1] = (permissions & S_IRUSR) ? 'r' : '-';
    formattedPermissions[2] = (permissions & S_IWUSR) ? 'w' : '-';
    formattedPermissions[3] = (permissions & S_IXUSR) ? 'x' : '-';
    formattedPermissions[4] = (permissions & S_IRGRP) ? 'r' : '-';
    formattedPermissions[5] = (permissions & S_IWGRP) ? 'w' : '-';
    formattedPermissions[6] = (permissions & S_IXGRP) ? 'x' : '-';
    formattedPermissions[7] = (permissions & S_IROTH) ? 'r' : '-';
    formattedPermissions[8] = (permissions & S_IWOTH) ? 'w' : '-';
    formattedPermissions[9] = (permissions & S_IXOTH) ? 'x' : '-';
    formattedPermissions[10] = '\0';

    printf("%s", formattedPermissions);
}

/**
 * Printa a lista de arquivos que esta na área do diretório em um formato
 * parecido com o 'tar -tvf'
 */
void printFilesList(FilesList *filesList) {
    FileInfo *currentFileInfo;
    struct passwd *userInformation;
    struct group *grp;
    char formattedDate[20];

    currentFileInfo = filesList->head;
    while (currentFileInfo) {
        printFilePermissions(currentFileInfo->permissions);

        userInformation = getpwuid(currentFileInfo->userId);
        if (!userInformation) {
            fprintf(stderr, "Error: could not get user information.\n");
            exit(1);
        }

        grp = getgrgid(userInformation->pw_gid);
        if (!grp) {
            fprintf(stderr, "Error: could not get group information.\n");
            exit(1);
        }

        printf(" %s/%s         ", userInformation->pw_name, grp->gr_name);
        printf("%8lld ", (long long)currentFileInfo->size);

        strftime(formattedDate, 20, "%Y-%m-%d %H:%M",
                 localtime(&currentFileInfo->lastModificationTime));

        printf("%s ", formattedDate);
        printf("%s \n", currentFileInfo->name);
        currentFileInfo = currentFileInfo->next;
    }
}

void removeFileFromFilesList(FilesList *filesList, char *filename) {
    FileInfo *currentFileInfo, *previousFileInfo;
    char* preparedFileName = prepareFileName(filename);
    

    currentFileInfo = filesList->head;
    previousFileInfo = NULL;
    while (currentFileInfo) {
        if (strcmp(currentFileInfo->name, preparedFileName) == 0) {
            if (previousFileInfo) {
                previousFileInfo->next = currentFileInfo->next;
            } else {
                filesList->head = currentFileInfo->next;
            }
            free(currentFileInfo);
            free(preparedFileName);
            filesList->size--;
            return;
        }
        previousFileInfo = currentFileInfo;
        currentFileInfo = currentFileInfo->next;
    }

    free(preparedFileName);
}

void updateFileInfoAfterDelete(FileInfo *fileInfo) {
    FileInfo *currentFileInfo;

    currentFileInfo = fileInfo;
    while (currentFileInfo) {
        currentFileInfo->location -= fileInfo->size;
        currentFileInfo->order--;
        currentFileInfo = currentFileInfo->next;
    }
}

void moveFileInfo(FilesList *filesList, FileInfo *targetFile,
                  FileInfo *fileToBeMoved) {
    FileInfo *currentFileInfo, *previousFileInfo;

    currentFileInfo = filesList->head;
    previousFileInfo = NULL;
    while (currentFileInfo) {
        if (strcmp(currentFileInfo->name,
                   prepareFileName(fileToBeMoved->name)) == 0) {
            if (previousFileInfo) {
                previousFileInfo->next = currentFileInfo->next;
            } else {
                filesList->head = currentFileInfo->next;
            }
            currentFileInfo->next = targetFile->next;
            targetFile->next = currentFileInfo;
            return;
        }
        previousFileInfo = currentFileInfo;
        currentFileInfo = currentFileInfo->next;
    }
}

FileInfo *findFileInfo(FilesList *filesList, char *filename) {
    FileInfo *currentFileInfo;
    char *preparedFileName = prepareFileName(filename);

    currentFileInfo = filesList->head;
    while (currentFileInfo) {
        if (strcmp(currentFileInfo->name, preparedFileName) == 0) {
            free(preparedFileName);
            return currentFileInfo;
        }
        currentFileInfo = currentFileInfo->next;
    }

    free(preparedFileName);
    return NULL;
}
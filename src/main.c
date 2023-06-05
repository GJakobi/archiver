#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 *  nome (sem espaços), UID (user ID), permissões, tamanho, data de modificação,
 * ordem no arquivo e localização.
 */
typedef struct FileInfo {
    char *name;                  /* File name*/
    uid_t userId;                /* user ID of owner */
    mode_t st_mode;              /* File type and mode */
    off_t st_size;               /* total size, in bytes */
    time_t lastModificationTime; /* time of last modification */
    int order;                   /* Order in the archive, starting from 1*/
    char *location;              /* File location*/
} FileInfo;

typedef enum { NONE, INSERT } COMMAND;

/**
 * Receives the fileName, the argc and argv params, and optind, that indicates
 * the index next argument to be read, in this case, the remaining files to be
 * inserted.
 */
void insertFilesIntoArchive(char *archiveFileName, int argc, char *argv[],
                            int optind) {
    FILE *archiveFile;
    int archiveFileExists;
    /* Indicates the start location of the directory area (it's stored in the
       end section of the archive) */
    int directoryAreaStart;

    archiveFileExists = access(archiveFileName, F_OK) != -1;

    archiveFile = fopen(archiveFileName, archiveFileExists ? "r+" : "w+");
    if (!archiveFile) {
        fprintf(stderr, "Error: could not open archive file %s.\n",
                archiveFileName);
        exit(1);
    }

    if (!archiveFileExists) {
        directoryAreaStart = 1;
        fwrite(&directoryAreaStart, sizeof(int), 1, archiveFile);
    } else {
        fread(&directoryAreaStart, sizeof(int), 1, archiveFile);
    }

    printf("directoryAreaStart: %d\n", directoryAreaStart);

    // Let's start inserting just one file....



    fclose(archiveFile);
}

int main(int argc, char *argv[]) {
    int option;
    char *archiveFileName = NULL;
    COMMAND command = NONE;

    while ((option = getopt(argc, argv, "i:")) != -1) {
        switch (option) {
            case 'i':
                archiveFileName = optarg;
                command = INSERT;
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

        default:
            break;
    }

    return 0;
}
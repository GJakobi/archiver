#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "./archive-manipulation/archive-manipulation.h"
#include "./buffer/buffer.h"
#include "./extract/extract.h"
#include "./files-list/files-list.h"

int main(int argc, char *argv[]) {
    int option;
    char *archiveFileName = NULL, *targetFileName = NULL;
    COMMAND command = NONE;

    while ((option = getopt(argc, argv, "i:c:x:r:a:m:h")) != -1) {
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
            case 'm':
                targetFileName = optarg;
                command = MOVE;
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
        case MOVE:
            moveFilesFromArchive(targetFileName, argc, argv, optind);
            break;
        case HELP:
            printHelp();
            break;
        default:
            break;
    }

    return 0;
}
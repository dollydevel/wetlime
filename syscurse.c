#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>

// SysCurce - инструмент безопасного удаления файлов и директорий. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  ____             ____                    
// / ___| _   _ ___ / ___|   _ _ __ ___  ___ 
// \___ \| | | / __| |  | | | | '__/ __|/ _ \
//  ___) | |_| \__ \ |__| |_| | |  \__ \  __/
// |____/ \__, |___/\____\__,_|_|  |___/\___|
//        |___/                              

#define BUF_SIZE 4096
int force = 0;
int recursive = 0;
void remove_file(const char *path) {
    if (unlink(path) == -1) {
        if (errno == EISDIR) {
            if (recursive) {
                return;
            } else {
                if (!force) {
                    fprintf(stderr, "SysCurse: невозможно удалить '%s': Это директория\n", path);
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            if (!force) {
                perror("rm");
                exit(EXIT_FAILURE);
            }
        }
    }
}
void remove_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        if (!force) {
            perror("rm");
            exit(EXIT_FAILURE);
        }
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char full_path[BUF_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        struct stat statbuf;
        if (lstat(full_path, &statbuf) == -1) {
            if (!force) {
                perror("rm");
                closedir(dir);
                exit(EXIT_FAILURE);
            }
            continue;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            remove_directory(full_path);
        } else {
            remove_file(full_path);
        }
    }
    closedir(dir);
    if (rmdir(path) == -1) {
        if (!force) {
            perror("rm");
            exit(EXIT_FAILURE);
        }
    }
}
void remove_path(const char *path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) == -1) {
        if (!force) {
            perror("rm");
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (S_ISDIR(statbuf.st_mode)) {
        if (recursive) {
            remove_directory(path);
        } else {
            if (!force) {
                fprintf(stderr, "SysCurse: невозможно удалить '%s': Это директория\n", path);
                exit(EXIT_FAILURE);
            }
        }
    } else {
        remove_file(path);
    }
}
void print_usage(const char *prog_name) {
    fprintf(stderr, "Использование: %s [-rf] файл...\n", prog_name);
    fprintf(stderr, "Удаляет файлы или директории.\n");
    fprintf(stderr, "  -r, -R, --recursive  удалять директории и их содержимое рекурсивно\n");
    fprintf(stderr, "  -f, --force           игнорировать несуществующие файлы и аргументы, никогда не запрашивать\n");
    exit(EXIT_FAILURE);
}
int main(int argc, char *argv[]) {
    int opt;
    struct option long_options[] = {
        {"recursive", no_argument, 0, 'r'},
        {"force", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "rRf", long_options, NULL)) != -1) {
        switch (opt) {
            case 'r':
            case 'R':
                recursive = 1;
                break;
            case 'f':
                force = 1;
                break;
            default:
                print_usage(argv[0]);
        }
    }
    if (optind >= argc) {
        print_usage(argv[0]);
    }
    for (int i = optind; i < argc; i++) {
        remove_path(argv[i]);
    }
    return EXIT_SUCCESS;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include <io.h>

// WindCurse - официальный порт SysCurse под операционную систему Windows. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
// __        ___           _  ____                    
// \ \      / (_)_ __   __| |/ ___|   _ _ __ ___  ___ 
//  \ \ /\ / /| | '_ \ / _` | |  | | | | '__/ __|/ _ \
//   \ V  V / | | | | | (_| | |__| |_| | |  \__ \  __/
//    \_/\_/  |_|_| |_|\__,_|\____\__,_|_|  |___/\___|

#define BUF_SIZE 4096
int force = 0;
int recursive = 0;
void remove_file(const char *path) {
    if (remove(path) != 0) {
        DWORD attr = GetFileAttributesA(path);
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            if (recursive) {
                return;
            } else {
                if (!force) {
                    fprintf(stderr, "WindCurse: невозможно удалить '%s': Это директория\n", path);
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
    WIN32_FIND_DATAA findFileData;
    char searchPath[BUF_SIZE];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);
    HANDLE hFind = FindFirstFileA(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        if (!force) {
            perror("rm");
            exit(EXIT_FAILURE);
        }
        return;
    }
    do {
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
            continue;
        }
        char full_path[BUF_SIZE];
        snprintf(full_path, sizeof(full_path), "%s\\%s", path, findFileData.cFileName);
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            remove_directory(full_path);
        } else {
            remove_file(full_path);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);
    FindClose(hFind);
    if (_rmdir(path) != 0) {
        if (!force) {
            perror("rm");
            exit(EXIT_FAILURE);
        }
    }
}
void remove_path(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (!force) {
            perror("rm");
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        if (recursive) {
            remove_directory(path);
        } else {
            if (!force) {
                fprintf(stderr, "WindCurse: невозможно удалить '%s': Это директория\n", path);
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
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "-R") == 0 || 
                strcmp(argv[i], "--recursive") == 0) {
                recursive = 1;
            } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
                force = 1;
            } else if (strcmp(argv[i], "-rf") == 0 || strcmp(argv[i], "-fr") == 0) {
                recursive = 1;
                force = 1;
            } else {
                print_usage(argv[0]);
            }
        }
    }
    int files_start = 1;
    while (files_start < argc && argv[files_start][0] == '-') {
        files_start++;
    }
    if (files_start >= argc) {
        print_usage(argv[0]);
    }
    for (int i = files_start; i < argc; i++) {
        remove_path(argv[i]);
    }
    return EXIT_SUCCESS;
}
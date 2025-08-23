#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>

// RetroClone - инструмент безопасного копирования данных. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  ____      _              ____ _                  
// |  _ \ ___| |_ _ __ ___  / ___| | ___  _ __   ___ 
// | |_) / _ \ __| '__/ _ \| |   | |/ _ \| '_ \ / _ \
// |  _ <  __/ |_| | | (_) | |___| | (_) | | | |  __/
// |_| \_\___|\__|_|  \___/ \____|_|\___/|_| |_|\___|

#define BUF_SIZE 4096
int recursive = 0;
int interactive = 0;
int path_exists(const char *path) {
    struct stat statbuf;
    return stat(path, &statbuf) == 0;
}
int ask_confirmation(const char *message) {
    printf("%s (y/n): ", message);
    fflush(stdout);
    char response[10];
    if (fgets(response, sizeof(response), stdin) == NULL) {
        return 0;
    }
    return (response[0] == 'y' || response[0] == 'Y');
}
void remove_path(const char *path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) == -1) {
        return;
    }
    if (S_ISDIR(statbuf.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            perror("cp");
            exit(EXIT_FAILURE);
        }
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            char sub_path[BUF_SIZE];
            snprintf(sub_path, sizeof(sub_path), "%s/%s", path, entry->d_name);
            remove_path(sub_path);
        }
        closedir(dir);
        if (rmdir(path) == -1) {
            perror("cp");
            exit(EXIT_FAILURE);
        }
    } else {
        if (unlink(path) == -1) {
            perror("cp");
            exit(EXIT_FAILURE);
        }
    }
}
void copy_file(const char *src, const char *dest) {
    if (path_exists(dest)) {
        if (interactive) {
            char message[BUF_SIZE];
            snprintf(message, sizeof(message), "RetroClone: перезаписать '%s'?", dest);
            if (!ask_confirmation(message)) {
                printf("RetroClone: пропуск '%s'\n", dest);
                return;
            }
        }
        remove_path(dest);
    }
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("cp");
        exit(EXIT_FAILURE);
    }
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        perror("cp");
        close(src_fd);
        exit(EXIT_FAILURE);
    }
    char buffer[BUF_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, buffer, BUF_SIZE)) > 0) {
        if (write(dest_fd, buffer, bytes_read) != bytes_read) {
            perror("cp");
            close(src_fd);
            close(dest_fd);
            exit(EXIT_FAILURE);
        }
    }
    if (bytes_read == -1) {
        perror("cp");
    }
    close(src_fd);
    close(dest_fd);
}
void copy_directory(const char *src, const char *dest) {
    if (path_exists(dest)) {
        if (interactive) {
            char message[BUF_SIZE];
            snprintf(message, sizeof(message), "RetroClone: перезаписать '%s'?", dest);
            if (!ask_confirmation(message)) {
                printf("RetroClone: пропуск '%s'\n", dest);
                return;
            }
        }
        remove_path(dest);
    }
    DIR *dir = opendir(src);
    if (!dir) {
        perror("cp");
        exit(EXIT_FAILURE);
    }
    if (mkdir(dest, 0755) == -1) {
        perror("cp");
        closedir(dir);
        exit(EXIT_FAILURE);
    }
    struct dirent *entry;
    struct stat statbuf;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char src_path[BUF_SIZE];
        char dest_path[BUF_SIZE];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);
        if (stat(src_path, &statbuf) == -1) {
            perror("cp");
            closedir(dir);
            exit(EXIT_FAILURE);
        }
        if (S_ISDIR(statbuf.st_mode)) {
            copy_directory(src_path, dest_path);
        } else {
            copy_file(src_path, dest_path);
        }
    }
    closedir(dir);
}
void print_usage(const char *prog_name) {
    fprintf(stderr, "Использование: %s [-ri] источник назначение\n", prog_name);
    fprintf(stderr, "Копирует файлы и директории.\n");
    fprintf(stderr, "  -r, --recursive  рекурсивное копирование директорий\n");
    fprintf(stderr, "  -i, --interactive запрашивать подтверждение перед перезаписью\n");
    exit(EXIT_FAILURE);
}
int main(int argc, char *argv[]) {
    int opt;
    struct option long_options[] = {
        {"recursive", no_argument, 0, 'r'},
        {"interactive", no_argument, 0, 'i'},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "ri", long_options, NULL)) != -1) {
        switch (opt) {
            case 'r':
                recursive = 1;
                break;
            case 'i':
                interactive = 1;
                break;
            default:
                print_usage(argv[0]);
        }
    }
    if (optind + 2 != argc) {
        print_usage(argv[0]);
    }
    const char *src = argv[optind];
    const char *dest = argv[optind + 1];
    struct stat statbuf;
    if (stat(src, &statbuf) == -1) {
        perror("cp");
        exit(EXIT_FAILURE);
    }
    if (S_ISDIR(statbuf.st_mode)) {
        if (recursive) {
            copy_directory(src, dest);
        } else {
            fprintf(stderr, "RetroClone: '%s' не скопирована: это директория\n", src);
            exit(EXIT_FAILURE);
        }
    } else {
        copy_file(src, dest);
    }
    return EXIT_SUCCESS;
}
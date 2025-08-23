#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>

// RetroMove - инструмент безопасного перемещения и переименования данных. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  ____      _             __  __                
// |  _ \ ___| |_ _ __ ___ |  \/  | _____   _____ 
// | |_) / _ \ __| '__/ _ \| |\/| |/ _ \ \ / / _ \
// |  _ <  __/ |_| | | (_) | |  | | (_) \ V /  __/
// |_| \_\___|\__|_|  \___/|_|  |_|\___/ \_/ \___|

#define BUF_SIZE 4096
int interactive = 0;
int force = 0;
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
void copy_file(const char *src, const char *dest) {
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("move");
        exit(EXIT_FAILURE);
    }
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        perror("move");
        close(src_fd);
        exit(EXIT_FAILURE);
    }
    char buffer[BUF_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(src_fd, buffer, BUF_SIZE)) > 0) {
        if (write(dest_fd, buffer, bytes_read) != bytes_read) {
            perror("move");
            close(src_fd);
            close(dest_fd);
            exit(EXIT_FAILURE);
        }
    }
    if (bytes_read == -1) {
        perror("move");
    }
    close(src_fd);
    close(dest_fd);
    if (unlink(src) == -1) {
        perror("move");
        exit(EXIT_FAILURE);
    }
}
void copy_directory(const char *src, const char *dest) {
    DIR *dir = opendir(src);
    if (!dir) {
        perror("move");
        exit(EXIT_FAILURE);
    }
    if (mkdir(dest, 0755) == -1) {
        perror("move");
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
            perror("move");
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
    if (rmdir(src) == -1) {
        perror("move");
        exit(EXIT_FAILURE);
    }
}
void move_path(const char *src, const char *dest) {
    struct stat src_stat, dest_stat;
    if (stat(src, &src_stat) == -1) {
        perror("move");
        exit(EXIT_FAILURE);
    }
    int dest_exists = stat(dest, &dest_stat) == 0;
    if (dest_exists) {
        if (S_ISDIR(dest_stat.st_mode)) {
            char final_dest[BUF_SIZE];
            const char *basename = strrchr(src, '/');
            if (basename == NULL) {
                basename = src;
            } else {
                basename++;
            }
            snprintf(final_dest, sizeof(final_dest), "%s/%s", dest, basename);
            if (path_exists(final_dest)) {
                if (interactive && !force) {
                    char message[BUF_SIZE];
                    snprintf(message, sizeof(message), "RetroMove: перезаписать '%s'?", final_dest);
                    if (!ask_confirmation(message)) {
                        printf("RetroMove: операция отменена\n");
                        exit(EXIT_SUCCESS);
                    }
                }
            }
            if (rename(src, final_dest) == -1) {
                if (errno == EXDEV) {
                    if (S_ISDIR(src_stat.st_mode)) {
                        copy_directory(src, final_dest);
                    } else {
                        copy_file(src, final_dest);
                    }
                } else {
                    perror("move");
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            if (interactive && !force) {
                char message[BUF_SIZE];
                snprintf(message, sizeof(message), "RetroMove: перезаписать '%s'?", dest);
                if (!ask_confirmation(message)) {
                    printf("RetroMove: операция отменена\n");
                    exit(EXIT_SUCCESS);
                }
            }
            if (unlink(dest) == -1) {
                perror("move");
                exit(EXIT_FAILURE);
            }
            if (rename(src, dest) == -1) {
                if (errno == EXDEV) {
                    copy_file(src, dest);
                } else {
                    perror("move");
                    exit(EXIT_FAILURE);
                }
            }
        }
    } else {
        if (rename(src, dest) == -1) {
            if (errno == EXDEV) {
                if (S_ISDIR(src_stat.st_mode)) {
                    copy_directory(src, dest);
                } else {
                    copy_file(src, dest);
                }
            } else {
                perror("move");
                exit(EXIT_FAILURE);
            }
        }
    }
}
void print_usage(const char *prog_name) {
    fprintf(stderr, "Использование: %s [-fi] источник назначение\n", prog_name);
    fprintf(stderr, "Перемещает или переименовывает файлы и директории.\n");
    fprintf(stderr, "  -f, --force       принудительное перемещение (без подтверждения)\n");
    fprintf(stderr, "  -i, --interactive запрашивать подтверждение перед перезаписью\n");
    exit(EXIT_FAILURE);
}
int main(int argc, char *argv[]) {
    int opt;
    struct option long_options[] = {
        {"force", no_argument, 0, 'f'},
        {"interactive", no_argument, 0, 'i'},
        {0, 0, 0, 0}
    };
    while ((opt = getopt_long(argc, argv, "fi", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f':
                force = 1;
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
    move_path(src, dest);
    printf("RetroMove: успешно перемещено '%s' -> '%s'\n", src, dest);
    return EXIT_SUCCESS;
}
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#define MAX_CMD_LEN 1024
#define MAX_ARGS 128
#define COLOR_BLUE  "\033[44m"
#define COLOR_RED   "\033[41m"
#define COLOR_GREEN "\033[42m"
#define COLOR_RESET "\033[0m"

// Командная оболочка MizuPipe для операционных систем на базе Linux. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  __  __ _           ____  _            
// |  \/  (_)_____   _|  _ \(_)_ __   ___ 
// | |\/| | |_  / | | | |_) | | '_ \ / _ \
// | |  | | |/ /| |_| |  __/| | |_) |  __/
// |_|  |_|_/___|\__,_|_|   |_| .__/ \___|
//                            |_|         

volatile sig_atomic_t interrupted = 0;
void sigint_handler(int sig) {
    (void)sig;
    interrupted = 1;
    write(STDOUT_FILENO, "\n", 1);
}
const char *system_dirs[] = {
    "/",
    "/bin",
    "/sbin",
    "/usr",
    "/lib",
    "/lib64",
    "/lib32",
    "/etc",
    "/var",
    "/boot",
    "/sys",
    NULL
};
const char *rest_dirs[] = {
    "/root",
    "/opt",
    "/mnt",
    "/dev",
    "/proc",
    "/srv",
    NULL
};
bool starts_with(const char *path, const char *prefix);
bool is_system_path(const char *path);
bool is_rest_path(const char *path);
bool can_read_path(const char *path);
void print_colored_path(const char *path);
void prompt();
int parse_command(char *line, char **argv);
int builtin_cmd(int argc, char **argv);
void execute_command(char **argv);
int handle_redirection(char **argv);
void execute_pipe(char *line);
void print_tree(const char *base_path, int depth);
int get_ansi_color_code(const char *name, int is_background);
void xecho(int argc, char *argv[]);
void tree_command(int argc, char *argv[]);
bool starts_with(const char *path, const char *prefix) {
    size_t len = strlen(prefix);
    return strncmp(path, prefix, len) == 0 && (path[len] == '/' || path[len] == '\0');
}
bool is_system_path(const char *path) {
    for (int i = 0; system_dirs[i] != NULL; i++) {
        if (starts_with(path, system_dirs[i])) {
            return true;
        }
    }
    return false;
}
bool is_rest_path(const char *path) {
    for (int i = 0; rest_dirs[i] != NULL; i++) {
        if (starts_with(path, rest_dirs[i])) {
            return true;
        }
    }
    return false;
}
bool can_read_path(const char *path) {
    return (access(path, R_OK) == 0);
}
void print_colored_path(const char *path) {
    if (!can_read_path(path)) {
        printf(COLOR_RED "%s" COLOR_RESET, path);
    } else if (is_system_path(path)) {
        printf(COLOR_BLUE "%s" COLOR_RESET, path);
    } else if (is_rest_path(path)) {
        printf(COLOR_RED "%s" COLOR_RESET, path);
    } else {
        printf(COLOR_GREEN "%s" COLOR_RESET, path);
    }
}
void prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        print_colored_path(cwd);
        if (geteuid() == 0) {
            printf("# ");
        } else {
            printf("$ ");
        }
    } else {
        if (geteuid() == 0) {
            printf("# ");
        } else {
            printf("$ ");
        }
    }
    fflush(stdout);
}
int parse_command(char *line, char **argv) {
    int argc = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    return argc;
}
int builtin_cmd(int argc, char **argv) {
    if (argv[0] == NULL) return 0;
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "cd: не найден аргумент\n");
        } else {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }
    if (strcmp(argv[0], "xecho") == 0 || strcmp(argv[0], "print") == 0 || strcmp(argv[0], "write") == 0) {
        xecho(argc, argv);
        return 1;
    }
    if (strcmp(argv[0], "tree") == 0 || strcmp(argv[0], "showtree") == 0 || strcmp(argv[0], "filetree") == 0 || strcmp(argv[0], "filelist") == 0) {
        tree_command(argc, argv);
        return 1;
    }
    return 0;
}
void execute_command(char **argv) {
    pid_t pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("fork");
    }
}
int handle_redirection(char **argv) {
    int i = 0;
    while (argv[i] != NULL) {
        if (strcmp(argv[i], ">") == 0) {
            if (argv[i+1] == NULL) {
                fprintf(stderr, "Ошибка синтаксиса! >\n");
                return -1;
            }
            int fd = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                return -1;
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            argv[i] = NULL;
            return 0;
        } else if (strcmp(argv[i], "<") == 0) {
            if (argv[i+1] == NULL) {
                fprintf(stderr, "Ошибка синтаксиса! <\n");
                return -1;
            }
            int fd = open(argv[i+1], O_RDONLY);
            if (fd < 0) {
                perror("open");
                return -1;
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            argv[i] = NULL;
            return 0;
        }
        i++;
    }
    return 0;
}
void execute_pipe(char *line) {
    char *cmd1_str = strtok(line, "|");
    char *cmd2_str = strtok(NULL, "|");
    if (cmd2_str == NULL) {
        char *argv[MAX_ARGS];
        int argc = parse_command(cmd1_str, argv);
        if (builtin_cmd(argc, argv)) return;
        pid_t pid = fork();
        if (pid == 0) {
            handle_redirection(argv);
            execvp(argv[0], argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            perror("fork");
        }
        return;
    }
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }
    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        char *argv1[MAX_ARGS];
        int argc1 = parse_command(cmd1_str, argv1);
        if (handle_redirection(argv1) < 0) exit(EXIT_FAILURE);
        execvp(argv1[0], argv1);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        char *argv2[MAX_ARGS];
        int argc2 = parse_command(cmd2_str, argv2);
        if (handle_redirection(argv2) < 0) exit(EXIT_FAILURE);
        execvp(argv2[0], argv2);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}
void print_tree(const char *base_path, int depth) {
    struct dirent *dp;
    DIR *dir = opendir(base_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }
    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        for (int i = 0; i < depth; i++) {
            printf("----");
        }
        printf("%s\n", dp->d_name);
        if (dp->d_type == DT_DIR) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", base_path, dp->d_name);
            print_tree(path, depth + 1);
        }
    }
    closedir(dir);
}
int get_ansi_color_code(const char *name, int is_background) {
    const char *colors[] = {
        "black",   "darkblue", "darkgreen", "darkcyan", "darkred",
        "darkmagenta", "darkyellow", "gray", "darkgray", "blue",
        "green", "cyan", "red", "magenta", "yellow", "white", NULL
    };
    int codes[] = {
        0, 4, 2, 6, 1,
        5, 3, 7, 8, 12,
        10, 14, 9, 13, 11, 15
    };
    for (int i = 0; colors[i] != NULL; i++) {
        if (strcmp(colors[i], name) == 0) {
            int base_code = codes[i];
            int bright = 0;
            if (base_code >= 8) {
                bright = 60;
                base_code -= 8;
            }
            if (is_background) {
                return 40 + base_code + bright;
            } else {
                return 30 + base_code + bright;
            }
        }
    }
    return -1;
}
void xecho(int argc, char *argv[]) {
    const char *fore_color_name = NULL;
    const char *back_color_name = NULL;
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--fore") == 0 || strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Ошибка: --fore и -f требуют название цвета в качестве аргумента\n");
                return;
            }
            fore_color_name = argv[i + 1];
            i += 2;
        } else if (strcmp(argv[i], "--back") == 0 || strcmp(argv[i], "-b") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Ошибка: --back и -b требуют название цвета в качестве аргумента\n");
                return;
            }
            back_color_name = argv[i + 1];
            i += 2;
        } else {
            break;
        }
    }
    if (i >= argc) {
        fprintf(stderr, "Ошибка: нет текста для вывода\n");
        return;
    }
    int fore_code = -1;
    int back_code = -1;
    if (fore_color_name != NULL) {
        fore_code = get_ansi_color_code(fore_color_name, 0);
        if (fore_code == -1) {
            fprintf(stderr, "Ошибка: неизвестный цвет фона '%s'\n", fore_color_name);
            return;
        }
    }
    if (back_color_name != NULL) {
        back_code = get_ansi_color_code(back_color_name, 1);
        if (back_code == -1) {
            fprintf(stderr, "Ошибка: неизвестный цвет текста '%s'\n", back_color_name);
            return;
        }
    }
    if (fore_code != -1 || back_code != -1) {
        printf("\033[");
        int first = 1;
        if (fore_code != -1) {
            printf("%d", fore_code);
            first = 0;
        }
        if (back_code != -1) {
            if (!first) printf(";");
            printf("%d", back_code);
        }
        printf("m");
    }
    for (; i < argc; i++) {
        printf("%s", argv[i]);
        if (i < argc - 1) printf(" ");
    }
    if (fore_code != -1 || back_code != -1) {
        printf("\033[0m");
    }
    printf("\n");
}
void tree_command(int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : ".";
    print_tree(path, 0);
}
int main() {
    signal(SIGINT, sigint_handler);
    char *line = NULL;
    size_t len = 0;
    while (1) {
        interrupted = 0;
        prompt();
        if (getline(&line, &len, stdin) == -1) {
            printf("\n");
            break;
        }
        if (interrupted) {
            continue;
        }
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) {
            continue;
        }
        execute_pipe(line);
    }
    free(line);
    return 0;
}
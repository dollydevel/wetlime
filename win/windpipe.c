#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <signal.h>
#include <stdbool.h>
#define MAX_CMD_LEN 1024
#define MAX_ARGS 128
#define COLOR_BLUE  "\033[44m"
#define COLOR_RED   "\033[41m"
#define COLOR_GREEN "\033[42m"
#define COLOR_RESET "\033[0m"

// WindPipe - официальный порт командной оболочки MizuPipe для операционной системы Windows. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
// __        ___           _ ____  _            
// \ \      / (_)_ __   __| |  _ \(_)_ __   ___ 
//  \ \ /\ / /| | '_ \ / _` | |_) | | '_ \ / _ \
//   \ V  V / | | | | | (_| |  __/| | |_) |  __/
//    \_/\_/  |_|_| |_|\__,_|_|   |_| .__/ \___|
//                                  |_|         

volatile sig_atomic_t interrupted = 0;
BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        interrupted = 1;
        printf("\n");
        return TRUE;
    }
    return FALSE;
}
const char *system_dirs[] = {
    "C:\\",
    "C:\\Windows",
    NULL
};
const char *rest_dirs[] = {
    "C:\\Program Files",
    "C:\\Program Files (x86)",
    "C:\\ProgramData",
    "C:\\PerfLogs",
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
char* get_current_directory();
void windows_sleep(int milliseconds);
bool starts_with(const char *path, const char *prefix) {
    size_t len = strlen(prefix);
    return _strnicmp(path, prefix, len) == 0 && (path[len] == '\\' || path[len] == '\0');
}
bool is_system_path(const char *path) {
    char full_path[MAX_PATH];
    if (_fullpath(full_path, path, MAX_PATH) == NULL) {
        return false;
    }
    for (int i = 0; system_dirs[i] != NULL; i++) {
        if (starts_with(full_path, system_dirs[i])) {
            return true;
        }
    }
    return false;
}
bool is_rest_path(const char *path) {
    char full_path[MAX_PATH];
    if (_fullpath(full_path, path, MAX_PATH) == NULL) {
        return false;
    }
    for (int i = 0; rest_dirs[i] != NULL; i++) {
        if (starts_with(full_path, rest_dirs[i])) {
            return true;
        }
    }
    return false;
}
bool can_read_path(const char *path) {
    return (_access(path, 04) == 0);
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
    char cwd[MAX_PATH];
    if (get_current_directory() != NULL) {
        strcpy(cwd, get_current_directory());
        print_colored_path(cwd);
        printf("$ ");
    } else {
        printf("$ ");
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
    if (_stricmp(argv[0], "exit") == 0) {
        exit(0);
    }
    if (_stricmp(argv[0], "cd") == 0 || _stricmp(argv[0], "chdir") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "cd: не найден аргумент\n");
        } else {
            if (_chdir(argv[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }
    if (_stricmp(argv[0], "xecho") == 0 || _stricmp(argv[0], "print") == 0 || _stricmp(argv[0], "write") == 0) {
        xecho(argc, argv);
        return 1;
    }
    if (_stricmp(argv[0], "tree") == 0 || _stricmp(argv[0], "showtree") == 0 || 
        _stricmp(argv[0], "filetree") == 0 || _stricmp(argv[0], "filelist") == 0) {
        tree_command(argc, argv);
        return 1;
    }
    if (_stricmp(argv[0], "cls") == 0 || _stricmp(argv[0], "clear") == 0) {
        system("cls");
        return 1;
    }
    if (_stricmp(argv[0], "dir") == 0 || _stricmp(argv[0], "ls") == 0) {
        char cmd[MAX_CMD_LEN] = "dir";
        if (argc > 1) {
            strcat(cmd, " ");
            for (int i = 1; i < argc; i++) {
                strcat(cmd, argv[i]);
                if (i < argc - 1) strcat(cmd, " ");
            }
        }
        system(cmd);
        return 1;
    }
    return 0;
}
void execute_command(char **argv) {
    char cmd_line[MAX_CMD_LEN] = "";
    for (int i = 0; argv[i] != NULL; i++) {
        strcat(cmd_line, argv[i]);
        if (argv[i + 1] != NULL) {
            strcat(cmd_line, " ");
        }
    }
    system(cmd_line);
}
int handle_redirection(char **argv) {
    int i = 0;
    while (argv[i] != NULL) {
        if (strcmp(argv[i], ">") == 0) {
            if (argv[i+1] == NULL) {
                fprintf(stderr, "Ошибка синтаксиса! >\n");
                return -1;
            }
            FILE *file = freopen(argv[i+1], "w", stdout);
            if (file == NULL) {
                perror("freopen");
                return -1;
            }
            argv[i] = NULL;
            return 0;  
        } else if (strcmp(argv[i], "<") == 0) {
            if (argv[i+1] == NULL) {
                fprintf(stderr, "Ошибка синтаксиса! <\n");
                return -1;
            }
            FILE *file = freopen(argv[i+1], "r", stdin);
            if (file == NULL) {
                perror("freopen");
                return -1;
            }
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
        int original_stdout = _dup(_fileno(stdout));
        int original_stdin = _dup(_fileno(stdin));
        if (handle_redirection(argv) < 0) {
            _close(original_stdout);
            _close(original_stdin);
            return;
        }
        execute_command(argv);
        _dup2(original_stdout, _fileno(stdout));
        _dup2(original_stdin, _fileno(stdin));
        _close(original_stdout);
        _close(original_stdin);
        return;
    }
    char temp1[MAX_PATH], temp2[MAX_PATH];
    GetTempPathA(MAX_PATH, temp1);
    GetTempFileNameA(temp1, "pipe", 0, temp1);
    GetTempPathA(MAX_PATH, temp2);
    GetTempFileNameA(temp2, "pipe", 0, temp2);
    char cmd1_with_redirect[MAX_CMD_LEN];
    snprintf(cmd1_with_redirect, sizeof(cmd1_with_redirect), "%s > %s", cmd1_str, temp1);
    system(cmd1_with_redirect);
    char cmd2_with_redirect[MAX_CMD_LEN];
    snprintf(cmd2_with_redirect, sizeof(cmd2_with_redirect), "%s < %s", cmd2_str, temp1);
    system(cmd2_with_redirect);
    remove(temp1);
    remove(temp2);
}
void print_tree(const char *base_path, int depth) {
    WIN32_FIND_DATAA find_data;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", base_path);
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        for (int i = 0; i < depth; i++) {
            printf("----");
        }
        printf("%s\n", find_data.cFileName);
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            char path[MAX_PATH];
            snprintf(path, sizeof(path), "%s\\%s", base_path, find_data.cFileName);
            print_tree(path, depth + 1);
        }
    } while (FindNextFileA(hFind, &find_data) != 0);
    FindClose(hFind);
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
        if (_stricmp(colors[i], name) == 0) {
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
        if (_stricmp(argv[i], "--fore") == 0 || _stricmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Ошибка: --fore и -f требуют название цвета в качестве аргумента\n");
                return;
            }
            fore_color_name = argv[i + 1];
            i += 2;
        } else if (_stricmp(argv[i], "--back") == 0 || _stricmp(argv[i], "-b") == 0) {
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
char* get_current_directory() {
    static char cwd[MAX_PATH];
    if (_getcwd(cwd, MAX_PATH) != NULL) {
        return cwd;
    }
    return NULL;
}
void windows_sleep(int milliseconds) {
    Sleep(milliseconds);
}
int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hConsole, &mode);
    SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleCtrlHandler(console_handler, TRUE);
    SetConsoleOutputCP(CP_UTF8);
    char line[MAX_CMD_LEN];
    while (1) {
        interrupted = 0;
        prompt();
        if (fgets(line, sizeof(line), stdin) == NULL) {
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
    return 0;
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Кроссплатформенное средство расширенного вывода информации Output. Версия 1.1
// Standalone-порт команды xecho из консольной оболочки MizuPipe (WindPipe).
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//   ___        _               _   
//  / _ \ _   _| |_ _ __  _   _| |_ 
// | | | | | | | __| '_ \| | | | __|
// | |_| | |_| | |_| |_) | |_| | |_ 
//  \___/ \__,_|\__| .__/ \__,_|\__|
//                 |_|              

typedef struct {
    const char *name;
    int code;
} Color;
Color colors[] = {
    {"black",   0},
    {"darkblue",4},
    {"darkgreen",2},
    {"darkcyan",6},
    {"darkred",1},
    {"darkmagenta",5},
    {"darkyellow",3},
    {"gray",7},
    {"darkgray",8},
    {"blue",12},
    {"green",10},
    {"cyan",14},
    {"red",9},
    {"magenta",13},
    {"yellow",11},
    {"white",15},
    {NULL, -1}
};
int get_ansi_color_code(const char *name, int is_background) {
    for (int i = 0; colors[i].name != NULL; i++) {
        if (strcmp(colors[i].name, name) == 0) {
            int base_code = colors[i].code;
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
void print_usage(const char *progname) {
    fprintf(stderr, "Использование: %s [--fore цвет | -f цвет] [--back цвет | -b цвет] [--nonewline | --single | -ln | -l | -s | -nnl] текст...\n", progname);
    fprintf(stderr, "Цвета: black, darkblue, darkgreen, darkcyan, darkred, darkmagenta, darkyellow, gray,\n");
    fprintf(stderr, "        darkgray, blue, green, cyan, red, magenta, yellow, white\n");
}
int main(int argc, char *argv[]) {
    const char *fore_color_name = NULL;
    const char *back_color_name = NULL;
    int no_newline = 0;
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--fore") == 0 || strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Ошибка: --fore и -f требуют название цвета в качестве аргумента\n");
                print_usage(argv[0]);
                return 1;
            }
            fore_color_name = argv[i + 1];
            i += 2;
        } else if (strcmp(argv[i], "--back") == 0 || strcmp(argv[i], "-b") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Ошибка: --back и -b требуют название цвета в качестве аргумента\n");
                print_usage(argv[0]);
                return 1;
            }
            back_color_name = argv[i + 1];
            i += 2;
        } else if (strcmp(argv[i], "--nonewline") == 0 || strcmp(argv[i], "--single") == 0 || 
                   strcmp(argv[i], "-ln") == 0 || strcmp(argv[i], "-l") == 0 || 
                   strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "-nnl") == 0) {
            no_newline = 1;
            i++;
        } else {
            break;
        }
    }
    if (i >= argc) {
        fprintf(stderr, "Ошибка: нет текста для вывода\n");
        print_usage(argv[0]);
        return 1;
    }
    int fore_code = -1;
    int back_code = -1;
    if (fore_color_name != NULL) {
        fore_code = get_ansi_color_code(fore_color_name, 0);
        if (fore_code == -1) {
            fprintf(stderr, "Ошибка: неизвестный цвет фона '%s'\n", fore_color_name);
            return 1;
        }
    }
    if (back_color_name != NULL) {
        back_code = get_ansi_color_code(back_color_name, 1);
        if (back_code == -1) {
            fprintf(stderr, "Ошибка: неизвестный цвет текста '%s'\n", back_color_name);
            return 1;
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
    if (!no_newline) {
        printf("\n");
    }
    return 0;
}

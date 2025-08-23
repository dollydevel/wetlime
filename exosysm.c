#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <locale.h>
#include <dirent.h>

// ExoSysm - средство для вывода основной информации о системе для Linux. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  _____          ____                      
// | ____|_  _____/ ___| _   _ ___ _ __ ___  
// |  _| \ \/ / _ \___ \| | | / __| '_ ` _ \ 
// | |___ >  < (_) |__) | |_| \__ \ | | | | |
// |_____/_/\_\___/____/ \__, |___/_| |_| |_|
//                       |___/               

char* get_cpu_info() {
    static char cpu_info[256] = "N/A";
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "model name")) {
                char *colon = strchr(line, ':');
                if (colon) {
                    colon += 2;
                    strncpy(cpu_info, colon, sizeof(cpu_info) - 1);
                    cpu_info[strcspn(cpu_info, "\n")] = 0;
                    break;
                }
            }
        }
        fclose(fp);
    }
    return cpu_info;
}
char* get_gpu_info() {
    static char gpu_info[256] = "N/A";
    FILE *fp = popen("lspci | grep -i 'vga\\|3d\\|display' | head -n 1", "r");
    if (fp) {
        if (fgets(gpu_info, sizeof(gpu_info), fp)) {
            gpu_info[strcspn(gpu_info, "\n")] = 0;
        } else {
            strcpy(gpu_info, "N/A");
        }
        pclose(fp);
    }
    if (strcmp(gpu_info, "N/A") == 0) {
        fp = popen("glxinfo 2>/dev/null | grep 'OpenGL renderer' | head -n 1", "r");
        if (fp) {
            if (fgets(gpu_info, sizeof(gpu_info), fp)) {
                char *colon = strchr(gpu_info, ':');
                if (colon) colon += 2;
                strncpy(gpu_info, colon ? colon : gpu_info, sizeof(gpu_info) - 1);
                gpu_info[strcspn(gpu_info, "\n")] = 0;
            }
            pclose(fp);
        }
    }
    return gpu_info;
}
char* get_desktop_environment() {
    static char de[64] = "N/A";
    char *de_env = getenv("XDG_CURRENT_DESKTOP");
    if (de_env && strlen(de_env) > 0) {
        strncpy(de, de_env, sizeof(de) - 1);
    } else {
        char *desktop = getenv("DESKTOP_SESSION");
        if (desktop && strlen(desktop) > 0) {
            strncpy(de, desktop, sizeof(de) - 1);
        }
    }
    return de;
}
char* get_display_server() {
    static char server[16] = "N/A";
    char *wayland = getenv("WAYLAND_DISPLAY");
    char *x11 = getenv("DISPLAY");
    if (wayland && strlen(wayland) > 0) {
        strcpy(server, "Wayland");
    } else if (x11 && strlen(x11) > 0) {
        strcpy(server, "X11");
    }
    return server;
}
char* get_theme_info(const char *type) {
    static char theme[128] = "N/A";
    if (strcmp(type, "gtk") == 0) {
        FILE *fp = popen("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null", "r");
        if (fp) {
            if (fgets(theme, sizeof(theme), fp)) {
                theme[strcspn(theme, "\n")] = 0;
                if (theme[0] == '\'') memmove(theme, theme + 1, strlen(theme));
                if (theme[strlen(theme)-1] == '\'') theme[strlen(theme)-1] = '\0';
            }
            pclose(fp);
        }
    } else if (strcmp(type, "cursor") == 0) {
        FILE *fp = popen("gsettings get org.gnome.desktop.interface cursor-theme 2>/dev/null", "r");
        if (fp) {
            if (fgets(theme, sizeof(theme), fp)) {
                theme[strcspn(theme, "\n")] = 0;
                if (theme[0] == '\'') memmove(theme, theme + 1, strlen(theme));
                if (theme[strlen(theme)-1] == '\'') theme[strlen(theme)-1] = '\0';
            }
            pclose(fp);
        }
    } else if (strcmp(type, "font") == 0) {
        FILE *fp = popen("gsettings get org.gnome.desktop.interface font-name 2>/dev/null", "r");
        if (fp) {
            if (fgets(theme, sizeof(theme), fp)) {
                theme[strcspn(theme, "\n")] = 0;
                if (theme[0] == '\'') memmove(theme, theme + 1, strlen(theme));
                if (theme[strlen(theme)-1] == '\'') theme[strlen(theme)-1] = '\0';
            }
            pclose(fp);
        }
    }
    return theme;
}
int main() {
    struct utsname sys_info;
    struct statvfs disk_info;
    struct sysinfo sys_info_struct;
    struct passwd *pw;
    if (uname(&sys_info) < 0) {
        perror("uname");
        return 1;
    }
    if (statvfs("/", &disk_info) != 0) {
        perror("statvfs");
        return 1;
    }
    unsigned long total_disk_space = disk_info.f_blocks * disk_info.f_frsize;
    pw = getpwuid(getuid());
    if (pw == NULL) {
        perror("getpwuid");
        return 1;
    }
    if (sysinfo(&sys_info_struct) != 0) {
        perror("sysinfo");
        return 1;
    }
    long swap_total = 0;
    FILE *fp = fopen("/proc/swaps", "r");
    if (fp) {
        char line[256];
        fgets(line, sizeof(line), fp);
        while (fgets(line, sizeof(line), fp)) {
            char swap_file[PATH_MAX];
            long size;
            if (sscanf(line, "%s %ld", swap_file, &size) == 2) {
                swap_total += size * 1024;
            }
        }
        fclose(fp);
    } else {
        swap_total = -1;
    }
    int columns = 0, rows = 0;
    FILE *tty = popen("stty size 2>/dev/null", "r");
    if (tty) {
        fscanf(tty, "%d %d", &rows, &columns);
        pclose(tty);
    }
    char *locale = setlocale(LC_ALL, NULL);
    if (!locale) locale = "N/A";
    printf("\n=== Системная информация ===\n");
    printf("ОС: %s %s\n", sys_info.sysname, sys_info.release);
    printf("Имя хоста: %s\n", sys_info.nodename);
    printf("Сборка ядра: %s\n", sys_info.version);
    printf("Архитектура: %s\n", sys_info.machine);
    printf("Имя пользователя: %s\n", pw->pw_name);
    printf("UID пользователя: %u\n", pw->pw_uid);
    printf("Размер файла подкачки: %s\n", swap_total == -1 ? "N/A" : "Available");
    if (swap_total > 0) printf("Размер файла подкачки: %.2f GB\n", swap_total / (1024.0 * 1024 * 1024));
    printf("Время работы: %.1f hours\n", sys_info_struct.uptime / 3600.0);
    printf("Оболочка: %s\n", pw->pw_shell);
    printf("Разрешение консоли: %dx%d\n", columns, rows);
    printf("\n=== Оборудование ===\n");
    printf("ЦП: %s\n", get_cpu_info());
    printf("Логические процессоры: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("ОЗУ: %ld MB\n", (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE)) / (1024 * 1024));
    printf("ПЗУ: %.2f GB\n", total_disk_space / (1024.0 * 1024 * 1024));
    printf("ГП: %s\n", get_gpu_info());
    printf("\n=== Графическая среда ===\n");
    printf("Тип локализации: %s\n", locale);
    printf("Графическая оболочка: %s\n", get_desktop_environment());
    printf("Система отображения: %s\n", get_display_server());
    printf("Тема GTK: %s\n", get_theme_info("gtk"));
    printf("Тема курсора: %s\n", get_theme_info("cursor"));
    printf("Шрифт: %s\n", get_theme_info("font"));
    return 0;
}
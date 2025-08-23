#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <sysinfoapi.h>
#include <lm.h>
#include <pdh.h>
#include <direct.h>
#include <locale.h>
#include <io.h>
#include <string.h>

// DumpSys - официальный порт ExoSysm для Windows. Версия 1.0
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  ____                       ____            
// |  _ \ _   _ _ __ ___  _ __/ ___| _   _ ___ 
// | | | | | | | '_ ` _ \| '_ \___ \| | | / __|
// | |_| | |_| | | | | | | |_) |__) | |_| \__ \
// |____/ \__,_|_| |_| |_| .__/____/ \__, |___/
//                       |_|         |___/     

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "pdh.lib")
char* get_cpu_info() {
    static char cpu_info[256] = "N/A";
    HKEY hKey;
    DWORD dwType = REG_SZ;
    DWORD dwSize = sizeof(cpu_info);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, &dwType, (LPBYTE)cpu_info, &dwSize) != ERROR_SUCCESS) {
            strcpy(cpu_info, "N/A");
        }
        RegCloseKey(hKey);
    }
    return cpu_info;
}
char* get_gpu_info() {
    static char gpu_info[256] = "N/A";
    HKEY hKey;
    DWORD dwType = REG_SZ;
    DWORD dwSize = sizeof(gpu_info);
    char keyPath[256];
    for (int i = 0; i < 10; i++) {
        sprintf(keyPath, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\%04d", i);
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hKey, "DriverDesc", NULL, &dwType, (LPBYTE)gpu_info, &dwSize) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return gpu_info;
            }
            RegCloseKey(hKey);
        }
    }
    return gpu_info;
}
char* get_desktop_environment() {
    static char de[64] = "Windows Desktop";
    return de;
}
char* get_display_server() {
    static char server[16] = "Windows GDI";
    return server;
}
char* get_theme_info(const char *type) {
    static char theme[128] = "N/A";
    if (strcmp(type, "gtk") == 0 || strcmp(type, "cursor") == 0 || strcmp(type, "font") == 0) {
        strcpy(theme, "Windows Default");
    }
    return theme;
}
void get_swap_info(unsigned long long *total_swap) {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    *total_swap = memInfo.ullTotalPageFile;
}
int main() {
    SYSTEM_INFO sysInfo;
    MEMORYSTATUSEX memInfo;
    OSVERSIONINFOA osInfo;
    char hostname[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(hostname);
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    unsigned long long total_disk_space = 0;
    unsigned long long total_swap = 0;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    char *locale;
    GetSystemInfo(&sysInfo);
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&osInfo);
    GetComputerNameA(hostname, &size);
    GetUserNameA(username, &username_len);
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        total_disk_space = totalNumberOfBytes.QuadPart;
    }
    get_swap_info(&total_swap);
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    locale = setlocale(LC_ALL, NULL);
    if (!locale) locale = "N/A";
    ULONGLONG uptime = GetTickCount64();
    printf("\n=== Системная информация ===\n");
    printf("ОС: Windows %d.%d.%d\n", osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber);
    printf("Имя хоста: %s\n", hostname);
    printf("Архитектура: ");
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: printf("x64\n"); break;
        case PROCESSOR_ARCHITECTURE_ARM: printf("ARM\n"); break;
        case PROCESSOR_ARCHITECTURE_ARM64: printf("ARM64\n"); break;
        case PROCESSOR_ARCHITECTURE_INTEL: printf("x86\n"); break;
        default: printf("Unknown\n"); break;
    }
    printf("Имя пользователя: %s\n", username);
    printf("Размер файла подкачки: %.2f GB\n", total_swap / (1024.0 * 1024 * 1024));
    printf("Время работы: %.1f hours\n", uptime / 3600000.0);
    printf("Разрешение консоли: %dx%d\n", columns, rows);
    printf("\n=== Оборудование ===\n");
    printf("ЦП: %s\n", get_cpu_info());
    printf("Логические процессоры: %d\n", sysInfo.dwNumberOfProcessors);
    printf("ОЗУ: %.2f GB\n", memInfo.ullTotalPhys / (1024.0 * 1024 * 1024));
    printf("ПЗУ: %.2f GB\n", total_disk_space / (1024.0 * 1024 * 1024));
    printf("ГП: %s\n", get_gpu_info());
    printf("\n=== Графическая среда ===\n");
    printf("Тип локализации: %s\n", locale);
    printf("Графическая оболочка: %s\n", get_desktop_environment());
    printf("Система отображения: %s\n", get_display_server());
    printf("Тема: %s\n", get_theme_info("gtk"));
    printf("Курсор: %s\n", get_theme_info("cursor"));
    printf("Шрифт: %s\n", get_theme_info("font"));
    return 0;
}
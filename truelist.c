#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Кроссплатформенное средство древовидного просмотра списка файлов TrueList. Версия 1.0
// Standalone-порт команды tree из консольной оболочки MizuPipe (WindPipe).
// Автор: Dolly! (aka DollyDev). Эта программа лицензирована под лицензией BSD-3-Clause.
//  _____                _     _     _   
// |_   _| __ _   _  ___| |   (_)___| |_ 
//   | || '__| | | |/ _ \ |   | / __| __|
//   | || |  | |_| |  __/ |___| \__ \ |_ 
//   |_||_|   \__,_|\___|_____|_|___/\__|

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    #include <dirent.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    
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

#elif defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    
    void print_tree(const char *base_path, int depth) {
        char search_path[MAX_PATH];
        WIN32_FIND_DATA find_data;
        HANDLE hFind;
        snprintf(search_path, sizeof(search_path), "%s\\*", base_path);
        hFind = FindFirstFile(search_path, &find_data);
        if (hFind == INVALID_HANDLE_VALUE) {
            return;
        }
        do {
            if (strcmp(find_data.cFileName, ".") == 0 || 
                strcmp(find_data.cFileName, "..") == 0) {
                continue;
            }
            for (int i = 0; i < depth; i++) {
                printf("----");
            }
            printf("%s\n", find_data.cFileName);
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                char new_path[MAX_PATH];
                snprintf(new_path, sizeof(new_path), "%s\\%s", base_path, find_data.cFileName);
                print_tree(new_path, depth + 1);
            }
        } while (FindNextFile(hFind, &find_data) != 0);
        FindClose(hFind);
    }

#else
    #error "Эта платформа не поддерживается"
#endif

int main(int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : ".";
    print_tree(path, 0);
    return 0;
}

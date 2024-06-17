#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *
extract_std_lib_path(char *argv_0, char *std_lib)
{
    uint32_t magic, pwd_len, lst_dlm;
    char    *std_lib_full_path;

    pwd_len = 0;
    lst_dlm = 0;
    while (argv_0[pwd_len] != '\0') {
        if (argv_0[pwd_len] == '/') {
            lst_dlm = pwd_len;
        }
        pwd_len++;
    }

    std_lib_full_path = malloc(sizeof(char) * (lst_dlm + strlen(std_lib)) + 1);

    strncpy(std_lib_full_path, argv_0, lst_dlm);
    sprintf(std_lib_full_path + lst_dlm, "/%s", std_lib);

    return std_lib_full_path;
}

size_t
extract_classpathes(size_t argc, char **argv, char *default_bootpath,
                    char *default_classpath, char ***classpath_arr_pointer)
{
    uint32_t classpath_count, classpath_pos, bootpath_pos, j, cp_start;
    char   **classpath_arr;

    classpath_count = 2;
    classpath_pos = -1;
    bootpath_pos = -1;
    for (size_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-cp") == 0) {
            classpath_pos = i + 1;
            j = 0;
            while (argv[classpath_pos][j] != '\0') {
                if (argv[classpath_pos][j] == ':') {
                    classpath_count++;
                }

                j++;
            }
        } else if (strcmp(argv[i], "-bt") == 0) {
            bootpath_pos = i + 1;
        }
    }

    classpath_arr = malloc(sizeof(char *) * (classpath_count));

    if (bootpath_pos == -1) {
        classpath_arr[0] = default_bootpath;
    } else {
        classpath_arr[0] = argv[bootpath_pos];
    }

    if (classpath_pos == -1) {
        classpath_arr[1] = default_classpath;
    } else {
        j = 0;
        cp_start = 0;

        for (size_t i = 1; i < classpath_count; i++) {
            while (argv[classpath_pos][j] != ':'
                   && argv[classpath_pos][j] != '\0') {
                j++;
            }

            classpath_arr[i] = malloc(sizeof(char) * j - cp_start);
            strncpy(classpath_arr[i], argv[classpath_pos] + cp_start,
                    j - cp_start);
            cp_start = ++j;
        }
    }

    *classpath_arr_pointer = classpath_arr;
    return classpath_count;
}

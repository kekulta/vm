#include <stdlib.h>

char  *extract_std_lib_path(char *argv_0, char *std_lib);

size_t extract_classpathes(size_t argc, char **argv, char *default_bootpath,
                           char   *default_classpath,
                           char ***classpath_arr_pointer);

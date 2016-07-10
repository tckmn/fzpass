#include <stdio.h>
#include <stdlib.h>
int main(int argc, char* argv[]) {
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t len;
    while ((len = getline(&line, &bufsize, stdin)) != -1) {
        int *highlight = calloc(len, sizeof *highlight);
        for (int i = 1; i < argc; ++i) {
            int pos = 0;
            for (char *pat = argv[i]; *pat; ++pat) {
                for(; pos < len && line[pos] != *pat; ++pos);
                if (pos == len) goto skipprint;
                highlight[pos++] = 30 + i;
            }
        }
        for (int i = 0; i < len; ++i) {
            if (highlight[i]) printf("\e[1m");
            printf("\e[%dm%c", highlight[i], line[i]);
        }
skipprint:
        free(highlight);
    }
    free(line);
    return 0;
}

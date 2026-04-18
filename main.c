#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "include/commit.h"
#include "include/tree.h"

#define CIT "/.cit"

extern char *g_cwd;

int main(int argc, char *argv[]) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd failed");
        return 1;
    }
    g_cwd = strdup(cwd);
    if (g_cwd == NULL) {
        perror("strdup failed");
        return 1;
    }
    // check if initialized
    DIR *dir = opendir(cwd);
    if (dir == NULL) {
        perror("opendir failed");
        free(g_cwd);
        return 1;
    }
    struct dirent *entry;
    bool init = false;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(".cit", entry->d_name) == 0) {
            char citPath[1024 + 5];
            struct stat st;
            snprintf(citPath, sizeof(citPath), "%s/.cit", cwd);
            if (stat(citPath, &st) == 0 && S_ISDIR(st.st_mode)) {
                init = true;
                break;
            }
        }
    }
    closedir(dir);

    if (!init) {
        char citDir[1024 + 5];
        snprintf(citDir, sizeof(citDir), "%s/.cit", cwd);
        if (mkdir(citDir, 0755) == -1) { perror("mkdir failed"); free(g_cwd); return 1; }
        printf("cit initialized at %s\n", ctime(&(time_t){time(NULL)}));
        free(g_cwd);
        return 0;
    }

    if (argc < 4) { fprintf(stderr, "usage: ./main . commit <message>\n"); free(g_cwd); return 1; }
    if (strcmp(argv[2], "commit") != 0) { fprintf(stderr, "unknown command: %s\n", argv[2]); free(g_cwd); return 1; }

    struct tree *tree = CreateTree(cwd);
    if (tree == NULL) {
        fprintf(stderr, "failed to build tree\n");
        free(g_cwd);
        return 1;
    }
    struct commit *last = loadLastCommit(cwd);
    struct commit *commit = Commit(tree, last, argv[3], cwd);
    (void)commit;
    free(g_cwd);
    return 0;
}
// comment in main should appear as +
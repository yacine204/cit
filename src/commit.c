#include "include/commit.h"
#include "include/tree.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>

struct commit *readCommit(char *path) {
    
}

struct commit *loadLastCommit(char *cwd) {
     
}

void writeTree(struct tree *tree, struct tree* parent_tree, FILE *f) {
    
}

void writeCommit(struct commit *commit, char *cwd){
    if (commit == NULL || commit->header == NULL) return;
   
}

struct commit *Commit(struct tree *commit_tree, struct commit *parent, char *commit_message, char *filepath) {
    struct commit *new_commit = malloc(sizeof(struct commit));
    
    return new_commit;
}
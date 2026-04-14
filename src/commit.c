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

    char path[2048];
    snprintf(path, sizeof(path), "%s/.cit/%s_%ld.cit", cwd, commit->header->commit_message, (long)commit->header->committed_at);
    FILE *f = fopen(path, "wb");
    if (f == NULL) { perror("fopen failed"); return; }
    struct tree *parent_tree = commit->parent ? commit->parent->commit_tree : NULL;

    fwrite(commit->header, sizeof(struct commitHeader), 1,f);

    writeTree(commit->commit_tree, parent_tree,f);
    fclose(f);
}

struct commit *Commit(struct tree *commit_tree, struct commit *parent, char *commit_message, char *filepath) {
    struct commit *new_commit = malloc(sizeof(struct commit));
    new_commit->header = malloc(sizeof(struct commitHeader)); 
    strncpy(new_commit->header->magic, "CIT1", 4);
    new_commit->commit_tree = commit_tree;
    new_commit->header->committed_at = time(NULL);
    strcpy(new_commit->header->commit_message, commit_message);
    strcpy(new_commit->header->filename, filepath);

    new_commit->parent = parent;  

    writeCommit(new_commit, filepath);
    return new_commit;
}
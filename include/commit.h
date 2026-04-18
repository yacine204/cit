#ifndef COMMIT_H
#define COMMIT_H

#include "tree.h"

#include <time.h>
#include <stdint.h>
#include <stdio.h>



struct commit{
    struct tree *commit_tree;
    struct commit *parent;
    time_t created_at;
    char *commit_message;
};

struct commit *Commit(struct tree *commit_tree, struct commit *parent, char *commit_message, char *filepath);
void writeCommit(struct commit *commit, char *cwd);
void writeTree(struct tree *tree, FILE *f);
int count_all_nodes(struct tree *tree);
struct commit *loadLastCommit(char *cwd);
#endif
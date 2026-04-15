#ifndef COMMIT_H
#define COMMIT_H

#include "tree.h"

#include <time.h>
#include <stdint.h>



struct commit{
    struct tree *commit_tree;
    struct commit *parent;
};

struct commit *Commit(struct tree *commit_tree, struct commit *parent, char *commit_message, char *filepath);
void writeCommit(struct commit *commit, char *cwd);
struct commit *loadLastCommit(char *cwd);
struct commit *readCommit(char *path);

#endif

#ifndef COMMIT_H
#define COMMIT_H

#include "tree.h"
#include <time.h>

struct commit{
    time_t commited_at;
    char *commit_message;
    struct tree commit_tree;
    struct commit *parent;
};

struct commit Commit(struct tree *commit_tree, struct commit *commit);

#endif

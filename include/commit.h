#ifndef COMMIT_H
#define COMMIT_H

#include "tree.h"

#include <time.h>
#include <stdint.h>

struct commitHeader {
    char magic[4];
    char filename[512];
    uint32_t commitIndex;
    uint32_t file_count;
    int32_t parent_index;  
    time_t committed_at;
    char commit_message[1024];
};


struct commit{
    struct commitHeader *header;
    struct tree *commit_tree;
    struct commit *parent;
};

struct commit *Commit(struct tree *commit_tree, struct commit *parent, char *commit_message, char *filepath);
void writeCommit(struct commit *commit, char *cwd);
struct commit *loadLastCommit(char *cwd);
struct commit *readCommit(char *path);

#endif

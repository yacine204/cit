#ifndef TREE_H
#define TREE_H

#include <time.h>


enum NodeType{
    FOLDER,
    FILE_NODE
};

struct node{
    enum NodeType nodeType;
    char *context;
    char hash[256];
    char *changes;
    char file_name[256];
    time_t created_at;
};

struct tree{
    char *root;
    struct node *nodes[100];
    int node_count;
    int root_count;
    struct tree *sub_trees[100];
};

struct node *CreateNode(char *context ,char *filename, enum NodeType nodeType); 
struct tree *CreateTree(char *root_path); 
char *diff(char *old, char *new);

#endif
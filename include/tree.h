#ifndef TREE_H
#define TREE_H

#include <time.h>


enum NodeType{
    FOLDER,
    FILE_NODE
};

struct nodeHeader{
    char *fileName;
    size_t index;
};

struct node{
    struct nodeHeader *nodeHeader;
    enum NodeType nodeType;
    char *context;
    char hash[256];
    char *changes;
};

struct tree{
    char *root;
    struct node **nodes;
    int node_count;
    int root_count;
    struct tree **sub_trees;
};

struct commit;

int count_lines(char *buffer);
char **split_lines(char *buffer);
char *generate_changes(char *old, char *new);
char* last_commit(void);
struct node *old_node_version(struct node *currentNode);
struct node *CreateNode(char *context ,char *filename, enum NodeType nodeType); 
struct tree *CreateTree(char *root_path); 
struct commit *read_from_binary(char *file_path);

#endif
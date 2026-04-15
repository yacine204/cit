#include "include/commit.h"
#include "include/tree.h"
#include "include/hash.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <paths.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

//helpers
int countLines(char *buffer){
    int counter = 0;
    char *copy = strdup(buffer);
    char *token = strtok(copy, "\n");

    while(token != NULL){
        counter ++;
        token = strtok(NULL, "\n");
        
    }
    free(copy);
    return counter;
}

char **split_lines(char *buffer) {

    int capacity = countLines(buffer);
    char **lines = malloc(capacity*(sizeof(char *)));
    char *copy = strdup(buffer);
    char *token = strtok(copy, "\n");
    

    for(int i=0; i<capacity; i++){
        lines[i] = strdup(token);
        token = strtok(NULL, "\n");
    }

    return lines;
}


char *generate_changes(char *old, char*new){
    //if new has has strings added on the indexes + the max len of the old then automatically make it an add on
    //if we find non similar lines before max len of old buffer we do:
    // +new buffer line
    // -old buffer line

    int old_len = countLines(old);
    int new_len = countLines(new);

    char **lined_old = split_lines(old);
    char **lined_new = split_lines(new);

    

    int changes_size = strlen(old) + strlen(new) + 10;
    char *changes = malloc(changes_size);
    changes[0]= '\0';
    
    int offset = 0;

    int min_len = old_len < new_len ? old_len : new_len;

    for(int i=0; i<min_len; i++){
        if(strcmp(lined_old[i],lined_new[i])!=0){
            offset += snprintf(changes+offset, changes_size - offset, "-%s\n+%s\n", lined_old[i], lined_new[i]);
        }
    }

    // deleted
    for (int i = min_len; i < old_len; i++) {
        offset += snprintf(changes + offset, changes_size - offset,
                           "-%s\n", lined_old[i]);
    }
    // add ons
    for (int i = min_len; i < new_len; i++) {
        offset += snprintf(changes + offset, changes_size - offset,
                           "+%s\n", lined_new[i]);
    }

    for (int i = 0; i < old_len; i++) free(lined_old[i]);
    free(lined_old);
    for (int i = 0; i < new_len; i++) free(lined_new[i]);
    free(lined_new);

    printf("%s", changes);
    return changes;
}

struct commit *findCommitFromNodeHash(struct commit *head,char *nodeHash){
    struct commit *commit = head;
    int commitTreeNodeCount = 0;
    
    while(commit!=NULL){
        commitTreeNodeCount = commit->commit_tree->node_count;
        
        for(int i=0; i<commitTreeNodeCount; i++){
            if(strcmp(commit->commit_tree->nodes[i]->hash, nodeHash)==0) return commit;
        }
        commit = commit->parent;
    }

    return NULL;
}


struct node *CreateNode(char *context, char *filename, enum NodeType nodeType){
    //if folder we only care ab name and creation timestamp
    struct node *new_node = malloc(sizeof(struct node));

    new_node->created_at = time(NULL);
    new_node->nodeType = nodeType;
    new_node->context = context;    

    new_node->nodeHeader = malloc(sizeof(struct nodeHeader));

    strcpy(new_node->nodeHeader->fileName, filename);

    new_node->changes = NULL;

    hash_node(new_node);

    if(nodeType == FILE_NODE){
        struct commit *node_parent = findCommitFromNodeHash(NULL,new_node->hash);
        
        
        if (node_parent != NULL){
            //get the same node from the parent of the node parent
            struct commit *node_parent_parent = node_parent->parent;
            
            //TODO: access the node in the node_parent_parent
        }else{
            new_node->nodeHeader->index = 0;
        }
    }

    
    return new_node;
}

struct tree *CreateTree(char *root_path) {
    struct tree *tree = malloc(sizeof(struct tree));
    tree->root = strdup(root_path);
    tree->node_count = 0;
    tree->root_count = 0;

    DIR *dir = opendir(root_path);
    if (dir == NULL) return tree;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) continue;

        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", root_path, entry->d_name);

        struct stat s;
        stat(full_path, &s);

        if (S_ISDIR(s.st_mode)) {
            if (strcmp(entry->d_name, ".git") == 0 ||
                strcmp(entry->d_name, ".cit") == 0) continue;
            struct tree *subtree = CreateTree(full_path);
            tree->sub_trees[tree->root_count++] = subtree;
        } else if (S_ISREG(s.st_mode)) {
            FILE *f = fopen(full_path, "r");
            
            if(f==NULL){
                perror("couldnt open file");
            }
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            rewind(f);

            char *buffer = malloc(size+1);
            if(buffer == NULL){
                fclose(f);
                return NULL;
            }
            fread(buffer, 1,size, f);
            buffer[size] = '\0';
            fclose(f);
            struct node *node = CreateNode(buffer, entry->d_name, FILE_NODE);
            tree->nodes[tree->node_count++] = node;
        }
    }

    closedir(dir);
    return tree;
}


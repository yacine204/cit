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

char *g_cwd = NULL;

//helpers
int count_lines(char *buffer){
    if(buffer == NULL || buffer[0] == '\0') return 0;
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

    int capacity = count_lines(buffer);
    if(capacity == 0) {
        char **lines = malloc(sizeof(char *));
        lines[0] = malloc(1);
        lines[0][0] = '\0';
        return lines;
    }
    
    char **lines = malloc(capacity*(sizeof(char *)));
    char *copy = strdup(buffer);
    char *token = strtok(copy, "\n");
    

    for(int i=0; i<capacity; i++){
        if(token == NULL) break;
        lines[i] = strdup(token);
        token = strtok(NULL, "\n");
    }
    free(copy);
    return lines;
}


char *generate_changes(char *old, char*new){
    //if new has has strings added on the indexes + the max len of the old then automatically make it an add on
    //if we find non similar lines before max len of old buffer we do:
    // +new buffer line
    // -old buffer line

    int old_len = count_lines(old);
    int new_len = count_lines(new);

    char **lined_old = split_lines(old);
    char **lined_new = split_lines(new);

    // Allocate enough space - each line could be doubled with +/- prefix, plus newlines
    size_t old_size = old ? strlen(old) : 0;
    size_t new_size = new ? strlen(new) : 0;
    int changes_size = old_size + new_size + (old_len + new_len) * 3 + 100;
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

    return changes;
}

char* last_commit(){
    //extract all commits timestamps from .cit
    //return the latest commit file name

    char cit_path[2048];
    snprintf(cit_path, sizeof(cit_path), "%s/.cit", g_cwd);
    
    DIR *dir = opendir(cit_path);
    if(dir == NULL){
        return NULL;
    }
    
    struct dirent *entry; 
    long latest_timestamp = 0;
    char *latest = NULL;

    while((entry = readdir(dir))!=NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char *copy = strdup(entry->d_name);
        char *token = strrchr(copy, '_');
        if (token == NULL) { free(copy); continue; }
        long timestamp = atoi(token + 1);

        if(timestamp>latest_timestamp){
            latest_timestamp = timestamp;
            free(latest);
            latest = strdup(entry->d_name);
        } 
        free(copy);
    }

    closedir(dir);
    return latest;
}

#define MAX_NODES 256

//returns a commit from a binary file
//binray's contains format like:
// ----------------------------------------------
// ##commit timestamp sub_tree_count node_counts
                                                    
// ##node filename                              
// ##changes                                     
// {node1_changes}
// ##context
// {node1_context}


// ##node filename
// ##changes
// {node2_changes}
// ##context
// {node2_context}


// ##node filename
// ##changes
// {node2_changes}
// ##context
// {node2_context}
// ------------------------------------------

struct commit *read_from_binary(char *file_path){
    if (file_path == NULL) return NULL;

    struct commit *commit = malloc(sizeof(struct commit));
    commit->commit_tree = malloc(sizeof(struct tree));
    commit->commit_tree->nodes = malloc(sizeof(struct node*)*256);
    
    FILE *f = fopen(file_path, "r");
    if(f==NULL){
        free(commit->commit_tree->nodes);
        free(commit->commit_tree);
        free(commit);
        return NULL;
    }

    fscanf(f, "##commit %ld %d %d\n", &commit->created_at, &commit->commit_tree->root_count, &commit->commit_tree->node_count);
    int node_count = commit->commit_tree->node_count;
    for(int i=0; i<node_count; i++){
        struct node *temp = malloc(sizeof(struct node));
        memset(temp, 0, sizeof(struct node)); 
        temp->nodeHeader = malloc(sizeof(struct nodeHeader));
        temp->nodeHeader->fileName = malloc(256); 

        fscanf(f, "##node %255s\n", temp->nodeHeader->fileName);

        char line[1024];
        long pos;
        fscanf(f, " ##changes\n");
        size_t changes_size = 0;
        long changes_start = ftell(f);
        int loop_count = 0;
        while((pos=ftell(f)), fgets(line, sizeof(line),f)){
            loop_count++;
            if(loop_count > 10000) break;
            if(strncmp(line, "##",2)==0){fseek(f,pos,SEEK_SET);break;}
            changes_size += strlen(line);
        }

        temp->changes = malloc(changes_size+1);
        if(temp->changes == NULL) {
            free(temp->nodeHeader->fileName);
            free(temp->nodeHeader);
            free(temp);
            continue;
        }
        temp->changes[0] = '\0';
        fseek(f, changes_start, SEEK_SET);
        loop_count = 0;
        while((pos=ftell(f)),fgets(line, sizeof(line),f)) {
            loop_count++;
            if(loop_count > 10000) break;
            if(strncmp(line, "##",2)==0){ fseek(f,pos, SEEK_SET);break;}
            strcat(temp->changes, line);
        }

        fscanf(f, " ##context\n");
        size_t context_size = 0;

        long context_start = 0;
        context_start = ftell(f);
        loop_count = 0;
        while ((pos = ftell(f)), fgets(line, sizeof(line), f)) {
            loop_count++;
            if(loop_count > 10000) break;
            if (strncmp(line, "##", 2) == 0) { fseek(f, pos, SEEK_SET); break; }
            context_size += strlen(line);
        }

        temp->context = malloc(context_size + 1);
        if(temp->context == NULL) {
            free(temp->changes);
            free(temp->nodeHeader->fileName);
            free(temp->nodeHeader);
            free(temp);
            continue;
        }
        temp->context[0] = '\0';
        fseek(f, context_start, SEEK_SET);
        loop_count = 0;
        while ((pos = ftell(f)), fgets(line, sizeof(line), f)) {
            loop_count++;
            if(loop_count > 10000) break;
            if (strncmp(line, "##", 2) == 0) { fseek(f, pos, SEEK_SET); break; }
            strcat(temp->context, line);
        }

        commit->commit_tree->nodes[i] = temp;        
    }
    fclose(f);
    return commit;
}

struct node *old_node_version(struct node *currentNode){
    // TODO: Fix this function - for now, return NULL to avoid hanging
    return NULL;
    /*
    char *latest_commit_file_name = last_commit();
    if (latest_commit_file_name == NULL) return NULL;

    char commit_path[512];
    snprintf(commit_path, sizeof(commit_path), ".cit/%s", latest_commit_file_name);
    free(latest_commit_file_name);

    struct commit *commit = read_from_binary(commit_path);
    if (commit == NULL) return NULL;

    struct node *old_version = malloc(sizeof(struct node));
    for(int i=0; i<commit->commit_tree->node_count; i++){
        if(strcmp(currentNode->nodeHeader->fileName, commit->commit_tree->nodes[i]->nodeHeader->fileName)==0){
            memcpy(old_version, commit->commit_tree->nodes[i], sizeof(struct node));
            return old_version;
        }
    }
    free(old_version);
    return NULL;
    */
}

struct node *CreateNode(char *context, char *filename, enum NodeType nodeType){
    //only create when encountering a file
    struct node *new_node = malloc(sizeof(struct node));
    memset(new_node, 0, sizeof(struct node));
    new_node->nodeHeader = malloc(sizeof(struct nodeHeader));
    new_node->nodeHeader->fileName = strdup(filename);
    new_node->context = strdup(context);
    new_node->changes = NULL;

    if(nodeType==FILE_NODE){
        struct node *old = old_node_version(new_node);
        if (old != NULL){
            new_node->changes = generate_changes(old->context, new_node->context);
            free(old);
        } else {
            // no old version this is a new file or first commit
            // generate changes showing all lines as additions
            new_node->changes = generate_changes("", new_node->context);
        }
    }
    return new_node;
}

struct tree *CreateTree(char *root_path) {
    struct tree *tree = malloc(sizeof(struct tree));
    if (tree == NULL) return NULL;
    tree->root = strdup(root_path);
    tree->nodes = malloc(sizeof(struct node *) * MAX_NODES);
    tree->sub_trees = malloc(sizeof(struct tree *) * MAX_NODES);
    if (tree->root == NULL || tree->nodes == NULL || tree->sub_trees == NULL) {
        free(tree->root);
        free(tree->nodes);
        free(tree->sub_trees);
        free(tree);
        return NULL;
    }
    tree->node_count = 0;
    tree->root_count = 0;

    DIR *dir = opendir(root_path);
    if (dir == NULL) return tree;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", root_path, entry->d_name);

        struct stat s;
        if (stat(full_path, &s) != 0) {
            continue;
        }

        if (S_ISDIR(s.st_mode)) {
            if (strcmp(entry->d_name, ".cit") == 0 || strcmp(entry->d_name, ".git") == 0) continue;
            struct tree *subtree = CreateTree(full_path);
            tree->sub_trees[tree->root_count++] = subtree;
        } else if (S_ISREG(s.st_mode)) {
            FILE *f = fopen(full_path, "r");
            
            if(f==NULL){
                perror("couldnt open file");
                continue;
            }
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            if (size < 0) {
                fclose(f);
                continue;
            }
            rewind(f);

            char *buffer = malloc(size+1);
            if(buffer == NULL){
                fclose(f);
                continue;
            }
            fread(buffer, 1,size, f);
            buffer[size] = '\0';
            fclose(f);
            struct node *node = CreateNode(buffer, entry->d_name, FILE_NODE);
            free(buffer);
            if (node != NULL) {
                tree->nodes[tree->node_count++] = node;
            }
        }
    }

    closedir(dir);
    return tree;
}

 
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

static int split_lines(char *text, char **lines, int max_lines) {
    int count = 0;
    char *line = strtok(text, "\n");
    while (line != NULL && count < max_lines) {
        lines[count++] = line;
        line = strtok(NULL, "\n");
    }
    return count;
}

char *diff(char *old, char *new) {
    
    size_t old_sz = strlen(old) + 1;
    size_t new_sz = strlen(new) + 1;
    char *old_copy = malloc(old_sz);
    char *new_copy = malloc(new_sz);
    if (!old_copy || !new_copy) { free(old_copy); free(new_copy); return NULL; }
    memcpy(old_copy, old, old_sz);
    memcpy(new_copy, new, new_sz);
 
    char *old_lines[256], *new_lines[256];
    int m = split_lines(old_copy, old_lines, 256);
    int n = split_lines(new_copy, new_lines, 256);
 
    // build LCS table
    int dp[m + 1][n + 1];
    for (int i = 0; i <= m; i++)
        for (int j = 0; j <= n; j++) {
            if (i == 0 || j == 0)
                dp[i][j] = 0;
            else if (strcmp(old_lines[i-1], new_lines[j-1]) == 0)
                dp[i][j] = dp[i-1][j-1] + 1;
            else
                dp[i][j] = dp[i-1][j] > dp[i][j-1] ? dp[i-1][j] : dp[i][j-1];
        }
 
    // traceback (stored reversed)
    char temp_lines[256][512];
    char temp_prefix[256];
    int count = 0;
    int i = m, j = n;
 
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && strcmp(old_lines[i-1], new_lines[j-1]) == 0) {
            temp_prefix[count] = ' ';
            strncpy(temp_lines[count++], old_lines[i-1], 511);
            i--; j--;
        } else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) {
            temp_prefix[count] = '+';
            strncpy(temp_lines[count++], new_lines[j-1], 511);
            j--;
        } else {
            temp_prefix[count] = '-';
            strncpy(temp_lines[count++], old_lines[i-1], 511);
            i--;
        }
    }
 
    // build result string
    size_t result_sz = (count + 1) * 520;
    char *result = malloc(result_sz);
    if (!result) { free(old_copy); free(new_copy); return NULL; }
    result[0] = '\0';
 
    for (int k = count - 1; k >= 0; k--) {
        char line[520];
        snprintf(line, sizeof(line), "%c %s\n", temp_prefix[k], temp_lines[k]);
        strncat(result, line, result_sz - strlen(result) - 1);
    }
 
    free(old_copy);
    free(new_copy);
    return result; // caller must free
}



char *generateChanges(struct commit *commit){
    if (commit == NULL) return NULL;
    char *change = NULL;
    if (commit->parent != NULL){

        struct tree *tree_temp = commit->commit_tree;
        struct commit *commit_temp = commit->parent;
        for(int i=0; i< commit->commit_tree->node_count; i++){
            
            if (strcmp(tree_temp->nodes[i]->context, commit_temp->commit_tree->nodes[i]->context)!=0
                && strcmp(tree_temp->nodes[i]->file_name,commit_temp->commit_tree->nodes[i]->file_name)==0){
                    
                    change = diff(tree_temp->nodes[i]->context,commit_temp->commit_tree->nodes[i]->context);
                }
                
        }
    }
    return change;
}

struct commit *findCommitsFromNodeHash(struct commit *head,char *nodeHash){
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
    strcpy(new_node->file_name, filename);
    new_node->changes = NULL;
    hash_node(new_node);

    if(nodeType == FILE_NODE){
        struct commit *parentOfNode = findCommitsFromNodeHash(NULL,new_node->hash);
        
        
        if (parentOfNode != NULL){
            char *changes = generateChanges(parentOfNode);
            new_node->changes = changes;
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


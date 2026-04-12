#include <include/tree.h>
#include <include/commit.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <paths.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
//helpers

int split_lines(char *text, char **lines, int max_lines) {
    int count = 0;
    char *line = strtok(text, "\n");
    while (line != NULL && count < max_lines) {
        lines[count++] = line;
        line = strtok(NULL, "\n");
    }
    return count;
}

char *diff(char *old, char *new) {
    char old_copy[4096], new_copy[4096];
    strncpy(old_copy, old, sizeof(old_copy));
    strncpy(new_copy, new, sizeof(new_copy));

    char *old_lines[256], *new_lines[256];
    int m = split_lines(old_copy, old_lines, 256);
    int n = split_lines(new_copy, new_lines, 256);

    // build LCS table on lines
    int dp[m+1][n+1];
    for (int i = 0; i <= m; i++) {
        for (int j = 0; j <= n; j++) {
            if (i == 0 || j == 0)
                dp[i][j] = 0;
            else if (strcmp(old_lines[i-1], new_lines[j-1]) == 0)
                dp[i][j] = dp[i-1][j-1] + 1;
            else
                dp[i][j] = dp[i-1][j] > dp[i][j-1] ? dp[i-1][j] : dp[i][j-1];
        }
    }

    // traceback
    char *result = malloc(8192);
    result[0] = '\0';

    int i = m, j = n;
    char temp_lines[256][512];  // store in reverse
    char temp_prefix[256];
    int count = 0;

    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && strcmp(old_lines[i-1], new_lines[j-1]) == 0) {
            // same line — no prefix
            temp_prefix[count] = ' ';
            strncpy(temp_lines[count++], old_lines[i-1], 511);
            i--; j--;
        } else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) {
            // added in new
            temp_prefix[count] = '+';
            strncpy(temp_lines[count++], new_lines[j-1], 511);
            j--;
        } else {
            // removed from old
            temp_prefix[count] = '-';
            strncpy(temp_lines[count++], old_lines[i-1], 511);
            i--;
        }
    }

    // reverse and build result string
    for (int k = count - 1; k >= 0; k--) {
        char line[520];
        snprintf(line, sizeof(line), "%c %s\n", temp_prefix[k], temp_lines[k]);
        strncat(result, line, 8192 - strlen(result) - 1);
    }

    return result;  // caller must free
}


char *generateChanges(struct commit *commit){
    char *change;
    if (commit->parent != NULL){

        struct tree tree_temp = commit->commit_tree;
        struct commit *commit_temp = commit->parent;

        

        for(int i=0; i<commit->commit_tree.node_count; i++){
            
            if (strcmp(tree_temp.nodes[i]->context, commit_temp->commit_tree.nodes[i]->context)!=0
                && strcmp(tree_temp.nodes[i]->file_name,commit_temp->commit_tree.nodes[i]->file_name)==0){
                    
                    change = diff(tree_temp.nodes[i]->context,commit_temp->commit_tree.nodes[i]->context);
                }
                
        }
        return change;
    }
    change = NULL;
    return change;
}

struct commit findCOmmitsFromNodeHash(char nodeHash){
    struct commit *commit;
    int commitTreeNodeCount = 0;
    bool found = false;
    while(commit->parent!=NULL){
        commitTreeNodeCount = commit->commit_tree.node_count;
        
        for(int i=0; i<commitTreeNodeCount; i++){
            if(strcmp(commit->commit_tree.nodes[i]->hash, nodeHash)==0) found = true;
        }

        if(found) return *commit;
        else{
            commit = commit->parent;
        }
    }

}


struct node *CreateNode(char *context, char *filename,  enum NodeType nodeType){
    //if folder we only care ab name and creation timestamp
    struct node *new_node = malloc(sizeof(struct node));
    new_node->created_at = time(NULL);
    strcpy(new_node->file_name, filename);

    if(nodeType == FILE_NODE){
        struct commit parentOfNode = findCOmmitsFromNodeHash(new_node->hash);
        char *changes = generateChanges(&parentOfNode);
        
        *new_node= (struct node){
            .changes = changes,
            .nodeType = FILE_NODE
        };

        strcpy(new_node->changes, changes);
    }
    new_node->nodeType = FOLDER;
    hash_node(new_node);
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
            hash_node(node);
            tree->nodes[tree->node_count++] = node;
        }
    }

    closedir(dir);
    return tree;
}
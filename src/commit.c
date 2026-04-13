#include "include/commit.h"
#include "include/tree.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>

struct commit *readCommit(char *path) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) return NULL;

    struct commit *c = malloc(sizeof(struct commit));
    c->header = malloc(sizeof(struct commitHeader));
    c->parent = NULL;

    fread(c->header, sizeof(struct commitHeader), 1, f);

    if (strncmp(c->header->magic, "CIT1", 4) != 0) {
        printf("incompatible commit format, ignoring: %s\n", path);
        free(c->header);
        free(c);
        fclose(f);
        return NULL;
    }

    struct tree *tree = malloc(sizeof(struct tree));
    tree->node_count = 0;
    tree->root_count = 0;
    tree->root = NULL;
    c->commit_tree = tree;

    char folder_marker[512];
    while (fread(folder_marker, sizeof(folder_marker), 1, f) == 1) {
        if (strncmp(folder_marker, "[DIR]", 5) != 0) break;

        char file_name[256];
        while (fread(file_name, sizeof(file_name), 1, f) == 1) {
            if (strncmp(file_name, "[DIR]", 5) == 0) {
                fseek(f, -(long)sizeof(file_name), SEEK_CUR);
                break;
            }

            struct node *n = malloc(sizeof(struct node));
            strncpy(n->file_name, file_name, sizeof(n->file_name));
            fread(&n->created_at, sizeof(n->created_at), 1, f);
            fread(&n->nodeType, sizeof(n->nodeType), 1, f);
            fread(n->hash, sizeof(n->hash), 1, f);

            bool is_diff;
            fread(&is_diff, sizeof(bool), 1, f);

            uint64_t ctx_len;
            fread(&ctx_len, sizeof(ctx_len), 1, f);
            if (ctx_len > 0) {
                n->context = malloc(ctx_len);
                fread(n->context, ctx_len, 1, f);
            } else {
                n->context = NULL;
            }
            n->changes = NULL;
            tree->nodes[tree->node_count++] = n;
        }
    }

    fclose(f);
    return c;
}

struct commit *loadLastCommit(char *cwd) {
    char cit_path[2048];
    snprintf(cit_path, sizeof(cit_path), "%s/.cit", cwd);
    
    DIR *dir = opendir(cit_path);
    if (dir == NULL) return NULL;

    struct dirent *entry;
    char latest[1024] = "";
    time_t latest_time = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".cit") == NULL) continue;
        
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", cit_path, entry->d_name);
        
        struct stat s;
        stat(full_path, &s);
        if (s.st_mtime > latest_time) {
            latest_time = s.st_mtime;
            strncpy(latest, full_path, sizeof(latest));
        }
    }
    closedir(dir);

    if (strlen(latest) == 0) return NULL;  
    return readCommit(latest);  
}

void writeTree(struct tree *tree, struct tree* parent_tree, FILE *f) {
    if (tree == NULL) return;

    char folder_marker[512];
    snprintf(folder_marker, sizeof(folder_marker), "[DIR]%s", tree->root);
    fwrite(folder_marker, sizeof(folder_marker), 1, f);

    for (int i = 0; i < tree->node_count; i++) {
        struct node *n = tree->nodes[i];
        if (n == NULL) continue;

        fwrite(n->file_name, sizeof(n->file_name), 1, f);
        fwrite(&n->created_at, sizeof(n->created_at), 1, f);
        fwrite(&n->nodeType, sizeof(n->nodeType), 1, f);
        fwrite(n->hash, sizeof(n->hash), 1, f);

        char *content_to_write = n->context;  
        bool is_diff = false;

        if (parent_tree != NULL) {
            for (int j = 0; j < parent_tree->node_count; j++) {
                if (strcmp(parent_tree->nodes[j]->file_name, n->file_name) == 0) {
                    // file existed before: store diff
                    content_to_write = diff(parent_tree->nodes[j]->context, n->context);
                    is_diff = true;
                    break;
                }
            }
        }
        fwrite(&is_diff, sizeof(bool), 1, f);

        uint64_t ctx_len = content_to_write ? strlen(content_to_write) + 1 : 0;
        fwrite(&ctx_len, sizeof(ctx_len), 1, f);
        if (ctx_len > 0) fwrite(content_to_write, ctx_len, 1, f);
        if (is_diff && content_to_write) free(content_to_write);
    }


    for (int i = 0; i < tree->root_count; i++) {
        struct tree *parent_sub = NULL;
        // find matching subtree in parent
        if (parent_tree != NULL) {
            for (int j = 0; j < parent_tree->root_count; j++) {
                if (strcmp(parent_tree->sub_trees[j]->root, tree->sub_trees[i]->root) == 0) {
                    parent_sub = parent_tree->sub_trees[j];
                    break;
                }
            }
        }
        writeTree(tree->sub_trees[i], parent_sub, f);
    }
}

void writeCommit(struct commit *commit, char *cwd){
    if (commit == NULL || commit->header == NULL) return;

    char path[2048];
    snprintf(path, sizeof(path), "%s/.cit/%s_%ld.cit", cwd, commit->header->commit_message, (long)commit->header->committed_at);
    FILE *f = fopen(path, "wb");
    if (f == NULL) { perror("fopen failed"); return; }
    struct tree *parent_tree = commit->parent ? commit->parent->commit_tree : NULL;

    fwrite(commit->header, sizeof(struct commitHeader), 1,f);

    writeTree(commit->commit_tree, parent_tree,f);
    fclose(f);
}

struct commit *Commit(struct tree *commit_tree, struct commit *parent, char *commit_message, char *filepath) {
    struct commit *new_commit = malloc(sizeof(struct commit));
    new_commit->header = malloc(sizeof(struct commitHeader)); 
    strncpy(new_commit->header->magic, "CIT1", 4);
    new_commit->commit_tree = commit_tree;
    new_commit->header->committed_at = time(NULL);
    strcpy(new_commit->header->commit_message, commit_message);
    strcpy(new_commit->header->filename, filepath);

    new_commit->parent = parent;  

    writeCommit(new_commit, filepath);
    return new_commit;
}
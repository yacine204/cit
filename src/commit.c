#include "include/commit.h"
#include "include/tree.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>


struct commit *loadLastCommit(char *cwd) {
    char dir_path[2048];
    snprintf(dir_path, sizeof(dir_path), "%s/.cit", cwd);

    DIR *dir = opendir(dir_path);
    if (dir == NULL) return NULL;

    struct dirent *entry;
    long latest_ts = 0;
    char latest_name[512] = {0};

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char *underscore = strrchr(entry->d_name, '_');
        if (underscore == NULL) continue;
        long ts = atol(underscore + 1);
        if (ts > latest_ts) {
            latest_ts = ts;
            strncpy(latest_name, entry->d_name, sizeof(latest_name) - 1);
        }
    }
    closedir(dir);

    if (latest_ts == 0) return NULL;

    char file_path[2048];
    snprintf(file_path, sizeof(file_path), "%s/.cit/%s", cwd, latest_name);
    return read_from_binary(file_path);
}

void writeTree(struct tree *tree, FILE *f) {
    for(int i=0; i<tree->node_count; i++){
        struct node *n = tree->nodes[i];
        fprintf(f, "##node %s\n", n->nodeHeader->fileName);
        fprintf(f, "##changes\n%s\n", n->changes ? n->changes  : "");
        fprintf(f, "##context\n%s\n", n->context ? n->context : "");
    }
    for(int i=0;i<tree->root_count; i++){
        writeTree(tree->sub_trees[i],f);
    }
}

int count_all_nodes(struct tree *tree) {
    int count = tree->node_count;
    for (int i = 0; i < tree->root_count; i++)
        count += count_all_nodes(tree->sub_trees[i]);
    return count;
}


void writeCommit(struct commit *commit, char *cwd){
   if (commit == NULL) return;
   char path[2048];

   snprintf(path, sizeof(path), "%s/.cit/%s_%ld.cit",
        cwd,
        commit->commit_message,
        commit->created_at);
    
    FILE *f = fopen(path, "w");
    if (f == NULL) { perror("writeCommit fopen failed"); return; }

    int total_nodes = count_all_nodes(commit->commit_tree);

    fprintf(f, "##commit %ld %d %d\n",
        commit->created_at,
        commit->commit_tree->root_count,
        total_nodes);

    writeTree(commit->commit_tree, f);
    fclose(f);
}

struct commit *Commit(struct tree *commit_tree, struct commit *parent, char *commit_message, char *cwd) {
    struct commit *new_commit = malloc(sizeof(struct commit));
    memset(new_commit, 0, sizeof(struct commit));

    new_commit->commit_tree = commit_tree;
    new_commit->parent = parent;
    new_commit->created_at = time(NULL);
    new_commit->commit_message = strdup(commit_message ? commit_message : "");
    if (new_commit->commit_message == NULL) {
        free(new_commit);
        return NULL;
    }

    writeCommit(new_commit, cwd);
    return new_commit;
}
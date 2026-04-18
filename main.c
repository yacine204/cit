#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "include/commit.h"
#include "include/tree.h"

#define CIT "/.cit"

struct commit *read_from_binary(char *file_path){
    struct commit *commit = malloc(sizeof(struct commit));
    commit->commit_tree = malloc(sizeof(struct tree));
    commit->commit_tree->nodes = malloc(sizeof(struct node*)*256);
    
    FILE *f = fopen(file_path, "r");
    if(f==NULL){
        perror("couldnt open file");
        free(commit);
        return NULL;
    }

    fscanf(f, "##commit %ld %ld %ld\n", &commit->created_at, &commit->commit_tree->root_count, &commit->commit_tree->node_count);
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
        while((pos=ftell(f)), fgets(line, sizeof(line),f)){
            if(strncmp(line, "##",2)==0){fseek(f,pos,SEEK_SET);break;}
            changes_size += strlen(line);
        }

        temp->changes = malloc(changes_size+1);
        temp->changes[0] = '\0';
        fseek(f, changes_start, SEEK_SET);
        while((pos=ftell(f)),fgets(line, sizeof(line),f)) {
            if(strncmp(line, "##",2)==0){ fseek(f,pos, SEEK_SET);break;}
            strcat(temp->changes, line);
        }

        fscanf(f, " ##context\n");
        size_t context_size = 0;

        long context_start = 0;
        context_start = ftell(f);
        while ((pos = ftell(f)), fgets(line, sizeof(line), f)) {
            if (strncmp(line, "##", 2) == 0) { fseek(f, pos, SEEK_SET); break; }
            context_size += strlen(line);
        }

        temp->context = malloc(context_size + 1);
        temp->context[0] = '\0';
        fseek(f, context_start, SEEK_SET);
        while ((pos = ftell(f)), fgets(line, sizeof(line), f)) {
            if (strncmp(line, "##", 2) == 0) { fseek(f, pos, SEEK_SET); break; }
            strcat(temp->context, line);
        }

        commit->commit_tree->nodes[i] = temp;        
    }
    fclose(f);
    return commit;
}

int main(int argc, char *argv[]){
    struct commit *commit = read_from_binary(".cit/test_commit.cit");
    printf("node 1:\nchanges:\n%s \ncontext:\n%s", commit->commit_tree->nodes[0]->changes, commit->commit_tree->nodes[0]->context);
    return 0;

    //takes current directory folders
    //create a .cit folder
    //store the commits there

    //commit command example: ./main . commit<"hi lol">
    // if(argc!=3){
    //     perror("usage: ./build <dir> commit<message>");
    //     return 1;
    // }

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    DIR *dir = opendir(cwd);
    struct dirent *entry;

    bool init = false;

    while((entry = readdir(dir))!=NULL){
        // printf("%s ", entry->d_name);
        if(strcmp(".cit", entry->d_name)==0 && entry->d_type == DT_DIR){
            init = true;
            break;
        }
    }

    closedir(dir);
    if(!init){
        //if not initialized create the dir
        char citDir[sizeof(cwd) + sizeof(CIT)];
        snprintf(citDir, sizeof(citDir), "%s%s", cwd, CIT);

        if(mkdir(citDir, 0755) == -1){
            perror("mkdir failed");
            return 1;
        }
        time_t now = time(NULL);
        printf("cit initialized at -%s\n", ctime(&now));
        return 0;
    }
    // struct tree *tree = CreateTree(cwd);
    // struct commit *last = loadLastCommit(cwd);
    // struct commit *commit = Commit(tree, last, argv[2], cwd);
    
    
    return 0;
}

// example
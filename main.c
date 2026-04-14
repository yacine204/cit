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


int main(int argc, char *argv[]){
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
    struct tree *tree = CreateTree(cwd);
    struct commit *last = loadLastCommit(cwd);
    printf("last commit: %s\n", last ? last->header->commit_message : "NULL");
    struct commit *commit = Commit(tree, last, argv[2], cwd);
    
    
    return 0;
}

// example
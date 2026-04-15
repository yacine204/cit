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


char *generateChanges(char *old, char*new){
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


int main(int argc, char *argv[]){

    char old_buffer[] = "line one\nline two\nline three\n";
    char new_buffer[] = "hi lol\n";
    // char count = countLines(buffer);
    // char **final = split_lines(buffer);
    // for(int i=0; i<count; i++) printf("%s\n",final[i]);
    char *changes = generateChanges(old_buffer, new_buffer);
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
    struct tree *tree = CreateTree(cwd);
    struct commit *last = loadLastCommit(cwd);
    struct commit *commit = Commit(tree, last, argv[2], cwd);
    
    
    return 0;
}

// example
#include <include/commit.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

//TODO: use sha/ssl

void hash_node(struct node *node) {
    //node:<timestamp><filename>
    snprintf(node->hash, sizeof(node->hash), "<node>:<%ld><%s>",node->created_at, node->file_name);

}
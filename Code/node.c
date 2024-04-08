#include "node.h"

TreeNode* root;
int syn_error;
extern int yylineno;

void initNode(){
    root = NULL;
    syn_error = 0;
    yylineno = 1;
}

TreeNode* addNode(char* newtype, char* newinfo) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));

    int typelen = strlen(newtype);
    node->type = (char*)malloc(sizeof(char)*typelen+1);
    strcpy(node->type, newtype);

    if (newinfo != NULL){
        int infolen = strlen(newinfo);
        node->info = (char*)malloc(sizeof(char)*infolen+1);
        strcpy(node->info, newinfo);
    }
    else node->info = NULL;

    node->lineno = yylineno;
    node->child = node->next = NULL;

    // printf("%d %s %s\n", node->lineno, node->type, node->info);
    return node;
}

void addChild(TreeNode* parent, int childnum, ...){
    va_list valist;
    va_start(valist, childnum);
    TreeNode* now = va_arg(valist, TreeNode*);
    parent->child = now;
    if (now != NULL) {
        parent->lineno = now->lineno;
        for (int i = 0; i < childnum-1; ++i){
            now->next = va_arg(valist, TreeNode*);
            if (now->next != NULL) now = now->next; // skip those reduced by empty string
        }
    }
    va_end(valist);
}

void printTree(TreeNode* node, int space){
    if (node == NULL) return;
    for (int i = 0; i < space; ++i) printf(" ");
    if (node->child != NULL) {
        printf("%s (%d)\n", node->type, node->lineno);
        TreeNode* now = node->child;
        while (now != NULL){
            printTree(now, space+2);
            now = now->next;
        }
    }
    else {
        if (strcmp(node->type, "INT") == 0)
            printf("%s: %lld\n", node->type, strtoll(node->info, NULL, 0));
        else if (strcmp(node->type, "FLOAT") == 0)
            printf("%s: %f\n", node->type, (float)atof(node->info));
        else if (strcmp(node->type, "ID") == 0 || strcmp(node->type, "TYPE") == 0)
            printf("%s: %s\n", node->type, node->info);
        else if (strcmp(node->type, "Program") == 0) // reduced by empty string
            printf("Program (%d)\n", node->lineno);
        else
            printf("%s\n", node->type);
    }
}
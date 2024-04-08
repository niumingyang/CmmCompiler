#ifndef SYNTAX_NODE
#define SYNTAX_NODE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef struct TreeNode {
    int lineno;
    char* type;
    char* info;
    struct TreeNode* child;
    struct TreeNode* next;
} TreeNode;

extern TreeNode* root;
extern int syn_error;

void initNode();
TreeNode* addNode(char* newtype, char* newinfo);
void addChild(TreeNode* parent, int childnum, ...);
void printTree(TreeNode* node, int space);

#endif
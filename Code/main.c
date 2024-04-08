#include <stdio.h>
#include "node.h"
#include "semantic.h"
#include "intercode.h"
#include "objectcode.h"

// #define SEMANTIC_DEBUG

extern void yyrestart(FILE *);
extern int yyparse();

void errorUsage() {
    puts("Format: ./parser ${input} ${output} [-ir ${intercode}] [-p]");
    puts("${input} \tcmmcode.cmm");
    puts("${output}\tobjectcode.s");
    puts("-ir      \tgenerate inter-mediate code");
    puts("-p       \tprint syntactic tree");
    exit(1);
}

int main(int argc, char** argv) {
    if (argc <= 2) errorUsage();
    FILE* in_f = fopen(argv[1], "r");
    if (!in_f) {
        perror(argv[1]);
        return 1;
    }
    int printSTree = 0;
    int printInter = 0;
    char* interFile = NULL;
    int tmp = 3;
    while (tmp < argc) {
        if (strcmp(argv[tmp], "-p") == 0) printSTree = 1;
        else if (strcmp(argv[tmp], "-ir") == 0) {
            printInter = 1;
            if (tmp+1 < argc && argv[tmp+1][0] != '-') {
                tmp++;
                interFile = argv[tmp];
            }
        }
        else errorUsage();
        tmp++;
    }
    
    /* lexical and syntactic analysis */
    yyrestart(in_f);
    initNode();
    yyparse();
    if (syn_error != 0) 
        return 0;

    /* print syntactic tree */
    if (printSTree)
        printTree(root, 0);

    /* semantic check and generate inter-mediate code */
    initSymbolTable();
    semanticProcess(root);
    if (sema_error != 0) 
        return 0;

    /* optimize inter-mediate code */
    optimizeCode();

    /* print inter-mediate code */
    if (printInter)
        printCode(interFile);

    /* generate and print object code */
    initObjectCode();
    translateCode(argv[2]);

    return 0;
}
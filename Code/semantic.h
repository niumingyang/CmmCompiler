#ifndef SEMANTIC_INFO
#define SEMANTIC_INFO

#include <stdio.h>
#include <string.h>
#include "node.h"
#include "intercode.h"

// this macro will print symbol table after every change
// #define SEMANTIC_DEBUG

#define HASH_SIZE 0x3fff
#define MAX_DEPTH 100

typedef struct Type_* Type;
typedef struct FieldList_* FieldList;
typedef struct BlockSymbol_* BlockPtr;

/* all types and their representation in struct Type_
 *    type                kind          u
 * int | float          BASIC       u.basic
 * array                ARRAY       u.array
 * struct               STRUCTURE   u.structure(only save the struct name in it, u.structure->type = NULL)
 * anonymous struct     STRUCTURE   u.structure(save the definition of structure in it)
 * struct definition    STRUCTDEF   u.structure(save the definition of structure in it)
 * function             FUNCTION    u.function
*/
struct Type_ {
    enum { BASIC, ARRAY, STRUCTURE, FUNCTION, STRUCTDEF } kind;
    union {
        enum { BASIC_INT, BASIC_FLOAT } basic;

        struct { 
            Type array_type; 
            int size; 
        } array;

        FieldList structure;

        struct {
            Type ret_type;
            FieldList param_info;
            int param_num;
            int hasdef; // if has definition hasdef = 0, else hasdef = lineno
        } function;
    } u;
    int size;// the size of this type
    // size = 0 iff the varible is a param of function
    // size is undefined iff the varible is a function
};

struct FieldList_ {
    char* name;
    Type type;
    FieldList next;
};

struct BlockSymbol_ {
    FieldList member;
    BlockPtr next;
};

extern int sema_error;

void initSymbolTable();
int addSymbol(FieldList symb, int dep);
FieldList findSymbol(char* name, int isfunc);
FieldList findSymbolInOneDep(char* name, int isfunc, int dep);
void delSymbol(int dep);
void semanticProcess();
int typeEqual(Type type1, Type type2);
void printType(Type type);
int sizeofType(Type type);

FieldList VarDecProcess(TreeNode* node, Type _type);
FieldList DefListProcess(TreeNode* node, int isstruct);
Type SpecifierProcess(TreeNode* node);
void ExtDefProcess(TreeNode* node);
void CompStProcess(TreeNode* node, Type func_ret_type);
void StmtProcess(TreeNode* node, Type func_ret_type);
Type ExpProcess(TreeNode* node, Operand place);
Type CondProcess(TreeNode* node, int label_true, int label_false);
void arrayAssign(Type type1, Type type2, Operand op1, Operand op2);
void structAssign(Type type1, Type type2, Operand op1, Operand op2);

#ifdef SEMANTIC_DEBUG
void printFieldList(FieldList f);
void printSymbolTable();
#endif

#endif
#ifndef INTERCODE_INFO
#define INTERCODE_INFO

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

// this macro will print IR after every addition
// #define INTERCODE_DEBUG

// this macro will print every improved IR
// #define OPTIMIZE_IR_DEBUG

// #define BASICBLOCK_DEBUG

typedef struct Operand_* Operand;
struct Operand_ {
    enum { VAR_OP, CONST_OP, SIZE_OP, ADDR_OP, TEMP_ADDR_OP, 
        LABEL_OP, TEMP_OP, OPERATOR_OP, FUNC_OP, NULL_OP } kind;
    union {
        int value;
        char name[64]; // func name
    } u;
};

typedef struct InterCode_* InterCode;
struct InterCode_ {
    enum { LABEL_IC, FUNCTION_IC, ASSIGN_IC, PLUS_IC, MINUS_IC, STAR_IC, DIV_IC, GET_ADDR_IC, GET_VALUE_IC,
        TO_MEM_IC, GOTO_IC, IF_GOTO_IC, RETURN_IC, DEC_IC, ARG_IC, CALL_IC, PARAM_IC, READ_IC, WRITE_IC,
        /* other Inter-mediate code (not included in handout) */
        MEM_TO_MEM_IC, } kind;
    union {
        struct { Operand op1; } uniop;
        struct { Operand op1, op2; } binop;
        struct { Operand op1, op2, op3; } triop;
        struct { Operand op1, op2, op3, op4; } ifgotoop;
    } u;
};
/*   IR                   InterCode->kind
 * LABEL x :                LABEL_IC
 * FUNCTION f :             FUNCTION_IC
 * x := y                   ASSIGN_IC
 * x := y + z               PLUS_IC
 * x := y - z               MINUS_IC
 * x := y * z               STAR_IC
 * x := y / z               DIV_IC
 * x := &y                  GET_ADDR_IC
 * x := *y                  GET_VALUE_IC
 * *x := y                  TO_MEM_IC
 * GOTO x                   GOTO_IC
 * IF x [relop] y GOTO z    IF_GOTO_IC
 * RETURN x                 RETURN_IC
 * DEC x [size]             DEC_IC
 * ARG x                    ARG_IC
 * x := CALL f              CALL_IC
 * PARAM x                  PARAM_IC
 * READ x                   READ_IC
 * WRITE x                  WRITE_IC
 * 
 *   other IR not included in handout
 * *x = *y                  MEM_TO_MEM_IC
*/

typedef struct BasicBlock_* BasicBlock;
struct BasicBlock_{
    int begin;
    int end;
    int visit;
    int scanned;
    Operand first;
    struct BasicBlock_* next;
};

extern BasicBlock block_head;
extern InterCode* codelist;
extern int temp_num;
extern int label_num;
extern int var_num;
extern char all_var[100000][64];
extern int code_cap;
extern int code_len;

void initIntercode();

int new_temp();
int new_label();
int get_varno(char* name);

Operand addOperand(int kind, int opval);
Operand addFuncOperand(char* name);
Operand copyOperand(Operand op);
int operandEqual(Operand op1, Operand op2);
void addIntercode(int kind, int opnum, ...);
void delIntercode(int lineno);
void insertIntercode(int loc, int kind, int opnum, ...);
void replaceIntercode(int loc, int kind, int opnum, ...);

void optimizeCode();
int constantFolding();
int peepholeOptimize();
void removeDeadCode();
int replaceOperand(InterCode code, Operand op, Operand new_op, int* changed);
int containOperand(InterCode code, Operand op);
int visitBlockbyLine(int line);
int visitBlockbyLabel(Operand op);
void addBlock(int begin, int end, Operand first);
void partitionBlock();

char* getOperand(Operand op);
char* getCode(InterCode code);
void printCode(char* filename);

#endif
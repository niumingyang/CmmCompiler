#ifndef OBJECTCODE_INFO
#define OBJECTCODE_INFO

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "intercode.h"

// #define OBJECTCODE_DEBUG

#define REG_NUM 32
#define CAN_USE 19

typedef struct Varible* Varptr;
struct Varible{
    Operand var;
    int reg; // saved in which reg (-1 if not in reg)
    int offset; // saved in offset($fp) (-1 if not in stack)
};

typedef struct Register* Regptr;
struct Register{
    char name[10];
    int var; // save varlist[var] (-1 if free)
};

void initObjectCode();
int getVar(Operand op, FILE* fp);
int ensureReg(int varno, int use_val, FILE* fp);
int allocateReg(int varno, FILE* fp);
void allocateAllVar(int lineno, FILE* fp);
void spillVar(int varno, FILE* fp);
void spillAllVar(FILE* fp);
int getReg(Operand op, int use_val, FILE* fp);
void collectReg(int regno, int lineno, FILE* fp);
void addParam(Operand op);
void delAllParam();
int isParam(Operand op);
void translateIR(int lineno, FILE* fp);
void translateCode(char* filename);

#endif
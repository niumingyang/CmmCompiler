#include "intercode.h"

BasicBlock block_head;
InterCode* codelist;
int temp_num;
int label_num;
int var_num;
char all_var[100000][64];
int code_cap;
int code_len;

void initIntercode(){
    temp_num = 0;
    label_num = 0;
    var_num = 0;

	code_cap = 10000;
	code_len = 0;
    codelist = (InterCode*)malloc(code_cap * sizeof(InterCode));
    block_head = NULL;
}

int new_temp(){
    return temp_num++;
}

int new_label(){
    return label_num++;
}

int get_varno(char* name){
    if (name == NULL) return -1;
    for (int i = 0; i < var_num; ++i)
        if (strcmp(all_var[i], name) == 0)
            return i;
    strcpy(all_var[var_num], name);
    return var_num++;
}

Operand addOperand(int kind, int opval){
    Operand ret = (Operand)malloc(sizeof(struct Operand_));
    ret->kind = kind;
    ret->u.value = opval;
    return ret;
}

Operand addFuncOperand(char* name){
    if (name == NULL) return NULL;
    Operand ret = (Operand)malloc(sizeof(struct Operand_));
    ret->kind = FUNC_OP;
    strcpy(ret->u.name, name);
    return ret;
}

Operand copyOperand(Operand op){
    if (op == NULL) return NULL;
    Operand ret = (Operand)malloc(sizeof(struct Operand_));
    ret->kind = op->kind;
    if (ret->kind == FUNC_OP)
        strcpy(ret->u.name, op->u.name);
    else ret->u.value = op->u.value;
    return ret;
}
int operandEqual(Operand op1, Operand op2){
    if (op1 == op2)
        return 1;
    if (op1==NULL || op2==NULL)
        return 0;
    if (op1->kind != op2->kind)
        return 0;
    if (op1->kind != FUNC_OP)
        return op1->u.value == op2->u.value;
    else return strcmp(op1->u.name, op2->u.name) == 0;
}

void addIntercode(int kind, int opnum, ...){
    va_list valist;
    va_start(valist, opnum);
    Operand op[4];
    for (int i = 0; i < opnum; ++i){
        op[i] = va_arg(valist, Operand);
        if ((op[i]==NULL || op[i]->kind==NULL_OP) && kind!=CALL_IC){
            va_end(valist);
            return;
        }
    }
    va_end(valist);
    InterCode new_code = (InterCode)malloc(sizeof(struct InterCode_));
    new_code->kind = kind;
    switch (opnum){
        case 1: new_code->u.uniop.op1 = op[0];
                break;
        case 2: new_code->u.binop.op1 = op[0];
                new_code->u.binop.op2 = op[1];
                break;
        case 3: new_code->u.triop.op1 = op[0];
                new_code->u.triop.op2 = op[1];
                new_code->u.triop.op3 = op[2];
                break;
        case 4: new_code->u.ifgotoop.op1 = op[0];
                new_code->u.ifgotoop.op2 = op[1];
                new_code->u.ifgotoop.op3 = op[2];
                new_code->u.ifgotoop.op4 = op[3];
                break;
    }

    if (code_len >= code_cap) {
		code_cap *= 2;
		codelist = (InterCode*)realloc(codelist, sizeof(InterCode)*code_cap);
	}
	codelist[code_len++] = new_code;

    #ifdef INTERCODE_DEBUG
    printf("\033[31madd new:\033[0m Line %d: %s\n", code_len, getCode(new_code));
    #endif
}

void replaceIntercode(int loc, int kind, int opnum, ...){
    va_list valist;
    va_start(valist, opnum);
    Operand op[4];
    for (int i = 0; i < opnum; ++i){
        op[i] = va_arg(valist, Operand);
        if ((op[i]==NULL || op[i]->kind==NULL_OP) && kind!=CALL_IC){
            va_end(valist);
            return;
        }
    }
    va_end(valist);
    InterCode new_code = (InterCode)malloc(sizeof(struct InterCode_));
    new_code->kind = kind;
    switch (opnum){
        case 1: new_code->u.uniop.op1 = op[0];
                break;
        case 2: new_code->u.binop.op1 = op[0];
                new_code->u.binop.op2 = op[1];
                break;
        case 3: new_code->u.triop.op1 = op[0];
                new_code->u.triop.op2 = op[1];
                new_code->u.triop.op3 = op[2];
                break;
        case 4: new_code->u.ifgotoop.op1 = op[0];
                new_code->u.ifgotoop.op2 = op[1];
                new_code->u.ifgotoop.op3 = op[2];
                new_code->u.ifgotoop.op4 = op[3];
                break;
    }

    #ifdef OPTIMIZE_IR_DEBUG
    printf("\033[33mreplace:\033[0m Line %d: %s \033[33mto\033[0m %s\n", loc, getCode(codelist[loc]), getCode(new_code));
    #endif
    free(codelist[loc]);
    codelist[loc] = new_code;
}

void delIntercode(int lineno){
    #ifdef OPTIMIZE_IR_DEBUG
    printf("\033[32mdelete:\033[0m Line %d: %s\n", lineno, getCode(codelist[lineno]));
    #endif
    InterCode code = codelist[lineno];
    if (code != NULL){
        switch (code->kind){
            case LABEL_IC:
            case FUNCTION_IC:
            case GOTO_IC:
            case ARG_IC:
            case RETURN_IC:
            case PARAM_IC:
            case READ_IC:
            case WRITE_IC:
                free(code->u.uniop.op1);
                break;
            case ASSIGN_IC:
            case GET_ADDR_IC:
            case GET_VALUE_IC:
            case TO_MEM_IC:
            case MEM_TO_MEM_IC:
            case DEC_IC:
            case CALL_IC:
                free(code->u.binop.op1);
                free(code->u.binop.op2);
                break;
            case PLUS_IC:
            case MINUS_IC:
            case STAR_IC:
            case DIV_IC:
                free(code->u.triop.op1);
                free(code->u.triop.op2);
                free(code->u.triop.op3);
                break;
            case IF_GOTO_IC:
                free(code->u.ifgotoop.op1);
                free(code->u.ifgotoop.op2);
                free(code->u.ifgotoop.op3);
                free(code->u.ifgotoop.op4);
                break;
        }
        free(code);
    }
    for (int i = lineno; i < code_len-1; ++i)
        codelist[i] = codelist[i+1];
    code_len--;
}

// insert IC in codelist[i-1] and codelist[i]
void insertIntercode(int loc, int kind, int opnum, ...){
    va_list valist;
    va_start(valist, opnum);
    Operand op[4];
    for (int i = 0; i < opnum; ++i){
        op[i] = va_arg(valist, Operand);
        if ((op[i]==NULL || op[i]->kind==NULL_OP) && kind!=CALL_IC){
            va_end(valist);
            return;
        }
    }
    va_end(valist);
    InterCode new_code = (InterCode)malloc(sizeof(struct InterCode_));
    new_code->kind = kind;
    switch (opnum){
        case 1: new_code->u.uniop.op1 = op[0];
                break;
        case 2: new_code->u.binop.op1 = op[0];
                new_code->u.binop.op2 = op[1];
                break;
        case 3: new_code->u.triop.op1 = op[0];
                new_code->u.triop.op2 = op[1];
                new_code->u.triop.op3 = op[2];
                break;
        case 4: new_code->u.ifgotoop.op1 = op[0];
                new_code->u.ifgotoop.op2 = op[1];
                new_code->u.ifgotoop.op3 = op[2];
                new_code->u.ifgotoop.op4 = op[3];
                break;
    }

    #ifdef OPTIMIZE_IR_DEBUG
    printf("\033[33minsert:\033[0m Line %d: %s\n", loc, getCode(new_code));
    #endif
    if (code_len >= code_cap) {
		code_cap *= 2;
		codelist = (InterCode*)realloc(codelist, sizeof(InterCode)*code_cap);
	}
    for (int i = code_len; i > loc; --i)
        codelist[i] = codelist[i-1];
    codelist[loc] = new_code;
    code_len++;
}

void optimizeCode(){
    #ifdef OPTIMIZE_IR_DEBUG
    printCode("../Res/raw.ir");
    #endif
    int changed = 1;
    while (changed == 1){
        changed = 0;
        changed = constantFolding();
        changed = peepholeOptimize();
    }
    removeDeadCode();
}

int constantFolding(){
    int ret = 0;
    int changed = 1;
    while (changed == 1){
        changed = 0;
        // t1 := #9   --> replace all t1 to #9
        // t1 := v1   --> replace all t1 to v1 (v1 must not be changed)
        for (int i = 0; i < code_len; ++i){
            if ((codelist[i]->kind==ASSIGN_IC && codelist[i]->u.binop.op2->kind==CONST_OP) || 
            (codelist[i]->kind==ASSIGN_IC && codelist[i]->u.binop.op1->kind==TEMP_OP && codelist[i]->u.binop.op2->kind==VAR_OP)){
                Operand replaced = codelist[i]->u.binop.op1;
                Operand new_op = codelist[i]->u.binop.op2;
                for (int j = i+1; j < code_len; ++j)
                    if (replaceOperand(codelist[j], replaced, copyOperand(new_op), &changed))
                        break;
            }
        }
        // t1 := #1 + #2            --> t1 := #3   (or + -> -,*,/)
        // IF #1 < #2 GOTO label1   --> GOTO label1
        // IF #1 > #2 GOTO label1   --> null
        /* caution:
         * in Lab3 (Lab4 not included)
         * irsim is written in python, so v1 := #-1 / #2 should be v1 := #-1
         * (roundding to minus infinity), which is different in C
         */
        for (int i = 0; i < code_len; ++i){
            if (codelist[i]->kind==PLUS_IC && codelist[i]->u.triop.op2->kind==CONST_OP
             && codelist[i]->u.triop.op3->kind==CONST_OP){
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), 
                    addOperand(CONST_OP, codelist[i]->u.triop.op2->u.value+codelist[i]->u.triop.op3->u.value));
                changed = 1;
            }
            if (codelist[i]->kind==MINUS_IC && codelist[i]->u.triop.op2->kind==CONST_OP
             && codelist[i]->u.triop.op3->kind==CONST_OP){
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), 
                    addOperand(CONST_OP, codelist[i]->u.triop.op2->u.value-codelist[i]->u.triop.op3->u.value));
                changed = 1;
            }
            if (codelist[i]->kind==STAR_IC && codelist[i]->u.triop.op2->kind==CONST_OP
             && codelist[i]->u.triop.op3->kind==CONST_OP){
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), 
                    addOperand(CONST_OP, codelist[i]->u.triop.op2->u.value*codelist[i]->u.triop.op3->u.value));
                changed = 1;
            }
            if (codelist[i]->kind==DIV_IC && codelist[i]->u.triop.op2->kind==CONST_OP
             && codelist[i]->u.triop.op3->kind==CONST_OP){
                // Lab3
                // float res = (float)codelist[i]->u.triop.op2->u.value/(float)codelist[i]->u.triop.op3->u.value;
                // replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), 
                //     addOperand(CONST_OP, res<0?(int)res-1:(int)res));
                // changed = 1;

                // Lab4
                if (codelist[i]->u.triop.op3->u.value != 0){
                    replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), 
                        addOperand(CONST_OP, codelist[i]->u.triop.op2->u.value/codelist[i]->u.triop.op3->u.value));
                    changed = 1;
                }
            }
            if (codelist[i]->kind==IF_GOTO_IC && codelist[i]->u.ifgotoop.op1->kind==CONST_OP
             && codelist[i]->u.ifgotoop.op3->kind==CONST_OP){
                int val;
                switch(codelist[i]->u.ifgotoop.op2->u.value){
                    case 0: val = codelist[i]->u.ifgotoop.op1->u.value>codelist[i]->u.ifgotoop.op3->u.value;  break;
                    case 1: val = codelist[i]->u.ifgotoop.op1->u.value<codelist[i]->u.ifgotoop.op3->u.value;  break;
                    case 2: val = codelist[i]->u.ifgotoop.op1->u.value>=codelist[i]->u.ifgotoop.op3->u.value; break;
                    case 3: val = codelist[i]->u.ifgotoop.op1->u.value<=codelist[i]->u.ifgotoop.op3->u.value; break;
                    case 4: val = codelist[i]->u.ifgotoop.op1->u.value==codelist[i]->u.ifgotoop.op3->u.value; break;
                    case 5: val = codelist[i]->u.ifgotoop.op1->u.value!=codelist[i]->u.ifgotoop.op3->u.value; break;
                }
                if (val)
                    replaceIntercode(i, GOTO_IC, 1, copyOperand(codelist[i]->u.ifgotoop.op4));
                else delIntercode(i);
                changed = 1;
            }
        }
        // remove dead code (non-used ASSIGN_IC / LABEL_IC code)
        for (int i = 0; i < code_len; ++i){
            if (codelist[i]->kind == ASSIGN_IC){
                #ifdef OPTIMIZE_IR_DEBUG
                printf("check: Line %d: %s\n    ", i, getCode(codelist[i]));
                #endif
                int flag = 0;
                for (int j = 0; j < code_len; ++j){
                    if (j == i) continue;
                    if (containOperand(codelist[j], codelist[i]->u.binop.op1) == 2){
                        #ifdef OPTIMIZE_IR_DEBUG
                        printf("used in: Line %d: %s\n", j, getCode(codelist[j]));
                        #endif
                        flag = 1;
                        break;
                    }
                }
                if (flag == 0){
                    delIntercode(i);
                    changed = 1;
                    --i;
                }
            }
            else if (codelist[i]->kind == LABEL_IC){
                #ifdef OPTIMIZE_IR_DEBUG
                printf("check: Line %d: %s\n    ", i, getCode(codelist[i]));
                #endif
                int flag = 0;
                for (int j = 0; j < code_len; ++j){
                    if (j == i) continue;
                    if (containOperand(codelist[j], codelist[i]->u.uniop.op1) == 2){
                        #ifdef OPTIMIZE_IR_DEBUG
                        printf("used in: Line %d: %s\n", j, getCode(codelist[j]));
                        #endif
                        flag = 1;
                        break;
                    }
                }
                if (flag == 0){
                    delIntercode(i);
                    changed = 1;
                    --i;
                }
            }
        }
        if (changed == 1)
            ret = 1;
    }
    return ret;
}

int peepholeOptimize(){
    // prepare
    for (int i = 0; i < code_len; ++i)
        if (codelist[i]->kind==PLUS_IC || codelist[i]->kind==STAR_IC){
            if (codelist[i]->u.triop.op2->kind == CONST_OP){
                Operand mid;
                mid = codelist[i]->u.triop.op2;
                codelist[i]->u.triop.op2 = codelist[i]->u.triop.op3;
                codelist[i]->u.triop.op3 = mid;
            }
        }

    int ret = 0;
    int changed = 1;
    while (changed == 1){
        changed = 0;
        for (int i = 0; i < code_len; ++i){
            // t1 := CALL f   -->   v1 := call f
            // v1 := t1
            if (codelist[i]->kind==CALL_IC && i+1<code_len && codelist[i+1]->kind==ASSIGN_IC
             && operandEqual(codelist[i]->u.binop.op1, codelist[i+1]->u.binop.op2)){
                changed = 1;
                replaceIntercode(i, CALL_IC, 2, copyOperand(codelist[i+1]->u.binop.op1), copyOperand(codelist[i]->u.binop.op2));
                delIntercode(i+1);
            }
            // READ t1    -->   READ v1
            // v1 := t1
            if (codelist[i]->kind==READ_IC && i+1<code_len && codelist[i+1]->kind==ASSIGN_IC
             && operandEqual(codelist[i]->u.uniop.op1, codelist[i+1]->u.binop.op2)){
                changed = 1;
                replaceIntercode(i, READ_IC, 1, copyOperand(codelist[i+1]->u.binop.op1));
                delIntercode(i+1);
            }
            // LABEL label1   -->   LABEL label1
            // LABEL label2         replace all label2 to label1
            if (codelist[i]->kind==LABEL_IC && i+1<code_len && codelist[i+1]->kind==LABEL_IC){
                changed = 1;
                Operand replaced = codelist[i+1]->u.uniop.op1;
                Operand new_op = codelist[i]->u.uniop.op1;
                for (int j = 0; j < code_len; ++j){
                    if (codelist[j]->kind==IF_GOTO_IC && operandEqual(codelist[j]->u.ifgotoop.op4, replaced))
                        replaceIntercode(j, IF_GOTO_IC, 4, copyOperand(codelist[j]->u.ifgotoop.op1), 
                            copyOperand(codelist[j]->u.ifgotoop.op2), copyOperand(codelist[j]->u.ifgotoop.op3), 
                            copyOperand(new_op));
                    else if (codelist[j]->kind==GOTO_IC && operandEqual(codelist[j]->u.uniop.op1, replaced))
                        replaceIntercode(j, GOTO_IC, 1, copyOperand(new_op));
                }
                delIntercode(i+1);
            }
            // GOTO label1   -->   GOTO label1
            // not LABEL or FUNCTION
            if (codelist[i]->kind==GOTO_IC && i+1<code_len && codelist[i+1]->kind!=LABEL_IC && codelist[i+1]->kind!=FUNCTION_IC){
                changed = 1;
                delIntercode(i+1);
            }
            // return v0   -->   return v0
            // not LABEL or FUNCTION
            if (codelist[i]->kind==RETURN_IC && i+1<code_len && codelist[i+1]->kind!=LABEL_IC && codelist[i+1]->kind!=FUNCTION_IC){
                changed = 1;
                delIntercode(i+1);
            }
            // GOTO label1    -->   LABEL label1
            // LABEL label1
            if (codelist[i]->kind==GOTO_IC && i+1<code_len && codelist[i+1]->kind==LABEL_IC
             && operandEqual(codelist[i]->u.uniop.op1, codelist[i+1]->u.uniop.op1)){
                changed = 1;
                delIntercode(i);
            }
            // IF t1 op t2 GOTO label1   -->   IF t1 nop t2 GOTO label2
            // GOTO label2                     GOTO label1
            // LABEL label1                    LABEL label1
            if (codelist[i]->kind==IF_GOTO_IC && i+2<code_len && codelist[i+1]->kind==GOTO_IC
             && codelist[i+2]->kind == LABEL_IC && operandEqual(codelist[i]->u.ifgotoop.op4, codelist[i+2]->u.uniop.op1)){
                changed = 1;
                Operand lab = codelist[i]->u.ifgotoop.op4;
                codelist[i]->u.ifgotoop.op4 = codelist[i+1]->u.uniop.op1;
                codelist[i+1]->u.uniop.op1 = lab;
                int operator_ = codelist[i]->u.ifgotoop.op2->u.value;
                operator_ = (3-operator_<0)?(9-operator_):(3-operator_); // reverse the operator
                codelist[i]->u.ifgotoop.op2->u.value = operator_;
            }
            // t1 := t2 + #1   -->   t3 := t2 + #3   (or + -> *)
            // t3 := t1 + #2
            if (codelist[i]->kind==PLUS_IC && i+1<code_len && codelist[i+1]->kind==PLUS_IC 
             && codelist[i]->u.triop.op3->kind==CONST_OP && codelist[i+1]->u.triop.op3->kind==CONST_OP
             && operandEqual(codelist[i]->u.triop.op1, codelist[i+1]->u.triop.op2)){
                changed = 1;
                replaceIntercode(i, PLUS_IC, 3, copyOperand(codelist[i+1]->u.triop.op1),
                    copyOperand(codelist[i]->u.triop.op2), 
                    addOperand(CONST_OP, codelist[i]->u.triop.op3->u.value+codelist[i+1]->u.triop.op3->u.value));
                delIntercode(i+1);
            }
            if (codelist[i]->kind==STAR_IC && i+1<code_len && codelist[i+1]->kind==STAR_IC 
             && codelist[i]->u.triop.op3->kind==CONST_OP && codelist[i+1]->u.triop.op3->kind==CONST_OP
             && operandEqual(codelist[i]->u.triop.op1, codelist[i+1]->u.triop.op2)){
                changed = 1;
                replaceIntercode(i, STAR_IC, 3, copyOperand(codelist[i+1]->u.triop.op1),
                    copyOperand(codelist[i]->u.triop.op2), 
                    addOperand(CONST_OP, codelist[i]->u.triop.op3->u.value*codelist[i+1]->u.triop.op3->u.value));
                delIntercode(i+1);
            }
            // t1 := t2 + v1   -->   t3 := t2   (or + -> -, - -> +)
            // t3 := t1 - v1
            if (codelist[i]->kind==PLUS_IC && i+1<code_len && codelist[i+1]->kind==MINUS_IC 
             && operandEqual(codelist[i]->u.triop.op3, codelist[i+1]->u.triop.op3)
             && operandEqual(codelist[i]->u.triop.op1, codelist[i+1]->u.triop.op2)){
                changed = 1;
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i+1]->u.triop.op1),
                    copyOperand(codelist[i]->u.triop.op2));
                delIntercode(i+1);
            }
            if (codelist[i]->kind==MINUS_IC && i+1<code_len && codelist[i+1]->kind==PLUS_IC 
             && operandEqual(codelist[i]->u.triop.op3, codelist[i+1]->u.triop.op3)
             && operandEqual(codelist[i]->u.triop.op1, codelist[i+1]->u.triop.op2)){
                changed = 1;
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i+1]->u.triop.op1),
                    copyOperand(codelist[i]->u.triop.op2));
                delIntercode(i+1);
            }
            // t1 := t2 + #0   -->   t1 := t2   (or + -> -)
            if (codelist[i]->kind==PLUS_IC && codelist[i]->u.triop.op3->kind==CONST_OP 
             && codelist[i]->u.triop.op3->u.value==0){
                changed = 1;
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), copyOperand(codelist[i]->u.triop.op2));
            }
            if (codelist[i]->kind==MINUS_IC && codelist[i]->u.triop.op3->kind==CONST_OP 
             && codelist[i]->u.triop.op3->u.value==0){
                changed = 1;
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), copyOperand(codelist[i]->u.triop.op2));
            }
            // t1 := t2 * #1   -->   t1 := t2   (or * -> /)
            if (codelist[i]->kind==STAR_IC && codelist[i]->u.triop.op3->kind==CONST_OP 
             && codelist[i]->u.triop.op3->u.value==1){
                changed = 1;
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), copyOperand(codelist[i]->u.triop.op2));
            }
            if (codelist[i]->kind==DIV_IC && codelist[i]->u.triop.op3->kind==CONST_OP 
             && codelist[i]->u.triop.op3->u.value==1){
                changed = 1;
                replaceIntercode(i, ASSIGN_IC, 2, copyOperand(codelist[i]->u.triop.op1), copyOperand(codelist[i]->u.triop.op2));
            }
            // t1 := *v1
            // t2 := t1 / t2 := t1 op t3 / t2 := &t1 / WRITE t1
            // TODO

            // t1 := &v1
            // t2 := t1 / t2 := t1 op t3 / t2 := *t1
            // TODO
        }
        if (changed == 1)
            ret = 1;
    }
    return ret;
}

void removeDeadCode(){
    partitionBlock();
    int change = 1;
    BasicBlock temp;
    InterCode ic;
    while (change == 1){
        #ifdef BASICBLOCK_DEBUG
        printf("new epoch:\n");
        #endif
        change = 0;
        temp = block_head;
        while (temp != NULL){
            if (temp->scanned == 1){
                temp = temp->next;
                continue;
            }
            if (temp->visit == 1){
                temp->scanned = 1;
                #ifdef BASICBLOCK_DEBUG
                printf("  scan block %s\n", getOperand(temp->first));
                #endif
                for (int i = temp->begin; i <= temp->end; ++i){
                    ic = codelist[i];
                    if (ic->kind == CALL_IC)
                        change |= visitBlockbyLabel(ic->u.binop.op2);
                }
                if (ic->kind == GOTO_IC){
                    change |= visitBlockbyLabel(ic->u.uniop.op1);
                    temp = temp->next;
                    continue;
                }
                else if (ic->kind == IF_GOTO_IC){
                    change |= visitBlockbyLabel(ic->u.ifgotoop.op4);
                    change |= visitBlockbyLine(temp->end+1);
                }
                else if (ic->kind == RETURN_IC){
                    temp = temp->next;
                    continue;
                }
                else change |= visitBlockbyLine(temp->end+1);
            }
            temp = temp->next;
        }
    }
    #ifndef BASICBLOCK_DEBUG
    temp = block_head;
    while (temp != NULL){
        if (temp->visit == 0){
            for (int i = temp->begin; i <= temp->end; ++i){
                free(codelist[i]);
                codelist[i] = NULL;
            }
        }
        temp = temp->next;
    }
    for (int i = 0; i < code_len; ){
        if (codelist[i] == NULL)
            delIntercode(i);
        else ++i;
    }
    #endif
}

// return 1 means come across op as left value
int replaceOperand(InterCode code, Operand op, Operand new_op, int* changed){
    switch (code->kind){
        case FUNCTION_IC: 
        case LABEL_IC:
        case GOTO_IC:
        case TO_MEM_IC:
        case MEM_TO_MEM_IC:
            return 1;
        case READ_IC:
            if (operandEqual(op, code->u.uniop.op1) || operandEqual(new_op, code->u.uniop.op1))
                return 1;
        case CALL_IC:
        case GET_ADDR_IC:
        case GET_VALUE_IC:
            if (operandEqual(op, code->u.binop.op1) || operandEqual(new_op, code->u.binop.op1))
                return 1;
        case DEC_IC:
        case PARAM_IC:
            break;
        case ASSIGN_IC:
            if (operandEqual(op, code->u.binop.op2)){
                #ifdef OPTIMIZE_IR_DEBUG
                printf("\033[31mreplace:\033[0m %s \033[31min\033[0m %s \033[31mto\033[0m %s\n", 
                    getOperand(op), getCode(code), getOperand(new_op));
                #endif
                free(code->u.binop.op2);
                code->u.binop.op2 = new_op;
                *changed = 1;
            }
            if (operandEqual(op, code->u.binop.op1) || operandEqual(new_op, code->u.binop.op1))
                return 1;
            break;
        case PLUS_IC:
        case MINUS_IC:
        case STAR_IC:
        case DIV_IC:
            if (operandEqual(op, code->u.triop.op2)){
                #ifdef OPTIMIZE_IR_DEBUG
                printf("\033[31mreplace:\033[0m %s \033[31min\033[0m %s \033[31mto\033[0m %s\n", 
                    getOperand(op), getCode(code), getOperand(new_op));
                #endif
                free(code->u.triop.op2);
                code->u.triop.op2 = new_op;
                *changed = 1;
            }
            if (operandEqual(op, code->u.triop.op3)){
                #ifdef OPTIMIZE_IR_DEBUG
                printf("\033[31mreplace:\033[0m %s \033[31min\033[0m %s \033[31mto\033[0m %s\n", 
                    getOperand(op), getCode(code), getOperand(new_op));
                #endif
                free(code->u.triop.op3);
                code->u.triop.op3 = copyOperand(new_op);
                *changed = 1;
            }
            if (operandEqual(op, code->u.triop.op1) || operandEqual(new_op, code->u.triop.op1))
                return 1;
            break;
        case IF_GOTO_IC:{
            if (operandEqual(op, code->u.ifgotoop.op1)){
                #ifdef OPTIMIZE_IR_DEBUG
                printf("\033[31mreplace:\033[0m %s \033[31min\033[0m %s \033[31mto\033[0m %s\n", 
                    getOperand(op), getCode(code), getOperand(new_op));
                #endif
                free(code->u.ifgotoop.op1);
                code->u.ifgotoop.op1 = new_op;
                *changed = 1;
            }
            if (operandEqual(op, code->u.ifgotoop.op3)){
                #ifdef OPTIMIZE_IR_DEBUG
                printf("\033[31mreplace:\033[0m %s \033[31min\033[0m %s \033[31mto\033[0m %s\n", 
                    getOperand(op), getCode(code), getOperand(new_op));
                #endif
                free(code->u.ifgotoop.op3);
                code->u.ifgotoop.op3 = copyOperand(new_op);
                *changed = 1;
            }
            return 1;
        }
        case RETURN_IC:
        case ARG_IC:
        case WRITE_IC:
            if (operandEqual(op, code->u.uniop.op1)){
                #ifdef OPTIMIZE_IR_DEBUG
                printf("\033[31mreplace:\033[0m %s \033[31min\033[0m %s \033[31mto\033[0m %s\n", 
                    getOperand(op), getCode(code), getOperand(new_op));
                #endif
                free(code->u.uniop.op1);
                code->u.uniop.op1 = new_op;
                *changed = 1;
            }
            break;
    }
    return 0;
}

// return 1 means contain op as left value
// return 2 means contain op as right value
int containOperand(InterCode code, Operand op){
    switch (code->kind){
        case LABEL_IC:
        case FUNCTION_IC:
        case GOTO_IC:
        case ARG_IC:
        case RETURN_IC:
        case PARAM_IC:
        case WRITE_IC:
            if (operandEqual(op, code->u.uniop.op1))
                return 2;
            return 0;
        case READ_IC:
            if (operandEqual(op, code->u.uniop.op1))
                return 1;
            return 0;
        case ASSIGN_IC:
        case GET_ADDR_IC:
        case GET_VALUE_IC:
        case DEC_IC:
        case CALL_IC:
            if (operandEqual(op, code->u.binop.op1))
                return 1;
            if (operandEqual(op, code->u.binop.op2))
                return 2;
            return 0;
        case TO_MEM_IC:
        case MEM_TO_MEM_IC:
            if (operandEqual(op, code->u.binop.op1) || operandEqual(op, code->u.binop.op2))
                return 2;
            return 0;
        case PLUS_IC:
        case MINUS_IC:
        case STAR_IC:
        case DIV_IC:
            if (operandEqual(op, code->u.triop.op1))
                return 1;
            if (operandEqual(op, code->u.triop.op2) || operandEqual(op, code->u.triop.op3))
                return 2;
            return 0;
        case IF_GOTO_IC:
            if (operandEqual(op, code->u.ifgotoop.op1) || operandEqual(op, code->u.ifgotoop.op2)
             || operandEqual(op, code->u.ifgotoop.op3) || operandEqual(op, code->u.ifgotoop.op4))
                return 2;
            return 0;
    }
    return 1;
}

int visitBlockbyLine(int line){
    int change = 0;
    BasicBlock temp = block_head;
    while (temp != NULL){
        if (temp->begin<=line && temp->end>=line){
            if (temp->visit == 1) return change;
            else {
                #ifdef BASICBLOCK_DEBUG
                printf("    visit block %s\n", getOperand(temp->first));
                #endif
                temp->visit = 1;
                change = 1;
                return change;
            }
        }
        temp = temp->next;
    }
}

int visitBlockbyLabel(Operand op){
    int change = 0;
    BasicBlock temp = block_head;
    while (temp != NULL){
        if (operandEqual(op, temp->first) == 1){
            if (temp->visit == 1) return change;
            else {
                #ifdef BASICBLOCK_DEBUG
                printf("    visit block %s\n", getOperand(temp->first));
                #endif
                temp->visit = 1;
                change = 1;
                return change;
            }
        }
        temp = temp->next;
    }
}

void addBlock(int begin, int end, Operand first){
    if (begin > end) return;
    BasicBlock new_block = (BasicBlock)malloc(sizeof(struct BasicBlock_));
    *new_block = (struct BasicBlock_){begin, end, 0, 0, first, NULL};
    if (first->kind==FUNC_OP && strcmp(first->u.name, "main")==0) new_block->visit = 1;
    new_block->next = block_head;
    block_head = new_block;
}

void partitionBlock(){
    int begin = 0;
    Operand op = addOperand(NULL_OP, 0);
    for (int i = 0; i < code_len; ++i){
        InterCode ic = codelist[i];
        if (ic->kind == FUNCTION_IC){
            addBlock(begin, i-1, copyOperand(op));
            begin = i;
            op->kind = FUNC_OP;
            strcpy(op->u.name, ic->u.uniop.op1->u.name);
        }
        else if (ic->kind == LABEL_IC){
            addBlock(begin, i-1, copyOperand(op));
            begin = i;
            op->kind = LABEL_OP;
            op->u.value = ic->u.uniop.op1->u.value;
        }
        else if (ic->kind==GOTO_IC || ic->kind==IF_GOTO_IC){
            addBlock(begin, i, copyOperand(op));
            begin = i+1;
            op->kind = NULL_OP;
            op->u.value = 0;
        }
    }
    addBlock(begin, code_len-1, copyOperand(op));
}

char* getOperand(Operand op){
    char* ret = (char*)malloc(32*sizeof(char));
    if (op==NULL || op->kind==NULL_OP){
        strcpy(ret, "null");
        return ret;
    }
    switch(op->kind){
        case VAR_OP:        sprintf(ret, "v%d", op->u.value);     break;
        case CONST_OP:      sprintf(ret, "#%d", op->u.value);     break;
        case SIZE_OP:       sprintf(ret, "%d", op->u.value);      break;
        case ADDR_OP:       sprintf(ret, "v%d", op->u.value);     break;
        case TEMP_ADDR_OP:  sprintf(ret, "t%d", op->u.value);     break;
        case TEMP_OP:       sprintf(ret, "t%d", op->u.value);     break;
        case LABEL_OP:      sprintf(ret, "label%d", op->u.value); break;
        case FUNC_OP:       strcpy(ret, op->u.name);              break;
        case OPERATOR_OP:{
            switch(op->u.value){
                case 0: sprintf(ret, ">");  break;
                case 1: sprintf(ret, "<");  break;
                case 2: sprintf(ret, ">="); break;
                case 3: sprintf(ret, "<="); break;
                case 4: sprintf(ret, "=="); break;
                case 5: sprintf(ret, "!="); break;
            }
            break;
        }
    }
    return ret;
}

char* getCode(InterCode code){
    char *ret;
    if (code == NULL){
        ret = (char*)malloc(sizeof("null")+1);
        strcpy(ret, "null");
        return ret;
    }
    char *op1, *op2, *op3, *op4;
    switch (code->kind){
        case LABEL_IC:{
            op1 = getOperand(code->u.uniop.op1);
            ret = (char*)malloc(strlen(op1)+sizeof("LABEL  : ")+1);
            sprintf(ret, "LABEL %s : ", op1);
            free(op1);
            break;
        }
        case FUNCTION_IC:{
            op1 = getOperand(code->u.uniop.op1);
            ret = (char*)malloc(strlen(op1)+sizeof("FUNCTION  : ")+1);
            sprintf(ret, "FUNCTION %s : ", op1);
            free(op1);
            break;
        }
        case ASSIGN_IC:{
            op1 = getOperand(code->u.binop.op1);
            op2 = getOperand(code->u.binop.op2);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+sizeof(" := ")+1);
            sprintf(ret, "%s := %s", op1, op2);
            free(op1); free(op2);
            break;
        }
        case PLUS_IC:{
            op1 = getOperand(code->u.triop.op1);
            op2 = getOperand(code->u.triop.op2);
            op3 = getOperand(code->u.triop.op3);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+strlen(op3)+sizeof(" :=  + ")+1);
            sprintf(ret, "%s := %s + %s", op1, op2, op3);
            free(op1); free(op2); free(op3);
            break;
        }
        case MINUS_IC:{
            op1 = getOperand(code->u.triop.op1);
            op2 = getOperand(code->u.triop.op2);
            op3 = getOperand(code->u.triop.op3);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+strlen(op3)+sizeof(" :=  - ")+1);
            sprintf(ret, "%s := %s - %s", op1, op2, op3);
            free(op1); free(op2); free(op3);
            break;
        }
        case STAR_IC:{
            op1 = getOperand(code->u.triop.op1);
            op2 = getOperand(code->u.triop.op2);
            op3 = getOperand(code->u.triop.op3);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+strlen(op3)+sizeof(" :=  * ")+1);
            sprintf(ret, "%s := %s * %s", op1, op2, op3);
            free(op1); free(op2); free(op3);
            break;
        }
        case DIV_IC:{
            op1 = getOperand(code->u.triop.op1);
            op2 = getOperand(code->u.triop.op2);
            op3 = getOperand(code->u.triop.op3);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+strlen(op3)+sizeof(" :=  / ")+1);
            sprintf(ret, "%s := %s / %s", op1, op2, op3);
            free(op1); free(op2); free(op3);
            break;
        }
        case GET_ADDR_IC:{
            op1 = getOperand(code->u.binop.op1);
            op2 = getOperand(code->u.binop.op2);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+sizeof(" := &")+1);
            sprintf(ret, "%s := &%s", op1, op2);
            free(op1); free(op2);
            break;
        }
        case GET_VALUE_IC:{
            op1 = getOperand(code->u.binop.op1);
            op2 = getOperand(code->u.binop.op2);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+sizeof(" := *")+1);
            sprintf(ret, "%s := *%s", op1, op2);
            free(op1); free(op2);
            break;
        }
        case TO_MEM_IC:{
            op1 = getOperand(code->u.binop.op1);
            op2 = getOperand(code->u.binop.op2);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+sizeof("* := ")+1);
            sprintf(ret, "*%s := %s", op1, op2);
            free(op1); free(op2);
            break;
        }
        case GOTO_IC:{
            op1 = getOperand(code->u.uniop.op1);
            ret = (char*)malloc(strlen(op1)+sizeof("GOTO ")+1);
            sprintf(ret, "GOTO %s", op1);
            free(op1);
            break;
        }
        case IF_GOTO_IC:{
            op1 = getOperand(code->u.ifgotoop.op1);
            op2 = getOperand(code->u.ifgotoop.op2);
            op3 = getOperand(code->u.ifgotoop.op3);
            op4 = getOperand(code->u.ifgotoop.op4);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+strlen(op3)+strlen(op4)+sizeof("IF    GOTO ")+1);
            sprintf(ret, "IF %s %s %s GOTO %s", op1, op2, op3, op4);
            free(op1); free(op2); free(op3); free(op4);
            break;
        }
        case RETURN_IC:{
            op1 = getOperand(code->u.uniop.op1);
            ret = (char*)malloc(strlen(op1)+sizeof("RETURN ")+1);
            sprintf(ret, "RETURN %s", op1);
            free(op1);
            break;
        }
        case DEC_IC:{
            op1 = getOperand(code->u.binop.op1);
            op2 = getOperand(code->u.binop.op2);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+sizeof("DEC  ")+1);
            sprintf(ret, "DEC %s %s", op1, op2);
            free(op1); free(op2);
            break;
        }
        case ARG_IC:{
            op1 = getOperand(code->u.uniop.op1);
            ret = (char*)malloc(strlen(op1)+sizeof("ARG ")+1);
            sprintf(ret, "ARG %s", op1);
            free(op1);
            break;
        }
        case CALL_IC:{
            op1 = getOperand(code->u.binop.op1);
            op2 = getOperand(code->u.binop.op2);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+sizeof(" := CALL ")+1);
            sprintf(ret, "%s := CALL %s", op1, op2);
            free(op1); free(op2);
            break;
        }
        case PARAM_IC:{
            op1 = getOperand(code->u.uniop.op1);
            ret = (char*)malloc(strlen(op1)+sizeof("PARAM ")+1);
            sprintf(ret, "PARAM %s", op1);
            free(op1);
            break;
        }
        case READ_IC:{
            op1 = getOperand(code->u.uniop.op1);
            ret = (char*)malloc(strlen(op1)+sizeof("READ ")+1);
            sprintf(ret, "READ %s", op1);
            free(op1);
            break;
        }
        case WRITE_IC:{
            op1 = getOperand(code->u.uniop.op1);
            ret = (char*)malloc(strlen(op1)+sizeof("WRITE ")+1);
            sprintf(ret, "WRITE %s", op1);
            free(op1);
            break;
        }
        case MEM_TO_MEM_IC:{
            op1 = getOperand(code->u.binop.op1);
            op2 = getOperand(code->u.binop.op2);
            ret = (char*)malloc(strlen(op1)+strlen(op2)+sizeof("* := *")+1);
            sprintf(ret, "*%s := *%s", op1, op2);
            free(op1); free(op2);
            break;
        }
    }
    return ret;
}

void printCode(char* filename){
    if (filename == NULL) filename = "debug.ir";
    FILE* out_f = fopen(filename, "w");
    if (!out_f) {
        perror(filename);
        exit(1);
    }

    for (int i = 0; i < code_len; ++i){
        #ifdef BASICBLOCK_DEBUG
        BasicBlock temp = block_head;
        while (temp != NULL){
            if (temp->begin == i){
                fprintf(out_f, "*** %s *** visit: %d\n", getOperand(temp->first), temp->visit);
                break;
            }
            temp = temp->next;
        }
        #endif
        fprintf(out_f, "%s\n", getCode(codelist[i]));
    }
}
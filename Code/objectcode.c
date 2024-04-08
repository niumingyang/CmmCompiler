#include "objectcode.h"

Varptr* varlist;
int var_cnt;
int var_cap;
struct Register reg[REG_NUM];
int reg_can_use[CAN_USE];
int arg_num;
int stkdepth;
Operand param_in_reg[4];
int par_num;

void initObjectCode(){
    var_cnt = 0;
    var_cap = 10000;
    varlist = (Varptr*)malloc(var_cap * sizeof(Varptr));

    char all_reg_name[REG_NUM][10] = {
        "$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3",
        "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7",
        "$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7",
        "$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra"
    };
    for (int i = 0; i < REG_NUM; ++i) {
        strcpy(reg[i].name, all_reg_name[i]);
        reg[i].var = -1;
    }
    int all_use_regs[CAN_USE] = {
        8,9,10,11,12,13,14,15,24,25, // t0~t9
        16,17,18,19,20,21,22,23, // s0~s7
        3 // v1 (v0 is reserved)
    };
    for (int i = 0; i < CAN_USE; ++i)
        reg_can_use[i] = all_use_regs[i];

    arg_num = 0;
    par_num = 0;
    stkdepth = 0;
    for (int i = 0; i < 4; ++i)
        param_in_reg[i] = NULL;
    srand((unsigned)time(NULL));
}

int getVar(Operand op, FILE* fp) {
    if (op == NULL) return -1;
    for (int i = 0; i < var_cnt; ++i)
        if (operandEqual(op, varlist[i]->var) == 1)
            return i;
    if (var_cnt >= var_cap) {
		var_cap *= 2;
		varlist = (Varptr*)realloc(varlist, sizeof(Varptr)*var_cap);
	}
    // new varible
    Varptr new_var = (Varptr)malloc(sizeof(struct Varible));
    new_var->var = copyOperand(op);
    new_var->reg = -1;
    new_var->offset = -1;
	varlist[var_cnt++] = new_var;
    if (op->kind != CONST_OP && fp != NULL && isParam(op) == 0){
        new_var->offset = stkdepth;
        stkdepth -= 4;
        #ifdef OBJECTCODE_DEBUG
        fprintf(fp, "    # allocate stack space for %s, offset: %d\n", getOperand(op), new_var->offset);
        #endif
    }
    return var_cnt-1;
}

int ensureReg(int varno, int use_val, FILE* fp){
    int result;
    if (varno == -1)
        return allocateReg(varno, fp);
    if (varlist[varno]->reg != -1)
        result = varlist[varno]->reg;
    else {
        result = allocateReg(varno, fp);
        if (use_val == 1){
            if (varlist[varno]->var->kind == CONST_OP)
                fprintf(fp, "  li %s, %d\n", reg[result].name, varlist[varno]->var->u.value);
            else fprintf(fp, "  lw %s, %d($fp)\n", reg[result].name, varlist[varno]->offset);
        }
    }
    return result;
}

int allocateReg(int varno, FILE* fp){
    int result = -1;
    for (int i = 0; i < CAN_USE; ++i){
        if (reg[reg_can_use[i]].var == -1){
            result = reg_can_use[i];
            break;
        }
    }
    if (result == -1){
        result = 16+rand()%8;
        spillVar(reg[result].var, fp);
    }
    reg[result].var = varno;
    varlist[varno]->reg = result;
    #ifdef OBJECTCODE_DEBUG
    fprintf(fp, "  # allocate reg: %s in %s, offset: %d\n", getOperand(varlist[varno]->var), reg[result].name, varlist[varno]->offset);
    #endif
    return result;
}

void allocateAllVar(int lineno, FILE* fp){
    for (int i = lineno+1; i < code_len; ++i){
        InterCode ic = codelist[i];
        switch(ic->kind){
            case LABEL_IC: 
            case GOTO_IC: 
                break;
            case PARAM_IC:
                par_num++;
                addParam(ic->u.uniop.op1);
                break;
            case FUNCTION_IC: 
                return;
            case ASSIGN_IC: 
            case MEM_TO_MEM_IC: 
            case GET_ADDR_IC: 
            case GET_VALUE_IC:
            case TO_MEM_IC: 
                getVar(ic->u.binop.op1, fp);
                getVar(ic->u.binop.op2, fp);
                break;
            case PLUS_IC: 
            case MINUS_IC: 
            case STAR_IC: 
            case DIV_IC: 
                getVar(ic->u.triop.op1, fp);
                getVar(ic->u.triop.op2, fp);
                getVar(ic->u.triop.op3, fp);
                break;
            case IF_GOTO_IC: 
                getVar(ic->u.ifgotoop.op1, fp);
                getVar(ic->u.ifgotoop.op3, fp);
                break;
            case RETURN_IC:  
            case READ_IC: 
            case WRITE_IC: 
            case ARG_IC: 
                getVar(ic->u.uniop.op1, fp);
                break;
            case CALL_IC: 
                getVar(ic->u.binop.op1, fp);
                break;
            case DEC_IC: {
                int varno = getVar(ic->u.binop.op1, NULL);
                int size = ic->u.binop.op2->u.value;
                Operand op = ic->u.binop.op1;
                stkdepth -= size;
                varlist[varno]->offset = stkdepth+4;
                #ifdef OBJECTCODE_DEBUG
                fprintf(fp, "    # allocate stack space for %s, offset: %d\n", getOperand(op), varlist[varno]->offset);
                #endif
                break;
            } 
        }
    }
    return;
}

void spillVar(int varno, FILE* fp){
    if (varlist[varno]->reg == -1) return;
    if (varlist[varno]->var->kind == CONST_OP){
        reg[varlist[varno]->reg].var = -1;
        varlist[varno]->reg = -1;
        return;
    }
    #ifdef OBJECTCODE_DEBUG
    fprintf(fp, "  # spill var: %s in %s\n", getOperand(varlist[varno]->var), reg[varlist[varno]->reg].name);
    #endif
    if (varlist[varno]->offset == -1){
        assert(0);
    }
    else{
        fprintf(fp, "  sw %s, %d($fp)\n", reg[varlist[varno]->reg].name, varlist[varno]->offset);
        reg[varlist[varno]->reg].var = -1;
        varlist[varno]->reg = -1;
    }
}

void spillAllVar(FILE* fp){
    for (int i = 0; i < CAN_USE; ++i){
        if (reg[reg_can_use[i]].var != -1)
            spillVar(reg[reg_can_use[i]].var, fp);
    }
}

int getReg(Operand op, int use_val, FILE* fp) {
    if (op==NULL || op->kind==NULL_OP) return -1;
    int param_or_not = isParam(op);
    if (param_or_not != 0)
        return 3+param_or_not;
    int var_no = getVar(op, fp);
    int ret = ensureReg(var_no, use_val, fp);
    return ret;
}

void collectReg(int regno, int lineno, FILE* fp){
    if (reg[regno].var == -1) return;
    if (varlist[reg[regno].var]->var->kind == CONST_OP){
        varlist[reg[regno].var]->reg = -1;
        reg[regno].var = -1;
        return;
    }
    for (int i = lineno+1; i < code_len; ++i)
        if (containOperand(codelist[i], varlist[reg[regno].var]->var) != 0)
            return;
    spillVar(reg[regno].var, fp);
}

void addParam(Operand op){
    if (par_num <= 4)
        param_in_reg[par_num-1] = copyOperand(op);
    else{
        int varno = getVar(op, NULL);
        varlist[varno]->offset = 4*(par_num-4);
    }
}

void delAllParam(){
    for (int i = 0; i < 4; ++i){
        free(param_in_reg[i]);
        param_in_reg[i] = NULL;
    }
    par_num = 0;
}

int isParam(Operand op){
    for (int i = 0; i < 4; ++i)
        if (operandEqual(param_in_reg[i], op) == 1)
            return i+1;
    return 0;
}

void translateIR(int lineno, FILE* fp) {
    InterCode ic = codelist[lineno];
    if (ic == NULL) return;
    switch (ic->kind){
        case LABEL_IC: {
            spillAllVar(fp);
            fprintf(fp, "label%d:\n", ic->u.uniop.op1->u.value);
            break;
        }
        case FUNCTION_IC: {
            stkdepth = 0;
            delAllParam();
            fprintf(fp, "\n%s:\n", ic->u.uniop.op1->u.name);
            allocateAllVar(lineno, fp);
            fprintf(fp, "  addi $fp, $sp, -4\n");
            fprintf(fp, "  addi $sp, $sp, %d\n", stkdepth);
            break;
        }
        case ASSIGN_IC: {
            if (ic->u.binop.op2->kind == CONST_OP){
                int reg1 = getReg(ic->u.binop.op1, 0, fp); 
                int val = ic->u.binop.op2->u.value;
                fprintf(fp, "  li %s, %d\n", reg[reg1].name, val);
                collectReg(reg1, lineno, fp);
            }
            else{
                int reg1 = getReg(ic->u.binop.op1, 0, fp);
                int reg2 = getReg(ic->u.binop.op2, 1, fp);
                fprintf(fp, "  move %s, %s\n", reg[reg1].name, reg[reg2].name);
                collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp);
            }
            break;
        }
        case PLUS_IC: {
            if (ic->u.triop.op3->kind==CONST_OP && ic->u.triop.op3->u.value>=-32768 && ic->u.triop.op3->u.value<=32767){
                int reg1 = getReg(ic->u.triop.op1, 0, fp);
                int reg2 = getReg(ic->u.triop.op2, 1, fp);
                int val = ic->u.triop.op3->u.value;
                fprintf(fp, "  addi %s, %s, %d\n", reg[reg1].name, reg[reg2].name, val);
                collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp);
            }
            else{
                int reg1 = getReg(ic->u.triop.op1, 0, fp);
                int reg2 = getReg(ic->u.triop.op2, 1, fp);
                int reg3 = getReg(ic->u.triop.op3, 1, fp);
                fprintf(fp, "  add %s, %s, %s\n", reg[reg1].name, reg[reg2].name, reg[reg3].name);
                collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp); collectReg(reg3, lineno, fp);
            }
            break;
        }
        case MINUS_IC: {
            if (ic->u.triop.op3->kind==CONST_OP && ic->u.triop.op3->u.value>=-32768 && ic->u.triop.op3->u.value<=32767){
                int reg1 = getReg(ic->u.triop.op1, 0, fp);
                int reg2 = getReg(ic->u.triop.op2, 1, fp);
                int val = ic->u.triop.op3->u.value;
                fprintf(fp, "  addi %s, %s, %d\n", reg[reg1].name, reg[reg2].name, -val);
                collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp);
            }
            else{
                int reg1 = getReg(ic->u.triop.op1, 0, fp);
                int reg2 = getReg(ic->u.triop.op2, 1, fp);
                int reg3 = getReg(ic->u.triop.op3, 1, fp);
                fprintf(fp, "  sub %s, %s, %s\n", reg[reg1].name, reg[reg2].name, reg[reg3].name);
                collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp); collectReg(reg3, lineno, fp);
            }
            break;
        }
        case STAR_IC: {
            int reg1 = getReg(ic->u.triop.op1, 0, fp);
            int reg2 = getReg(ic->u.triop.op2, 1, fp);
            int reg3 = getReg(ic->u.triop.op3, 1, fp);
            fprintf(fp, "  mul %s, %s, %s\n", reg[reg1].name, reg[reg2].name, reg[reg3].name);
            collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp); collectReg(reg3, lineno, fp);
            break;
        }
        case DIV_IC: {
            int reg1 = getReg(ic->u.triop.op1, 0, fp);
            int reg2 = getReg(ic->u.triop.op2, 1, fp);
            int reg3 = getReg(ic->u.triop.op3, 1, fp);
            fprintf(fp, "  div %s, %s\n", reg[reg2].name, reg[reg3].name);
            fprintf(fp, "  mflo %s\n", reg[reg1].name);
            collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp); collectReg(reg3, lineno, fp);
            break;
        }
        case GET_ADDR_IC: {
            int reg1 = getReg(ic->u.binop.op1, 0, fp);
            int oft = varlist[getVar(ic->u.binop.op2, fp)]->offset;
            fprintf(fp, "  addi %s, $fp, %d\n", reg[reg1].name, oft);
            collectReg(reg1, lineno, fp);
            break;
        }
        case GET_VALUE_IC: {
            int reg1 = getReg(ic->u.binop.op1, 0, fp);
            int reg2 = getReg(ic->u.binop.op2, 1, fp);
            fprintf(fp, "  lw %s, 0(%s)\n", reg[reg1].name, reg[reg2].name);
            collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp);
            break;
        }
        case TO_MEM_IC: {
            int reg1 = getReg(ic->u.binop.op1, 1, fp);
            int reg2 = getReg(ic->u.binop.op2, 1, fp);
            fprintf(fp, "  sw %s, 0(%s)\n", reg[reg2].name, reg[reg1].name);
            collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp);
            break;
        }
        case GOTO_IC: {
            spillAllVar(fp);
            fprintf(fp, "  j label%d\n", ic->u.uniop.op1->u.value);
            break;
        }
        case IF_GOTO_IC: {
            spillAllVar(fp);
            int reg1 = getReg(ic->u.ifgotoop.op1, 1, fp);
            int reg2 = getReg(ic->u.ifgotoop.op3, 1, fp);
            int label = ic->u.ifgotoop.op4->u.value;
            switch (ic->u.ifgotoop.op2->u.value){
                case 0: fprintf(fp, "  bgt %s, %s, label%d\n", reg[reg1].name, reg[reg2].name, label); break;
                case 1: fprintf(fp, "  blt %s, %s, label%d\n", reg[reg1].name, reg[reg2].name, label); break;
                case 2: fprintf(fp, "  bge %s, %s, label%d\n", reg[reg1].name, reg[reg2].name, label); break;
                case 3: fprintf(fp, "  ble %s, %s, label%d\n", reg[reg1].name, reg[reg2].name, label); break;
                case 4: fprintf(fp, "  beq %s, %s, label%d\n", reg[reg1].name, reg[reg2].name, label); break;
                case 5: fprintf(fp, "  bne %s, %s, label%d\n", reg[reg1].name, reg[reg2].name, label); break;
            }
            collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp);
            break;
        }
        case RETURN_IC: {
            int reg1 = getReg(ic->u.uniop.op1, 1, fp);
            fprintf(fp, "  move $v0, %s\n", reg[reg1].name);
            fprintf(fp, "  addi $sp, $sp, %d\n", -stkdepth);
            fprintf(fp, "  jr $ra\n");
            collectReg(reg1, lineno, fp); 
            break;
        } 
        case DEC_IC: {
            break;
        } 
        case ARG_IC: {
            arg_num++;
            break;
        }
        case CALL_IC: {
            // store special reg
            int args = arg_num>4?4:arg_num;
            fprintf(fp, "  addi $sp, $sp, %d\n", -4*(args+2));
            for (int i = 0; i < args; ++i)
                fprintf(fp, "  sw $a%d, %d($sp)\n", i, 4*(args+1-i));
            fprintf(fp, "  sw $fp, 4($sp)\n");
            fprintf(fp, "  sw $ra, ($sp)\n");
            // store arguments
            for (int i = 0; i < args; ++i){
                int reg1 = getReg(codelist[lineno-i-1]->u.uniop.op1, 1, fp);
                if (reg1>=4 && reg1<=7)
                    fprintf(fp, "  lw $a%d, %d($sp)\n", i, 4*(args+1-(reg1-4)));
                else fprintf(fp, "  move $a%d, %s\n", i, reg[reg1].name);
                collectReg(reg1, lineno, fp); 
            }
            if (arg_num > 4){
                fprintf(fp, "  subu $sp, $sp, %d\n", 4*(arg_num-4));
                for (int i = 0; i < arg_num-4; ++i){
                    int reg1 = getReg(codelist[lineno-i-5]->u.uniop.op1, 1, fp);
                    fprintf(fp, "  sw %s, %d($sp)\n", reg[reg1].name, 4*i);
                    collectReg(reg1, lineno, fp); 
                }
            }
            // store active regs
            int reg_active = 0;
            for (int i = 0; i < CAN_USE; ++i)
                if (reg[reg_can_use[i]].var != -1)
                    reg_active++;
            fprintf(fp, "  addi $sp, $sp, %d\n", -4*reg_active);
            int index = 0;
            for (int i = 0; i < CAN_USE && index < reg_active; ++i)
                if (reg[reg_can_use[i]].var != -1){
                    fprintf(fp, "  sw %s, %d($sp)\n", reg[reg_can_use[i]].name, 4*index);
                    index++;
                }
            // jump
            fprintf(fp, "  jal %s\n", ic->u.binop.op2->u.name);
            // restore regs
            index = 0;
            for (int i = 0; i < CAN_USE && index < reg_active; ++i)
                if (reg[reg_can_use[i]].var != -1){
                    fprintf(fp, "  lw %s, %d($sp)\n", reg[reg_can_use[i]].name, 4*index);
                    index++;
                }
            if (arg_num > 4)
                fprintf(fp, "  addi $sp, $sp, %d\n", 4*(arg_num-4)+4*reg_active);
            else fprintf(fp, "  addi $sp, $sp, %d\n", 4*reg_active);
            fprintf(fp, "  lw $ra, ($sp)\n");
            fprintf(fp, "  lw $fp, 4($sp)\n");
            for (int i = 0; i < args; ++i)
                fprintf(fp, "  lw $a%d, %d($sp)\n", i, 4*(1+args-i));
            fprintf(fp, "  addi $sp, $sp, %d\n", 4*(2+args));
            // return value
            int ret_reg = getReg(ic->u.binop.op1, 0, fp);
            if (ret_reg != -1){
                fprintf(fp, "  move %s, $v0\n", reg[ret_reg].name);
                collectReg(ret_reg, lineno, fp);
            }
            arg_num = 0;
            break;
        }
        case PARAM_IC: {
            break;
        }
        case READ_IC: {
            // store active regs
            fprintf(fp, "  addi $sp, $sp, -8\n");
            fprintf(fp, "  sw $a0, 4($sp)\n");
            fprintf(fp, "  sw $ra, ($sp)\n");
            // jump
            fprintf(fp, "  jal read\n");
            // restore regs
            fprintf(fp, "  lw $ra, ($sp)\n");
            fprintf(fp, "  lw $a0, 4($sp)\n");
            fprintf(fp, "  addi $sp, $sp, 8\n");
            // return value
            int ret_reg = getReg(ic->u.uniop.op1, 0, fp);
            if (ret_reg != -1){
                fprintf(fp, "  move %s, $v0\n", reg[ret_reg].name);
                collectReg(ret_reg, lineno, fp);
            }
            break;
        }
        case WRITE_IC: {
            // store active regs
            fprintf(fp, "  addi $sp, $sp, -8\n");
            fprintf(fp, "  sw $a0, 4($sp)\n");
            fprintf(fp, "  sw $ra, ($sp)\n");
            // store arguments
            int reg1 = getReg(ic->u.uniop.op1, 1, fp);
            if (reg1 != 4)
                fprintf(fp, "  move $a0, %s\n", reg[reg1].name);
            collectReg(reg1, lineno, fp); 
            // jump
            fprintf(fp, "  jal write\n");
            // restore regs
            fprintf(fp, "  lw $ra, ($sp)\n");
            fprintf(fp, "  lw $a0, 4($sp)\n");
            fprintf(fp, "  addi $sp, $sp, 8\n");
            break;
        }
        case MEM_TO_MEM_IC: {
            int reg1 = getReg(ic->u.binop.op1, 1, fp);
            int reg2 = getReg(ic->u.binop.op2, 1, fp);
            int reg3 = getReg(NULL, 0, fp);
            fprintf(fp, "  lw %s, 0(%s)\n", reg[reg3].name, reg[reg2].name);
            fprintf(fp, "  sw %s, 0(%s)\n", reg[reg3].name, reg[reg1].name);
            collectReg(reg1, lineno, fp); collectReg(reg2, lineno, fp);
            break;
        }
    }
}

void translateCode(char* filename) {
    FILE* out_f = fopen(filename, "w");
    if (!out_f) {
        perror(filename);
        exit(1);
    }

    fprintf(out_f, 
        ".data\n"
        "_prompt: .asciiz \"Enter an integer:\"\n"
        "_ret: .asciiz \"\\n\"\n"
        ".globl main\n"
        ".text\n"
        "read:\n"
        "  li $v0, 4\n"
        "  la $a0, _prompt\n"
	    "  syscall\n"
	    "  li $v0, 5\n"
	    "  syscall\n"
	    "  jr $ra\n"
        "\n"
	    "write:\n"
	    "  li $v0, 1\n"
	    "  syscall\n"
	    "  li $v0, 4\n"
	    "  la $a0, _ret\n"
	    "  syscall\n"
	    "  move $v0, $0\n"
	    "  jr $ra\n"
    );
    for (int i = 0; i < code_len; ++i){
        #ifdef OBJECTCODE_DEBUG
        fprintf(out_f, "# line %d: %s\n",i,getCode(codelist[i]));
        #endif
        translateIR(i, out_f);
    }
}
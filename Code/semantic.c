#include "semantic.h"

FieldList symbol_table[HASH_SIZE+1];
BlockPtr stack[MAX_DEPTH];
int depth; // level of nesting
int sema_error;

unsigned int hash_pjw(char* name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if (i = val & ~HASH_SIZE) 
            val = (val ^ (i >> 12)) & HASH_SIZE;
    }
    return val;
}

void initSymbolTable() {
    for (int i = 0; i < HASH_SIZE; ++i)
        symbol_table[i] = NULL;
    depth = 0;
    sema_error = 0;

    initIntercode();
    // add function int read() and int write(int)
    Type int_type = (Type)malloc(sizeof(struct Type_));
    int_type->kind = BASIC;
    int_type->u.basic = BASIC_INT;

    FieldList read_func = (FieldList)malloc(sizeof(struct FieldList_));
    read_func->name = (char*)malloc(sizeof("read")+1);
    strcpy(read_func->name, "read");
    read_func->type = (Type)malloc(sizeof(struct Type_));
    read_func->type->kind = FUNCTION;
    read_func->type->u.function.hasdef = 0;
    read_func->type->u.function.ret_type = int_type;
    read_func->type->u.function.param_num = 0;
    read_func->type->u.function.param_info = NULL;
    read_func->next = NULL;
    addSymbol(read_func, 0);

    FieldList write_func = (FieldList)malloc(sizeof(struct FieldList_));
    write_func->name = (char*)malloc(sizeof("write")+1);
    strcpy(write_func->name, "write");
    write_func->type = (Type)malloc(sizeof(struct Type_));
    write_func->type->kind = FUNCTION;
    write_func->type->u.function.hasdef = 0;
    write_func->type->u.function.ret_type = int_type;
    write_func->type->u.function.param_num = 1;
    write_func->type->u.function.param_info = (FieldList)malloc(sizeof(struct FieldList_));
    *write_func->type->u.function.param_info = (struct FieldList_){NULL, int_type, NULL};
    write_func->next = NULL;
    addSymbol(write_func, 0);
}


int addSymbol(FieldList symb, int dep){
    if (symb == NULL) return -1;
    if (findSymbolInOneDep(symb->name, symb->type->kind==FUNCTION, dep) != NULL)
        return 0;
    unsigned int key = hash_pjw(symb->name);
    if (symbol_table[key] == NULL)
        symbol_table[key] = symb;
    else {
        symb->next = symbol_table[key];
        symbol_table[key] = symb;
    }
    BlockPtr bp = (BlockPtr)malloc(sizeof(struct BlockSymbol_));
    bp->member = symb;
    if (stack[dep] == NULL){
        stack[dep] = bp;
        bp->next = NULL;
    }
    else {
        bp->next = stack[dep];
        stack[dep] = bp;
    }
    #ifdef SEMANTIC_DEBUG
    printf("\033[31madd depth %d---\033[0m", dep);
    printFieldList(symb);
    printf("***********after addition***********\n");
    printSymbolTable();
    printf("************************************\n\n");
    #endif
    return 1;
}

FieldList findSymbol(char* name, int isfunc){ // search all the symbols
    unsigned int key = hash_pjw(name);
    FieldList now = symbol_table[key];
    while (now != NULL){
        if (strcmp(now->name, name) == 0){
            if ((isfunc==1 && now->type->kind==FUNCTION) || (isfunc==0 && now->type->kind!=FUNCTION))
                return now; // the first is always the newest
        }
        now = now->next;
    }
    return NULL;
}

FieldList findSymbolInOneDep(char* name, int isfunc, int dep){ // search symbols in one depth
    BlockPtr now = stack[dep];
    while (now != NULL){
        if (strcmp(now->member->name, name)==0){
            if (now->member->type->kind==FUNCTION && isfunc==1)
                return now->member;
            if (now->member->type->kind!=FUNCTION && isfunc==0)
                return now->member;
        }
        now = now->next;
    }
    return NULL;
}

void delSymbol(int dep){
    BlockPtr now = stack[dep];
    stack[dep] = NULL;
    BlockPtr temp;
    while (now != NULL){
        #ifdef SEMANTIC_DEBUG
        printf("\033[32mdelete depth %d---\033[0m", dep);
        printFieldList(now->member);
        #endif
        temp = now;
        now = now->next;
        unsigned int key = hash_pjw(temp->member->name);
        symbol_table[key] = symbol_table[key]->next;
        free(temp->member);
        free(temp);
    }
    #ifdef SEMANTIC_DEBUG
    printf("************after delete************\n");
    printSymbolTable();
    printf("************************************\n\n");
    #endif
}

void semanticProcess(TreeNode* node){
    if (node == NULL) return;
    if (strcmp(node->type, "ExtDef") == 0)
        ExtDefProcess(node);
    if (node->child != NULL) {
        TreeNode* now = node->child;
        while (now != NULL){
            semanticProcess(now);
            now = now->next;
        }
    }
    if (node == root){
        for (int i = 0; i < HASH_SIZE; ++i)
            if (symbol_table[i]!=NULL && symbol_table[i]->type->kind==FUNCTION 
                && symbol_table[i]->type->u.function.hasdef!=0){
                    sema_error++;
                    printf("Error type 18 at Line %d: function \"%s\" has no definition.\n", symbol_table[i]->type->u.function.hasdef, symbol_table[i]->name);
                }
    }
}

int typeEqual(Type type1, Type type2){
    // printf("type judge:\n");
    // printf("    type1: ");
    // printType(type1);
    // puts("");
    // printf("    type2: ");
    // printType(type2);
    // puts("");
    if (type1==NULL && type2==NULL)
        return 1;
    if (type1==NULL || type2==NULL)
        return 0;
    if (type1->kind != type2->kind)
        return 0;
    switch(type1->kind){
        case BASIC: 
            return type1->u.basic == type2->u.basic;
        case ARRAY: 
            return typeEqual(type1->u.array.array_type, type2->u.array.array_type);
        case STRUCTURE:{
            int flag1 = 0, flag2 = 0;
            Type struct_type1, struct_type2;
            if (type1->u.structure->type == NULL) // normal struct
                struct_type1 = findSymbol(type1->u.structure->name, 0)->type;
            else { // anonymous struct
                struct_type1 = type1;
                type1->kind = STRUCTDEF;
                flag1 = 1;
            }
            if (type2->u.structure->type == NULL) // normal struct
                struct_type2 = findSymbol(type2->u.structure->name, 0)->type;
            else { // anonymous struct
                struct_type2 = type2;
                type2->kind = STRUCTDEF;
                flag2 = 1;
            }
            if (flag1==0 && flag2==0){
                if (strcmp(type1->u.structure->name, type2->u.structure->name) == 0)
                    return 1;
            }
            int ret =  typeEqual(struct_type1, struct_type2);
            if (flag1) type1->kind = STRUCTURE;
            if (flag2) type2->kind = STRUCTURE;
            return ret;
        }
        case STRUCTDEF:{ // the definition of struct
            FieldList temp1 = type1->u.structure;
            FieldList temp2 = type2->u.structure;
            while (temp1!=NULL && temp2!=NULL){
                if (typeEqual(temp1->type, temp2->type) == 0)
                    return 0;
                temp1 = temp1->next;
                temp2 = temp2->next;
            }
            if (temp1==NULL && temp2==NULL)
                return 1;
            else return 0;
        }
        case FUNCTION:{ // judge whether the params are the same
            if (type1->u.function.param_num != type2->u.function.param_num)
                return 0;
            FieldList temp1 = type1->u.function.param_info;
            FieldList temp2 = type2->u.function.param_info;
            while (temp1!=NULL && temp2!=NULL){
                if (typeEqual(temp1->type, temp2->type) == 0)
                    return 0;
                temp1 = temp1->next;
                temp2 = temp2->next;
            }
            if (temp1==NULL && temp2==NULL)
                return 1;
            else return 0;
        }
    }
}

int sizeofType(Type type){
    if (type == NULL) return 0;
    switch (type->kind){
        case BASIC: return 4; // no float
        case ARRAY: return type->u.array.size * sizeofType(type->u.array.array_type);
        case STRUCTURE:
            if (type->u.structure->type == NULL)
                return sizeofType(findSymbol(type->u.structure->name, 0)->type);
        case STRUCTDEF:{
            int ret = 0;
            FieldList temp = type->u.structure;
            while (temp != NULL){
                ret += sizeofType(temp->type);
                temp = temp->next;
            }
            return ret;
        }
    }
}

FieldList VarDecProcess(TreeNode* node, Type _type){
    Type type = _type;
    if (type == NULL) return NULL;
    FieldList ret = (FieldList)malloc(sizeof(struct FieldList_));
    Type newtype;
    while (node->child->next != NULL){
        int size = (int)strtoll(node->child->next->next->info, NULL, 0);
        newtype = (Type)malloc(sizeof(struct Type_));
        newtype->kind = ARRAY;
        newtype->u.array.array_type = type;
        newtype->u.array.size = size;
        newtype->size = size * type->size;
        type = newtype;
        node = node->child;
    }
    ret->name = (char*)malloc(strlen(node->child->info)+1);
    strcpy(ret->name, node->child->info);
    ret->next = NULL;
    ret->type = type;
    return ret;
}

FieldList DefListProcess(TreeNode* node, int isstruct){
    if (node->child == NULL) // DefList is empty string
        return NULL;
    FieldList ret = NULL, head = NULL;
    TreeNode* Def = node->child;
    while (Def != NULL){
        Type Deftype = SpecifierProcess(Def->child);
        TreeNode* Dec = Def->child->next->child;
        while (1){
            FieldList newfield = VarDecProcess(Dec->child, Deftype);
            if (isstruct == 1) {
                if (Dec->child->next!=NULL){
                    sema_error++;
                    printf("Error type 15 at Line %d: Initialization in struct definition.\n", Dec->lineno);
                }
                FieldList newfield_copy = NULL;
                if (newfield != NULL){
                    newfield_copy = (FieldList)malloc(sizeof(struct FieldList_));
                    newfield_copy->name = newfield->name;
                    newfield_copy->type = newfield->type;
                    newfield_copy->next = NULL;
                }
                if (addSymbol(newfield_copy, depth) == 0){
                    sema_error++;
                    printf("Error type 15 at Line %d: Redefined field \"%s\".\n", Dec->lineno, newfield_copy->name);
                }
                if (ret == NULL) {
                    ret = newfield;
                    head = ret;
                }
                else {
                    ret->next = newfield;
                    if (ret->next != NULL)
                        ret = ret->next;
                }
            }
            else {
                if (newfield != NULL){
                    FieldList nowsymbol = findSymbolInOneDep(newfield->name, 0, depth);
                    FieldList allsymbol = findSymbol(newfield->name, 0);
                    if (nowsymbol != NULL){
                        sema_error++;
                        printf("Error type 3 at Line %d: Redefined variable \"%s\".\n", Dec->lineno, newfield->name);
                    } 
                    else {
                        if (allsymbol!=NULL && allsymbol->type->kind==STRUCTDEF){
                            sema_error++;
                            printf("Error type 3 at Line %d: Redefined variable \"%s\".\n", Dec->lineno, newfield->name);
                        }
                        else {
                            addSymbol(newfield, depth);
                            if (newfield->type->kind != BASIC)
                                addIntercode(DEC_IC, 2, 
                                    addOperand(VAR_OP, get_varno(newfield->name)), 
                                    addOperand(SIZE_OP, newfield->type->size));
                            // assign 
                            if (Dec->child->next != NULL){
                                TreeNode* leftvar = Dec->child; 
                                Type type1 = newfield->type;
                                Operand temp2;
                                if (type1!=NULL && type1->kind!=BASIC)
                                    temp2 = addOperand(TEMP_ADDR_OP, new_temp());
                                else temp2 = addOperand(TEMP_OP, new_temp());
                                Type type2 = ExpProcess(leftvar->next->next, temp2);
                                if (typeEqual(type1, type2) == 0){
                                    sema_error++;
                                    printf("Error type 5 at Line %d: Type mismatched for assignment.\n", Dec->lineno);
                                }
                                if (type1->kind == ARRAY) // array assign
                                    arrayAssign(type1, type2, addOperand(ADDR_OP, get_varno(newfield->name)), copyOperand(temp2));
                                else if (type1->kind != BASIC) // struct assign
                                    structAssign(type1, type2, addOperand(ADDR_OP, get_varno(newfield->name)), copyOperand(temp2));
                                else // basic type assign
                                    addIntercode(ASSIGN_IC, 2, addOperand(VAR_OP, get_varno(newfield->name)), copyOperand(temp2));
                            }
                        }
                    }
                }
            }
            if (Dec->next == NULL) break;
            else Dec = Dec->next->next->child;
        }
        if (Def->next == NULL) break;
        Def = Def->next->child;
    }
    return head;
}

Type SpecifierProcess(TreeNode* node){
    Type ret = (Type)malloc(sizeof(struct Type_));
    if (strcmp(node->child->type, "TYPE") == 0){ // TYPE
        ret->kind = BASIC;
        if (strcmp(node->child->info, "int") == 0)
            ret->u.basic = BASIC_INT;
        else ret->u.basic = BASIC_FLOAT;
        ret->size = 4;
    }
    else{ // StructSpecifier
        ret->kind = STRUCTURE;
        if (strcmp(node->child->child->next->type, "Tag") == 0){ // STRUCT Tag
            char* tag_id = node->child->child->next->child->info;
            FieldList item = findSymbol(tag_id, 0);
            if (item == NULL){
                sema_error++;
                printf("Error type 17 at Line %d: Undefined structure \"%s\".\n", node->lineno, tag_id);
                ret = NULL;
            }
            else {
                if (item->type->kind == STRUCTDEF){
                    ret->kind = STRUCTURE;
                    ret->u.structure = (FieldList)malloc(sizeof(struct FieldList_));
                    ret->u.structure->name = (char*)malloc(strlen(tag_id)+1);
                    strcpy(ret->u.structure->name, tag_id);
                    ret->u.structure->type = NULL;
                    ret->size = item->type->size;
                }
                else {
                    // find a varible of same name
                    sema_error++;
                    printf("Error type 17 at Line %d: Undefined structure \"%s\".\n", node->lineno, tag_id);
                    ret = NULL;
                }
            }
        }
        else { // STRUCT OptTag LC DefList RC
            TreeNode* DefList;
            int flag;
            char* s = NULL;
            if (strcmp(node->child->child->next->type, "LC") == 0){
                DefList = node->child->child->next->next;
                flag = 0; // OptTag is empty string
            }
            else{
                DefList = node->child->child->next->next->next;
                flag = 1; // OptTag is not empty string
                s = node->child->child->next->child->info;
            }

            // add struct member
            ret->kind = STRUCTURE;
            depth++;
            ret->u.structure = DefListProcess(DefList, 1);
            delSymbol(depth--);
            int struct_size = sizeofType(ret);
            ret->size = struct_size;

            if (flag == 1){
                // put the struct definition into symbol table
                FieldList newfield = (FieldList)malloc(sizeof(struct FieldList_));
                newfield->name = (char*)malloc(strlen(s)+1);
                strcpy(newfield->name, s);
                ret->kind = STRUCTDEF;
                newfield->type = ret;
                newfield->next = NULL;
                if (addSymbol(newfield, 0) == 0){
                    sema_error++;
                    printf("Error type 16 at Line %d: Duplicated name \"%s\".\n", node->lineno, newfield->name);
                    return NULL;
                }
                // return a STRUCTURE type
                ret = (Type)malloc(sizeof(struct Type_));
                ret->kind = STRUCTURE;
                ret->size = struct_size;
                ret->u.structure = (FieldList)malloc(sizeof(struct FieldList_));
                ret->u.structure->name = (char*)malloc(strlen(newfield->name)+1);
                strcpy(ret->u.structure->name, newfield->name);
                ret->u.structure->type = NULL;
            }
        }
    }
    return ret;
}

void ExtDefProcess(TreeNode* node){
    Type type = SpecifierProcess(node->child);
    if (strcmp(node->child->next->type, "ExtDecList") == 0){ // Specifier ExtDecList SEMI
        TreeNode* VarDec = node->child->next->child;
        while(1){
            FieldList tempfield = VarDecProcess(VarDec, type);
            if (tempfield != NULL){
                if (addSymbol(tempfield, depth) == 0){
                    sema_error++;
                    printf("Error type 3 at Line %d: Redefined variable \"%s\".\n", node->lineno, tempfield->name);
                }
                else {
                    if (tempfield->type->kind != BASIC)
                        addIntercode(DEC_IC, 2, 
                            addOperand(VAR_OP, get_varno(tempfield->name)),
                            addOperand(SIZE_OP, tempfield->type->size));
                }
            }
            if (VarDec->next == NULL) break;
            else VarDec = VarDec->next->next->child;
        }
    }
    else if (strcmp(node->child->next->type, "SEMI") == 0){ // Specifier SEMI
        // do nothing
    }
    else { // Specifier FunDec CompSt | Specifier FunDec SEMI
        // handle FunDec
        TreeNode* FunDec = node->child->next;
        FieldList func = (FieldList)malloc(sizeof(struct FieldList_));
        func->type = (Type)malloc(sizeof(struct Type_));
        func->type->kind = FUNCTION;
        func->type->u.function.ret_type = type;
        char* s = FunDec->child->info;
        func->name = (char*)malloc(strlen(s)+1);
        strcpy(func->name, s);
        func->next = NULL;
        TreeNode* VarList = FunDec->child->next->next;
        if (strcmp(VarList->type, "VarList") != 0){
            func->type->u.function.param_num = 0;
            func->type->u.function.param_info = NULL;
        }
        else{
            TreeNode* ParamDec = VarList->child;
            FieldList temp = NULL;
            while (1){
                func->type->u.function.param_num++;
                Type param_type = SpecifierProcess(ParamDec->child);
                if (temp == NULL){
                    temp = VarDecProcess(ParamDec->child->next, param_type);
                    func->type->u.function.param_info = temp;
                }
                else{
                    temp->next = VarDecProcess(ParamDec->child->next, param_type);
                    if (temp->next != NULL)
                        temp = temp->next;
                }
                temp->type->size = 0;
                if (ParamDec->next == NULL) break;
                else ParamDec = ParamDec->next->next->child; 
            }
            temp->next = NULL;
        }
        FieldList samefunc = findSymbol(func->name, 1);
        // Specifier FunDec CompSt | Specifier FunDec SEMI
        if (strcmp(node->child->next->next->type, "CompSt") == 0){
            func->type->u.function.hasdef = 0;
            if (samefunc != NULL){
                if (typeEqual(func->type, samefunc->type)==1 
                 && typeEqual(func->type->u.function.ret_type, samefunc->type->u.function.ret_type)==1){
                    if (samefunc->type->u.function.hasdef != 0){
                        samefunc->type->u.function.hasdef = 0;
                    }
                    else {
                        sema_error++;
                        printf("Error type 4 at Line %d: Redefined function \"%s\".\n", FunDec->lineno, func->name);
                    }
                }
                else {
                    if (samefunc->type->u.function.hasdef == 0){
                        sema_error++;
                        printf("Error type 4 at Line %d: Redefined function \"%s\".\n", FunDec->lineno, func->name);
                    }
                    else {
                        sema_error++;
                        printf("Error type 19 at Line %d: function \"%s\" has different declaration and definition.\n", FunDec->lineno, func->name);
                    }
                }
            }
            else addSymbol(func, depth);

            // handle CompSt
            ++depth;
            int add_num = func->type->u.function.param_num;
            FieldList add_field = func->type->u.function.param_info;
            for (int i = 0; i < add_num; ++i){
                FieldList add_field_copy = (FieldList)malloc(sizeof(struct FieldList_));
                add_field_copy->name = add_field->name;
                add_field_copy->type = add_field->type;
                add_field_copy->next = NULL;
                if (addSymbol(add_field_copy, depth) == 0){
                    sema_error++;
                    printf("Error type 3 in Line %d: Redefined function parameter.\n", FunDec->lineno);
                }
                add_field = add_field->next;
            }
            // add inter-mediate code
            addIntercode(FUNCTION_IC, 1, addFuncOperand(func->name));
            FieldList temp_var = func->type->u.function.param_info;
            for (int i = 0; i < func->type->u.function.param_num; ++i){
                if (temp_var->type->kind == BASIC)
                    addIntercode(PARAM_IC, 1, addOperand(VAR_OP, get_varno(temp_var->name)));
                else addIntercode(PARAM_IC, 1, addOperand(ADDR_OP, get_varno(temp_var->name)));
                temp_var = temp_var->next;
            }
            CompStProcess(FunDec->next, type);
            delSymbol(depth--);
        }
        else {
            func->type->u.function.hasdef = FunDec->lineno;
            if (samefunc != NULL){
                if (typeEqual(func->type, samefunc->type)==1 
                 && typeEqual(func->type->u.function.ret_type, samefunc->type->u.function.ret_type)==1){
                    // do nothing
                }
                else {
                    if (samefunc->type->u.function.hasdef == 0){
                        sema_error++;
                        printf("Error type 19 at Line %d: function \"%s\" has different declaration and definition.\n", FunDec->lineno, func->name);
                    }
                    else {
                        sema_error++;
                        printf("Error type 19 at Line %d: function \"%s\" has different declarations.\n", FunDec->lineno, func->name);
                    }
                }
            }
            else addSymbol(func, depth);
        }
    }
}

void CompStProcess(TreeNode* node, Type func_ret_type){
    TreeNode* DefList = node->child->next;
    if (strcmp(DefList->type, "DefList") == 0){ // handle DefList
        FieldList alldef = DefListProcess(DefList, 0);
    }
    // handle StmtList
    TreeNode* StmtList = NULL;
    if (strcmp(DefList->type, "StmtList") == 0)
        StmtList = DefList;
    else if (DefList->next != NULL && strcmp(DefList->next->type, "StmtList")==0)
        StmtList = DefList->next;
    if (StmtList != NULL){
        TreeNode* Stmt = StmtList->child;
        while (1){ // handle Stmt
            StmtProcess(Stmt, func_ret_type);
            if (Stmt->next == NULL) break;
            else Stmt = Stmt->next->child;
        }
    }
}

void StmtProcess(TreeNode* node, Type func_ret_type){
    if (strcmp(node->child->type, "Exp") == 0){ // Exp SEMI
        ExpProcess(node->child, addOperand(NULL_OP, 0));
    }
    else if (strcmp(node->child->type, "CompSt") == 0){ // CompSt
        ++depth;
        CompStProcess(node->child, func_ret_type);
        delSymbol(depth--);
    }
    else if (strcmp(node->child->type, "RETURN") == 0){ // RETURN Exp SEMI
        Operand temp1;
        if (func_ret_type->kind != BASIC)
            temp1 = addOperand(TEMP_ADDR_OP, new_temp());
        else temp1 = addOperand(TEMP_OP, new_temp());
        Type type1 = ExpProcess(node->child->next, temp1);
        addIntercode(RETURN_IC, 1, copyOperand(temp1));
        if (typeEqual(func_ret_type, type1) == 0){
            sema_error++;
            printf("Error type 8 at Line %d: Type mismatched for return.\n", node->lineno);
        }
    }
    else if (strcmp(node->child->type, "WHILE") == 0){ // WHILE LP Exp RP Stmt
        TreeNode* Exp = node->child->next->next;
        TreeNode* Stmt = node->child->next->next->next->next;
        int label1 = new_label(), label2 = new_label(), label3 = new_label();
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label1));
        Type temp = CondProcess(Exp, label2, label3);
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label2));
        Type int_type = (Type)malloc(sizeof(struct Type_));
        int_type->kind = BASIC;
        int_type->u.basic = BASIC_INT;
        if (typeEqual(temp, int_type) == 0){
            sema_error++;
            printf("Error type 7 at Line %d: \"While\" condition is not an integer.\n", Exp->lineno);
        }
        free(int_type);
        StmtProcess(Stmt, func_ret_type);
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label1));
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label3));
    }
    else{ // IF LP Exp RP Stmt | IF LP Exp RP Stmt ELSE Stmt
        TreeNode* Exp = node->child->next->next;
        TreeNode* Stmt = node->child->next->next->next->next;
        Type int_type = (Type)malloc(sizeof(struct Type_));
        int_type->kind = BASIC;
        int_type->u.basic = BASIC_INT;
        if (Stmt->next != NULL){ // ELSE stmt
            int label1 = new_label(), label2 = new_label(), label3 = new_label();
            Type temp = CondProcess(Exp, label1, label2);
            addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label1));
            if (typeEqual(temp, int_type) == 0){
                sema_error++;
                printf("Error type 7 at Line %d: \"if\" condition is not an integer.\n", Exp->lineno);
            }
            free(int_type);
            StmtProcess(Stmt, func_ret_type);
            addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label3));
            addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label2));
            Stmt = Stmt->next->next;
            StmtProcess(Stmt, func_ret_type);
            addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label3));
        }
        else { // no ELSE stmt
            int label1 = new_label(), label2 = new_label();
            Type temp = CondProcess(Exp, label1, label2);
            addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label1));
            if (typeEqual(temp, int_type) == 0){
                sema_error++;
                printf("Error type 7 at Line %d: \"if\" condition is not an integer.\n", Exp->lineno);
            }
            free(int_type);
            StmtProcess(Stmt, func_ret_type);
            addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label2));
        }
    }
}

Type ExpProcess(TreeNode* node, Operand place){
    if (node->child->next == NULL){ // ID | INT | FLOAT
        if (strcmp(node->child->type, "ID") == 0){
            FieldList temp = findSymbol(node->child->info, 0);
            if (temp == NULL){
                sema_error++;
                printf("Error type 1 at Line %d: Undefined variable \"%s\".\n", node->lineno, node->child->info);
                return NULL;
            }
            else {
                if (place->kind==ADDR_OP || place->kind==TEMP_ADDR_OP){ // need an address 
                    if (temp->type->size==0 && temp->type->kind!=BASIC) // a parameter of array or struct type
                        addIntercode(ASSIGN_IC, 2, place, addOperand(VAR_OP, get_varno(node->child->info)));
                    else addIntercode(GET_ADDR_IC, 2, place, addOperand(VAR_OP, get_varno(node->child->info)));
                }
                else { // need a varible
                    if (temp->type->size==0 && temp->type->kind!=BASIC) // a parameter of array or struct type
                        addIntercode(GET_VALUE_IC, 2, place, addOperand(VAR_OP, get_varno(node->child->info)));
                    else addIntercode(ASSIGN_IC, 2, place, addOperand(VAR_OP, get_varno(node->child->info)));
                }
                return temp->type;
            }
        }
        else if (strcmp(node->child->type, "INT") == 0){
            Type ret = (Type)malloc(sizeof(struct Type_));
            ret->kind = BASIC;
            ret->u.basic = BASIC_INT;
            if (place->kind==ADDR_OP || place->kind==TEMP_ADDR_OP){
                Operand temp1 = addOperand(TEMP_OP, new_temp());
                addIntercode(ASSIGN_IC, 2, temp1, addOperand(CONST_OP, (int)strtoll(node->child->info, NULL, 0)));
                addIntercode(GET_ADDR_IC, 2, place, copyOperand(temp1));
            }
            else addIntercode(ASSIGN_IC, 2, place, addOperand(CONST_OP, (int)strtoll(node->child->info, NULL, 0)));
            return ret;
        }
        else {
            Type ret = (Type)malloc(sizeof(struct Type_));
            ret->kind = BASIC;
            ret->u.basic = BASIC_FLOAT;
            return ret;
        }
    }
    else if (strcmp(node->child->type, "ID") == 0){ // ID LP Args RP | ID LP RP
        FieldList temp1 = findSymbol(node->child->info, 1);
        FieldList temp2 = findSymbol(node->child->info, 0);
        if (temp1 == NULL){
            sema_error++;
            printf("Error type 2 at Line %d: Undefined function \"%s\".\n", node->lineno, node->child->info);
            if (temp2 != NULL){
                sema_error++;
                printf("Error type 11 at Line %d: \"%s\" is not an function.\n", node->lineno, node->child->info);
            }
            return NULL;
        }
        else {
            TreeNode* Args = node->child->next->next;
            Type now = (Type)malloc(sizeof(struct Type_));
            now->kind = FUNCTION;
            now->u.function.param_num = 0;
            now->u.function.param_info = NULL;
            if (Args->child != NULL){ // ID LP Args RP
                FieldList tail = NULL, fnew;
                FieldList funcfield = temp1->type->u.function.param_info;
                Operand all_args[100];
                int args_num = 0;
                while (1){
                    fnew = (FieldList)malloc(sizeof(struct FieldList_));
                    // add Inter-mediate code: ARG
                    Operand new_arg;
                    if (funcfield->type->kind == BASIC)
                        new_arg = addOperand(TEMP_OP, new_temp());
                    else new_arg = addOperand(TEMP_ADDR_OP, new_temp());
                    fnew->type = ExpProcess(Args->child, new_arg);
                    all_args[args_num++] = copyOperand(new_arg);
                    fnew->next = NULL; 
                    if (now->u.function.param_info == NULL){
                        now->u.function.param_info = fnew;
                        tail = fnew;
                    }
                    else {
                        tail->next = fnew;
                        tail = fnew;
                    }
                    if (funcfield->next != NULL)
                        funcfield = funcfield->next;
                    now->u.function.param_num++;
                    if (Args->child->next == NULL) break;
                    else Args = Args->child->next->next;
                }
                if (strcmp(temp1->name, "write") == 0){
                    addIntercode(WRITE_IC, 1, all_args[0]);
                    addIntercode(ASSIGN_IC, 2, place, addOperand(CONST_OP, 0));
                }
                else{
                    // add the args to IR
                    for (int i = args_num-1; i >= 0; --i)
                        addIntercode(ARG_IC, 1, all_args[i]);
                    // add Inter-mediate code: CALL
                    if (place->kind==ADDR_OP || place->kind==TEMP_ADDR_OP){
                        if (temp1->type->u.function.ret_type->kind == BASIC){
                            Operand new_op = addOperand(TEMP_ADDR_OP, new_temp());
                            addIntercode(CALL_IC, 2, new_op, addFuncOperand(temp1->name));
                            addIntercode(GET_ADDR_IC, 2, place, copyOperand(new_op));
                        }
                        else addIntercode(CALL_IC, 2, place, addFuncOperand(temp1->name));
                    }
                    else {
                        if (temp1->type->u.function.ret_type->kind == BASIC)
                            addIntercode(CALL_IC, 2, place, addFuncOperand(temp1->name));
                        else {
                            Operand new_op = addOperand(TEMP_ADDR_OP, new_temp());
                            addIntercode(CALL_IC, 2, new_op, addFuncOperand(temp1->name));
                            addIntercode(GET_VALUE_IC, 2, place, copyOperand(new_op));
                        }
                    }
                }
            }
            else { // ID LP RP
                if (strcmp(temp1->name, "read") == 0)
                    addIntercode(READ_IC, 1, place);
                else{
                    // add Inter-mediate code: CALL
                    if (place->kind==ADDR_OP || place->kind==TEMP_ADDR_OP){
                        if (temp1->type->u.function.ret_type->kind == BASIC){
                            Operand new_op = addOperand(TEMP_ADDR_OP, new_temp());
                            addIntercode(CALL_IC, 2, new_op, addFuncOperand(temp1->name));
                            addIntercode(GET_ADDR_IC, 2, place, copyOperand(new_op));
                        }
                        else addIntercode(CALL_IC, 2, place, addFuncOperand(temp1->name));
                    }
                    else {
                        if (temp1->type->u.function.ret_type->kind == BASIC)
                            addIntercode(CALL_IC, 2, place, addFuncOperand(temp1->name));
                        else {
                            Operand new_op = addOperand(TEMP_ADDR_OP, new_temp());
                            addIntercode(CALL_IC, 2, new_op, addFuncOperand(temp1->name));
                            addIntercode(GET_VALUE_IC, 2, place, copyOperand(new_op));
                        }
                    }
                }
            }
            if (typeEqual(temp1->type, now) == 0){
                sema_error++;
                printf("Error type 9 at Line %d:  Function \"%s", node->lineno, node->child->info);
                printf("\" is not applicable for arguments \"(");
                FieldList pfield = now->u.function.param_info;
                for (int i = 0; i < now->u.function.param_num; ++i){
                    printType(pfield->type);
                    if (i != now->u.function.param_num-1)
                        printf(",");
                }
                printf(")\".\n");
            }
            return temp1->type->u.function.ret_type;
        }
    }
    else if (strcmp(node->child->type, "NOT") == 0){ // NOT EXP
        int label1 = new_label(), label2 = new_label();
        Operand const_0_op = addOperand(CONST_OP, 0);
        Operand const_1_op = addOperand(CONST_OP, 1);
        addIntercode(ASSIGN_IC, 2, copyOperand(place), const_0_op);
        Type type1 = CondProcess(node, label1, label2);
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label1));
        addIntercode(ASSIGN_IC, 2, copyOperand(place), const_1_op);
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label2));
        return type1;
    }
    else if (strcmp(node->child->type, "MINUS") == 0){ // MINUS EXP
        Operand new_op = addOperand(TEMP_OP, new_temp());
        Operand const_0_op = addOperand(CONST_OP, 0);
        Type type1 = ExpProcess(node->child->next, new_op);
        if (type1==NULL || type1->kind!=BASIC){
            sema_error++;
            printf("Error type 7 at Line %d: Use \"-\" not on integer or float", node->lineno);
        }
        if (place->kind==ADDR_OP || place->kind==TEMP_ADDR_OP){
            Operand temp1 = addOperand(TEMP_OP, new_temp());
            addIntercode(MINUS_IC, 3, temp1, const_0_op, copyOperand(new_op));
            addIntercode(GET_ADDR_IC, 2, place, copyOperand(temp1));
        }
        else addIntercode(MINUS_IC, 3, place, const_0_op, copyOperand(new_op));
        return type1;
    }
    else if (strcmp(node->child->type, "LP") == 0){ // LP Exp RP
        return ExpProcess(node->child->next, place);
    }
    else if (strcmp(node->child->next->type, "ASSIGNOP") == 0){ // Exp ASSIGNOP Exp
        TreeNode* leftExp = node->child;
        Type type1; Type type2;
        Operand temp1; Operand temp2;
        if (strcmp(leftExp->child->type, "ID")==0 && findSymbol(leftExp->child->info, 0)!=NULL
            && findSymbol(leftExp->child->info, 0)->type->kind==BASIC) 
            // if Exp1-->ID && type(Exp1)==BASIC, do not generate code for Exp1
            temp1 = addOperand(NULL_OP, 0);
        else temp1 = addOperand(TEMP_ADDR_OP, new_temp());
        type1 = ExpProcess(leftExp, temp1);
        if (type1!=NULL && type1->kind!=BASIC){
            temp2 = addOperand(TEMP_ADDR_OP, new_temp());
            type2 = ExpProcess(leftExp->next->next, temp2);
        }
        else {
            temp2 = addOperand(TEMP_OP, new_temp());
            type2 = ExpProcess(leftExp->next->next, temp2);
        }
        if (typeEqual(type1, type2) == 0){
            sema_error++;
            printf("Error type 5 at Line %d: Type mismatched for assignment.\n", node->lineno);
            return type1;
        }
        else { 
            if (strcmp(leftExp->child->type, "ID")==0 && leftExp->child->next==NULL // ID
             || leftExp->child->next!=NULL && strcmp(leftExp->child->next->type, "LB")==0 // Exp LB Exp RB
             || leftExp->child->next!=NULL && strcmp(leftExp->child->next->type, "DOT")==0){ // Exp DOT ID
                if (type1->kind == ARRAY) // array assign
                    arrayAssign(type1, type2, copyOperand(temp1), copyOperand(temp2));
                else if (type1->kind != BASIC) // struct assign
                    structAssign(type1, type2, copyOperand(temp1), copyOperand(temp2));
                else { // basic type assign
                    Operand ret_op;
                    if (leftExp->child->next != NULL) { // not ID
                        ret_op = copyOperand(temp1);
                        addIntercode(TO_MEM_IC, 2, ret_op, copyOperand(temp2));
                        addIntercode(GET_VALUE_IC, 2, place, copyOperand(ret_op));
                    }
                    else { // ID
                        ret_op = addOperand(VAR_OP, get_varno(leftExp->child->info));
                        addIntercode(ASSIGN_IC, 2, ret_op, copyOperand(temp2));
                        addIntercode(ASSIGN_IC, 2, place, copyOperand(ret_op));
                    }
                }
                return type1;
            }
            else {
                sema_error++;
                printf("Error type 6 at Line %d: The left-hand side of an assignment must be a variable.\n", node->lineno);
                return type1;
            }
        }
    }
    else if (strcmp(node->child->next->type, "AND") == 0
          || strcmp(node->child->next->type, "OR") == 0){ // Exp AND Exp | Exp OR Exp
        int label1 = new_label(), label2 = new_label();
        Operand const_0_op = addOperand(CONST_OP, 0);
        Operand const_1_op = addOperand(CONST_OP, 1);
        addIntercode(ASSIGN_IC, 2, copyOperand(place), const_0_op);
        Type type1 = CondProcess(node, label1, label2);
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label1));
        addIntercode(ASSIGN_IC, 2, copyOperand(place), const_1_op);
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label2));
        return type1;
    }
    else if (strcmp(node->child->next->type, "PLUS") == 0
          || strcmp(node->child->next->type, "MINUS") == 0
          || strcmp(node->child->next->type, "STAR") == 0
          || strcmp(node->child->next->type, "DIV") == 0){ 
    // Exp PLUS Exp | Exp MINUS Exp | Exp STAR Exp | Exp DIV Exp
        TreeNode* leftExp = node->child;
        Operand new_op1 = addOperand(TEMP_OP, new_temp());
        Operand new_op2 = addOperand(TEMP_OP, new_temp());
        Type type1 = ExpProcess(leftExp, new_op1);
        Type type2 = ExpProcess(leftExp->next->next, new_op2);
        if (place->kind==ADDR_OP || place->kind==TEMP_ADDR_OP){
            Operand temp1 = addOperand(TEMP_OP, new_temp());
            if (strcmp(node->child->next->type, "PLUS") == 0)
                addIntercode(PLUS_IC, 3, temp1, copyOperand(new_op1), copyOperand(new_op2));
            else if (strcmp(node->child->next->type, "MINUS") == 0)
                addIntercode(MINUS_IC, 3, temp1, copyOperand(new_op1), copyOperand(new_op2));
            else if (strcmp(node->child->next->type, "STAR") == 0)
                addIntercode(STAR_IC, 3, temp1, copyOperand(new_op1), copyOperand(new_op2));
            else addIntercode(DIV_IC, 3, temp1, copyOperand(new_op1), copyOperand(new_op2));
            addIntercode(GET_ADDR_IC, 1, place, copyOperand(temp1));
        }
        else{
            if (strcmp(node->child->next->type, "PLUS") == 0)
                addIntercode(PLUS_IC, 3, place, copyOperand(new_op1), copyOperand(new_op2));
            else if (strcmp(node->child->next->type, "MINUS") == 0)
                addIntercode(MINUS_IC, 3, place, copyOperand(new_op1), copyOperand(new_op2));
            else if (strcmp(node->child->next->type, "STAR") == 0)
                addIntercode(STAR_IC, 3, place, copyOperand(new_op1), copyOperand(new_op2));
            else addIntercode(DIV_IC, 3, place, copyOperand(new_op1), copyOperand(new_op2));
        }
        Type int_type = (Type)malloc(sizeof(struct Type_));
        int_type->kind = BASIC;
        int_type->u.basic = BASIC_INT;
        Type float_type = (Type)malloc(sizeof(struct Type_));
        float_type->kind = BASIC;
        float_type->u.basic = BASIC_FLOAT;
        if (typeEqual(type1, type2) == 0){
            sema_error++;
            printf("Error type 7 at Line %d: Type mismatched for operands.\n", node->lineno);
            free(int_type);
            free(float_type);
            return NULL;
        }
        else {
            if (typeEqual(type1, int_type)==0 && typeEqual(type1, float_type)==0){
                sema_error++;
                printf("Error type 7 at Line %d: Illegal type for operands.\n", node->lineno);
                free(int_type);
                free(float_type);
                return NULL;
            }
            free(int_type);
            free(float_type);
            return type1;
        }
    }
    else if (strcmp(node->child->next->type, "RELOP") == 0){ // Exp RELOP Exp
        int label1 = new_label(), label2 = new_label();
        Operand const_0_op = addOperand(CONST_OP, 0);
        Operand const_1_op = addOperand(CONST_OP, 1);
        addIntercode(ASSIGN_IC, 2, copyOperand(place), const_0_op);
        Type type1 = CondProcess(node, label1, label2);
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label1));
        addIntercode(ASSIGN_IC, 2, copyOperand(place), const_1_op);
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label2));
        return type1;
    }
    else if (strcmp(node->child->next->type, "LB") == 0){ // Exp LB Exp RB
    /* translate Exp1 LB Exp2 RB
     * t1 = new_temp()
     * t2 = new_temp()
     * t3 = new_temp()
     * code1 = translate_Exp(Exp1, sym_table, t1) // t1 is an address
     * code2 = translate_Exp(Exp2, sym_table, t2) // t2 is a varible
     * code3 = [t3 := t2 * sizeof(Exp1)]
     * if (place is not a basic type) 
     *     code3 = code3 + [place := t1 + t3] // place is an address
     * else {
     *     t4 = new_temp()
     *     code3 = code3 + [t4 := t1 + t3] + [place := *t4] // place is a varible
     * }
     * return code1 + code2 + code3
    */
        int err_t = sema_error;
        Type ret;
        Operand temp1 = addOperand(TEMP_ADDR_OP, new_temp());
        Operand temp2 = addOperand(TEMP_OP, new_temp());
        Operand temp3 = addOperand(TEMP_OP, new_temp());
        Type type1 = ExpProcess(node->child, temp1);
        Type type2 = ExpProcess(node->child->next->next, temp2);
        if (type1->kind != ARRAY){
            sema_error++;
            printf("Error type 10 at Line %d: Use \"[]\" not on an array.\n", node->lineno);
        }
        if (type2->kind!=BASIC || (type2->kind==BASIC && type2->u.basic!=BASIC_INT)){
            sema_error++;
            printf("Error type 12 at Line %d: Array index is not an integer.\n", node->lineno);
        }
        if (err_t != sema_error) return NULL;
        else ret = type1->u.array.array_type;
        addIntercode(STAR_IC, 3, temp3, copyOperand(temp2), addOperand(CONST_OP, sizeofType(ret)));
        if (place->kind==ADDR_OP || place->kind==TEMP_ADDR_OP) 
            addIntercode(PLUS_IC, 3, place, copyOperand(temp1), copyOperand(temp3));
        else {
            Operand temp4 = addOperand(TEMP_ADDR_OP, new_temp());
            addIntercode(PLUS_IC, 3, temp4, copyOperand(temp1), copyOperand(temp3));
            addIntercode(GET_VALUE_IC, 2, place, copyOperand(temp4));
        }
        return ret;
    }
    else if (strcmp(node->child->next->type, "DOT") == 0){ // Exp DOT ID
    /* translate Exp1 DOT ID
     * t1 = new_temp()
     * code1 = translate_Exp(Exp1, sym_table, t1) // t1 is an address
     * if (place is not a basic type) 
     *     code2 = [place := t1 + offset(ID)] // place is an address
     * else {
     *     t2 = new_temp()
     *     code2 = [t2 := t1 + offset(ID)] + [place := *t2] // place is a varible
     * }
     * return code1 + code2
    */
        Operand temp1 = addOperand(TEMP_ADDR_OP, new_temp());
        Type type1 = ExpProcess(node->child, temp1);
        char* field_id = node->child->next->next->info;
        if (type1==NULL || type1->kind!=STRUCTURE){
            sema_error++;
            printf("Error type 13 at Line %d: Illegal use of \".\".\n", node->lineno);
            return NULL;
        }
        else{
            FieldList temp = NULL;
            int offset = 0;
            if (type1->u.structure != NULL){
                if (type1->u.structure->type != NULL)
                    temp = type1->u.structure;
                else {
                    FieldList tempdef = findSymbol(type1->u.structure->name, 0);
                    temp = tempdef->type->u.structure;
                }
            }
            while (temp != NULL){
                if (strcmp(temp->name, field_id) == 0) break;
                offset += temp->type->size;
                temp = temp->next;
            }
            if (temp == NULL){
                sema_error++;
                printf("Error type 14 at Line %d: Non-existent field \"%s\".\n", node->lineno, field_id);
                return NULL;
            }
            if (place->kind==ADDR_OP || place->kind==TEMP_ADDR_OP)
                addIntercode(PLUS_IC, 3, place, copyOperand(temp1), addOperand(CONST_OP, offset));
            else {
                Operand temp2 = addOperand(TEMP_ADDR_OP, new_temp());
                addIntercode(PLUS_IC, 3, temp2, copyOperand(temp1), addOperand(CONST_OP, offset));
                addIntercode(GET_VALUE_IC, 2, place, copyOperand(temp2));
            }
            return temp->type;
        }
    }
}

Type CondProcess(TreeNode* node, int label_true, int label_false){
    if (node->child->next == NULL){ // ID | INT | FLOAT
        Operand temp1 = addOperand(TEMP_OP, new_temp());
        Type ret = ExpProcess(node, temp1);
        addIntercode(IF_GOTO_IC, 4, 
            copyOperand(temp1), addOperand(OPERATOR_OP, 5),
            addOperand(CONST_OP, 0), addOperand(LABEL_OP, label_true));
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label_false));
        return ret;
    }
    else if (strcmp(node->child->type, "ID") == 0){ // ID LP Args RP | ID LP RP
        Operand temp1 = addOperand(TEMP_OP, new_temp());
        Type ret = ExpProcess(node, temp1);
        addIntercode(IF_GOTO_IC, 4, 
            copyOperand(temp1), addOperand(OPERATOR_OP, 5),
            addOperand(CONST_OP, 0), addOperand(LABEL_OP, label_true));
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label_false));
        return ret;
    }
    else if (strcmp(node->child->type, "NOT") == 0){ // NOT EXP
        Type ret = CondProcess(node->child->next, label_false, label_true);
        if (ret==NULL || ret->kind!=BASIC || (ret->kind==BASIC && ret->u.basic!=BASIC_INT)){
            sema_error++;
            printf("Error type 7 at Line %d: Use \"!\" not on integer", node->lineno);
        }
        return ret;
    }
    else if (strcmp(node->child->type, "MINUS") == 0){ // MINUS EXP
        Operand temp1 = addOperand(TEMP_OP, new_temp());
        Type ret = ExpProcess(node, temp1);
        addIntercode(IF_GOTO_IC, 4, 
            copyOperand(temp1), addOperand(OPERATOR_OP, 5),
            addOperand(CONST_OP, 0), addOperand(LABEL_OP, label_true));
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label_false));
        return ret;
    }
    else if (strcmp(node->child->type, "LP") == 0){ // LP Exp RP
        return CondProcess(node->child->next, label_true, label_false);
    }
    else if (strcmp(node->child->next->type, "ASSIGNOP") == 0){ // Exp ASSIGNOP Exp
        Operand temp1 = addOperand(TEMP_OP, new_temp());
        Type ret = ExpProcess(node, temp1);
        addIntercode(IF_GOTO_IC, 4, 
            copyOperand(temp1), addOperand(OPERATOR_OP, 5), 
            addOperand(CONST_OP, 0), addOperand(LABEL_OP, label_true));
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label_false));
        return ret;
    }
    else if (strcmp(node->child->next->type, "AND") == 0
          || strcmp(node->child->next->type, "OR") == 0){ // Exp AND Exp | Exp OR Exp
        TreeNode* leftExp = node->child;
        Type type1;
        Type type2;
        int label1 = new_label();
        if (strcmp(node->child->next->type, "AND") == 0)
            type1 = CondProcess(leftExp, label1, label_false);
        else type1 = CondProcess(leftExp, label_true, label1);
        addIntercode(LABEL_IC, 1, addOperand(LABEL_OP, label1));
        type2 = CondProcess(leftExp->next->next, label_true, label_false);
        Type int_type = (Type)malloc(sizeof(struct Type_));
        int_type->kind = BASIC;
        int_type->u.basic = BASIC_INT;
        if (typeEqual(type1, type2) == 0){
            sema_error++;
            printf("Error type 7 at Line %d: Type mismatched for operands.\n", node->lineno);
            free(int_type);
            return NULL;
        }
        else {
            if (typeEqual(type1, int_type) == 0){
                sema_error++;
                printf("Error type 7 at Line %d: Illegal type for operands.\n", node->lineno);
                free(int_type);
                return NULL;
            }
            free(int_type);
            return type1;
        }
    }
    else if (strcmp(node->child->next->type, "PLUS") == 0
          || strcmp(node->child->next->type, "MINUS") == 0
          || strcmp(node->child->next->type, "STAR") == 0
          || strcmp(node->child->next->type, "DIV") == 0){ 
    // Exp PLUS Exp | Exp MINUS Exp | Exp STAR Exp | Exp DIV Exp
        Operand temp1 = addOperand(TEMP_OP, new_temp());
        Type ret = ExpProcess(node, temp1);
        addIntercode(IF_GOTO_IC, 4, 
            copyOperand(temp1), addOperand(OPERATOR_OP, 5),
            addOperand(CONST_OP, 0), addOperand(LABEL_OP, label_true));
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label_false));
        return ret;
    }
    else if (strcmp(node->child->next->type, "RELOP") == 0){ // Exp RELOP Exp
        TreeNode* leftExp = node->child;
        Operand temp1 = addOperand(TEMP_OP, new_temp());
        Operand temp2 = addOperand(TEMP_OP, new_temp());
        Type type1 = ExpProcess(leftExp, temp1);
        Type type2 = ExpProcess(leftExp->next->next, temp2);
        int relop;
        if (strcmp(node->child->next->info, ">") == 0) relop = 0;
        else if (strcmp(node->child->next->info, "<") == 0) relop = 1;
        else if (strcmp(node->child->next->info, ">=") == 0) relop = 2;
        else if (strcmp(node->child->next->info, "<=") == 0) relop = 3;
        else if (strcmp(node->child->next->info, "==") == 0) relop = 4;
        else relop = 5;
        addIntercode(IF_GOTO_IC, 4, 
            copyOperand(temp1), addOperand(OPERATOR_OP, relop),
            copyOperand(temp2), addOperand(LABEL_OP, label_true));
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label_false));
        Type int_type = (Type)malloc(sizeof(struct Type_));
        int_type->kind = BASIC;
        int_type->u.basic = BASIC_INT;
        Type float_type = (Type)malloc(sizeof(struct Type_));
        float_type->kind = BASIC;
        float_type->u.basic = BASIC_FLOAT;
        if (typeEqual(type1, type2) == 0){
            sema_error++;
            printf("Error type 7 at Line %d: Type mismatched for operands.\n", node->lineno);
            free(int_type);
            free(float_type);
            return NULL;
        }
        else {
            if (typeEqual(type1, int_type)==0 && typeEqual(type1, float_type)==0){
                sema_error++;
                printf("Error type 7 at Line %d: Illegal type for operands.\n", node->lineno);
                free(int_type);
                free(float_type);
                return NULL;
            }
            free(int_type);
            free(float_type);
            return type1;
        }
    }
    else if (strcmp(node->child->next->type, "LB") == 0){ // Exp LB Exp RB
        Operand temp1 = addOperand(TEMP_OP, new_temp());
        Type ret = ExpProcess(node, temp1);
        addIntercode(IF_GOTO_IC, 4, 
            copyOperand(temp1), addOperand(OPERATOR_OP, 5),
            addOperand(CONST_OP, 0), addOperand(LABEL_OP, label_true));
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label_false));
        return ret;
    }
    else if (strcmp(node->child->next->type, "DOT") == 0){ // Exp DOT ID
        Operand temp1 = addOperand(TEMP_OP, new_temp());
        Type ret = ExpProcess(node, temp1);
        addIntercode(IF_GOTO_IC, 4, 
            copyOperand(temp1), addOperand(OPERATOR_OP, 5),
            addOperand(CONST_OP, 0), addOperand(LABEL_OP, label_true));
        addIntercode(GOTO_IC, 1, addOperand(LABEL_OP, label_false));
        return ret;
    }
}

void arrayAssign(Type type1, Type type2, Operand op1, Operand op2){
    if (type1 == NULL) return;
    int max_assign_size = type1->u.array.size;
    if (type1->u.array.size > type2->u.array.size)
        max_assign_size = type2->u.array.size;
    if (type1->u.array.array_type->kind == BASIC){
        for (int i = 0; i < max_assign_size; ++i){
            // addIntercode(MEM_TO_MEM_IC, 2, copyOperand(op1), copyOperand(op2));
            Operand temp_val = addOperand(TEMP_OP, new_temp());
            addIntercode(GET_VALUE_IC, 2, temp_val, copyOperand(op2));
            addIntercode(TO_MEM_IC, 2, copyOperand(op1), copyOperand(temp_val));
            if (i < max_assign_size-1){
                addIntercode(PLUS_IC, 3, copyOperand(op1), copyOperand(op1), addOperand(CONST_OP, 4));
                addIntercode(PLUS_IC, 3, copyOperand(op2), copyOperand(op2), addOperand(CONST_OP, 4));
            }
        }
    }
    else if (type1->u.array.array_type->kind == ARRAY){
        for (int i = 0; i < max_assign_size; ++i){
            arrayAssign(type1->u.array.array_type, type2->u.array.array_type, op1, op2);
            if (i < max_assign_size-1){
                addIntercode(PLUS_IC, 3, copyOperand(op1), copyOperand(op1), 
                    addOperand(CONST_OP, sizeofType(type1->u.array.array_type)));
                addIntercode(PLUS_IC, 3, copyOperand(op2), copyOperand(op2), 
                    addOperand(CONST_OP, sizeofType(type2->u.array.array_type)));
            }
        }
    }
    else {
        for (int i = 0; i < max_assign_size; ++i){
            structAssign(type1->u.array.array_type, type2->u.array.array_type, op1, op2);
            if (i < max_assign_size-1){
                addIntercode(PLUS_IC, 3, copyOperand(op1), copyOperand(op1), 
                    addOperand(CONST_OP, sizeofType(type1->u.array.array_type)));
                addIntercode(PLUS_IC, 3, copyOperand(op2), copyOperand(op2), 
                    addOperand(CONST_OP, sizeofType(type2->u.array.array_type)));
            }
        }
    }
}

void structAssign(Type type1, Type type2, Operand op1, Operand op2){
    if (type1 == NULL) return;
    if (type1->u.structure->type == NULL)
        type1 = findSymbol(type1->u.structure->name, 0)->type;
    if (type2->u.structure->type == NULL)
        type2 = findSymbol(type2->u.structure->name, 0)->type;
    FieldList struct_field1 = type1->u.structure;
    FieldList struct_field2 = type1->u.structure;
    while (struct_field1 != NULL){
        if (struct_field1->type->kind == BASIC){
            // addIntercode(MEM_TO_MEM_IC, 2, copyOperand(op1), copyOperand(op2));
            Operand temp_val = addOperand(TEMP_OP, new_temp());
            addIntercode(GET_VALUE_IC, 2, temp_val, copyOperand(op2));
            addIntercode(TO_MEM_IC, 2, copyOperand(op1), copyOperand(temp_val));
        }
        else if (struct_field1->type->kind == ARRAY)
            arrayAssign(struct_field1->type, struct_field2->type, op1, op2);
        else structAssign(struct_field1->type, struct_field2->type, op1, op2);
        addIntercode(PLUS_IC, 3, copyOperand(op1), copyOperand(op1), addOperand(CONST_OP, struct_field1->type->size));
        addIntercode(PLUS_IC, 3, copyOperand(op2), copyOperand(op2), addOperand(CONST_OP, struct_field2->type->size));
        struct_field1 = struct_field1->next;
        struct_field2 = struct_field2->next;
    }
}

void printType(Type type){
    if (type == NULL) {
        printf("null");
        return;
    }
    switch (type->kind){
        case BASIC: 
            if (type->u.basic == BASIC_INT) printf("int");
            else  printf("float");
            break;
        case ARRAY: 
            printf("array(%d,", type->u.array.size);
            printType(type->u.array.array_type);
            printf(")");
            break;
        case STRUCTURE:{
            if (type->u.structure == NULL) printf("null");
            else {
                if (type->u.structure->type == NULL)
                    printf("struct %s", type->u.structure->name);
                else {
                    printf("anonymous struct");
                    FieldList temp = type->u.structure;
                    if (temp != NULL) printf("  { ");
                    while (temp != NULL){
                        printf("%s: ", temp->name);
                        printType(temp->type);
                        if (temp->next != NULL)
                            printf(" | ");
                        temp = temp->next;
                    }
                    printf(" }");
                }
            }
            break;
        }
        case FUNCTION:{
            printType(type->u.function.ret_type);
            printf(" function of %d params (", type->u.function.param_num);
            FieldList temp = type->u.function.param_info;
            for (int i = 0; i < type->u.function.param_num; ++i){
                printType(temp->type);
                if (temp->name != NULL)
                    printf(" %s", temp->name);
                if (i != type->u.function.param_num-1)
                    printf(",");
                temp = temp->next;
            }
            printf(")");
            break;
        }
        case STRUCTDEF:{
            printf("struct definition");
            FieldList temp = type->u.structure;
            if (temp != NULL) printf("    { ");
            while (temp != NULL){
                printf("%s: ", temp->name);
                printType(temp->type);
                if (temp->next != NULL)
                    printf(" | ");
                temp = temp->next;
            }
            printf(" }");
            break;
        }
    }
}

#ifdef SEMANTIC_DEBUG
void printFieldList(FieldList f) {
    printf("%s: ", f->name);
    printType(f->type);
    printf("\n");
}

void printSymbolTable(){
    for (int i = 0; i < HASH_SIZE; ++i)
        if (symbol_table[i] != NULL){
            FieldList temp = symbol_table[i];
            while(temp != NULL){
                printFieldList(temp);
                temp = temp->next;
            }
        }
}
#endif
%{
    #include "node.h"
    #include "lex.yy.c"

    int yyerror(const char* format);
    int synerror(int lineno, const char* msg);
    int errorline = 0;
%}

%union {
    struct TreeNode* node;
}

%token <node> INT FLOAT ID SEMI COMMA ASSIGNOP RELOP PLUS MINUS STAR 
    DIV AND OR DOT NOT TYPE LP RP LB RB LC RC STRUCT RETURN IF ELSE WHILE

%type <node> Program ExtDefList ExtDef ExtDecList Specifier 
    StructSpecifier OptTag Tag VarDec FunDec VarList ParamDec 
    CompSt StmtList Stmt DefList Def DecList Dec Exp Args

%right ASSIGNOP  
%left OR 
%left AND 
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left DOT LB RB LP RP
%nonassoc LOWER_THAN_ELSE 
%nonassoc ELSE

%%

/* High-level Definitions */
Program : ExtDefList    { $$ = addNode("Program", NULL); addChild($$, 1, $1); root = $$; }
    ;
ExtDefList : ExtDef ExtDefList  { $$ = addNode("ExtDefList", NULL); addChild($$, 2, $1, $2); }
    | /* empty */   { $$=NULL; }
    ;
ExtDef : Specifier ExtDecList SEMI  { $$ = addNode("ExtDef", NULL); addChild($$, 3, $1, $2, $3); }
    | Specifier SEMI    { $$ = addNode("ExtDef", NULL); addChild($$, 2, $1, $2); }
    | Specifier FunDec CompSt   { $$ = addNode("ExtDef", NULL); addChild($$, 3, $1, $2, $3); }
    | Specifier FunDec SEMI   { $$ = addNode("ExtDef", NULL); addChild($$, 3, $1, $2, $3); }
    | error SEMI    { synerror(@1.first_line, "invalid global definition"); }
    | Specifier error    { synerror(@1.first_line, "illegal definition or missing \";\""); }
    ;
ExtDecList : VarDec    { $$ = addNode("ExtDecList", NULL); addChild($$, 1, $1); }
    | VarDec COMMA ExtDecList   { $$ = addNode("ExtDecList", NULL); addChild($$, 3, $1, $2, $3); }
    ;

/* Specifiers */
Specifier : TYPE    { $$ = addNode("Specifier", NULL); addChild($$, 1, $1); }
    | StructSpecifier   { $$ = addNode("Specifier", NULL); addChild($$, 1, $1); }
    ;
StructSpecifier : STRUCT OptTag LC DefList RC   { $$ = addNode("StructSpecifier", NULL); addChild($$, 5, $1, $2, $3, $4, $5); }
    | STRUCT Tag    { $$ = addNode("StructSpecifier", NULL); addChild($$, 2, $1, $2); }
    ;
OptTag : ID    { $$ = addNode("OptTag", NULL); addChild($$, 1, $1); }
    | /* empty */   { $$=NULL; }
    ;
Tag : ID    { $$ = addNode("Tag", NULL); addChild($$, 1, $1); }
    ;

/* Declarators */
VarDec : ID    { $$ = addNode("VarDec", NULL); addChild($$, 1, $1); }
    | VarDec LB INT RB    { $$ = addNode("VarDec", NULL); addChild($$, 4, $1, $2, $3, $4); }
    | VarDec LB error RB    { synerror(@1.first_line, "invalid array index"); }
    ;
FunDec : ID LP VarList RP   { $$ = addNode("FunDec", NULL); addChild($$, 4, $1, $2, $3, $4); }
    | ID LP RP  { $$ = addNode("FunDec", NULL); addChild($$, 3, $1, $2, $3); }
    | ID LP error RP    { synerror(@1.first_line, "function parameters list error"); }
    ;
VarList : ParamDec COMMA VarList    { $$ = addNode("VarList", NULL); addChild($$, 3, $1, $2, $3); }
    | ParamDec  { $$ = addNode("VarList", NULL); addChild($$, 1, $1); }
    ;
ParamDec : Specifier VarDec { $$ = addNode("ParamDec", NULL); addChild($$, 2, $1, $2); }
    ;

/* Statements */
CompSt : LC DefList StmtList RC { $$ = addNode("CompSt", NULL); addChild($$, 4, $1, $2, $3, $4); }
    ;
StmtList : Stmt StmtList    { $$ = addNode("StmtList", NULL); addChild($$, 2, $1, $2); }
    | /* empty */   { $$=NULL; }
    ;
Stmt : Exp SEMI { $$ = addNode("Stmt", NULL); addChild($$, 2, $1, $2); }
    | CompSt    { $$ = addNode("Stmt", NULL); addChild($$, 1, $1); }
    | RETURN Exp SEMI   { $$ = addNode("Stmt", NULL); addChild($$, 3, $1, $2, $3); }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   { $$ = addNode("Stmt", NULL); addChild($$, 5, $1, $2, $3, $4, $5); }
    | IF LP Exp RP Stmt ELSE Stmt   { $$ = addNode("Stmt", NULL); addChild($$, 7, $1, $2, $3, $4, $5, $6, $7); }
    | WHILE LP Exp RP Stmt  { $$ = addNode("Stmt", NULL); addChild($$, 5, $1, $2, $3, $4, $5); }
    | Exp error SEMI    { synerror(@1.first_line, "invalid statement"); }
    | RETURN Exp error SEMI { synerror(@1.first_line, "invalid statement"); }
    | IF LP error RP Stmt %prec LOWER_THAN_ELSE { synerror(@1.first_line, "invalid expression"); }
    | IF LP error RP Stmt ELSE Stmt { synerror(@1.first_line, "invalid expression"); }
    | WHILE LP error RP Stmt    { synerror(@1.first_line, "invalid expression"); }
    ;

/* Local Definitions */
DefList : Def DefList   { $$ = addNode("DefList", NULL); addChild($$, 2, $1, $2); }
    | /* empty */   { $$=NULL; }
    ;
Def : Specifier DecList SEMI    { $$ = addNode("Def", NULL); addChild($$, 3, $1, $2, $3); }
    | error SEMI    { synerror(@1.first_line, "invalid definition or statement"); }
    ;
DecList : Dec   { $$ = addNode("DecList", NULL); addChild($$, 1, $1); }
    | Dec COMMA DecList { $$ = addNode("DecList", NULL); addChild($$, 3, $1, $2, $3); }
    ;
Dec : VarDec    { $$ = addNode("Dec", NULL); addChild($$, 1, $1); }
    | VarDec ASSIGNOP Exp   { $$ = addNode("Dec", NULL); addChild($$, 3, $1, $2, $3); }
    ;

/* Expressions */
Exp : Exp ASSIGNOP Exp  { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp AND Exp   { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp OR Exp    { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp RELOP Exp { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp PLUS Exp  { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp MINUS Exp { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp STAR Exp  { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp DIV Exp   { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | LP Exp RP { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | MINUS Exp { $$ = addNode("Exp", NULL); addChild($$, 2, $1, $2); }
    | NOT Exp   { $$ = addNode("Exp", NULL); addChild($$, 2, $1, $2); }
    | ID LP Args RP { $$ = addNode("Exp", NULL); addChild($$, 4, $1, $2, $3, $4); }
    | ID LP RP  { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp LB Exp RB { $$ = addNode("Exp", NULL); addChild($$, 4, $1, $2, $3, $4); }
    | Exp DOT ID    { $$ = addNode("Exp", NULL); addChild($$, 3, $1, $2, $3); }
    | ID    { $$ = addNode("Exp", NULL); addChild($$, 1, $1); }
    | INT   { $$ = addNode("Exp", NULL); addChild($$, 1, $1); }
    | FLOAT { $$ = addNode("Exp", NULL); addChild($$, 1, $1); }
    | Exp ASSIGNOP error    { synerror(@1.first_line, "invalid expression"); }
    | Exp AND error     { synerror(@1.first_line, "invalid expression"); }   
    | Exp OR error      { synerror(@1.first_line, "invalid expression"); }     
    | Exp RELOP error       { synerror(@1.first_line, "invalid expression"); }           
    | Exp PLUS error    { synerror(@1.first_line, "invalid expression"); } 
    | Exp MINUS error    { synerror(@1.first_line, "invalid expression"); }           
    | Exp STAR error    { synerror(@1.first_line, "invalid expression"); } 
    | Exp DIV error    { synerror(@1.first_line, "invalid expression"); }   
    | LP error RP   { synerror(@1.first_line, "invalid expression"); }
    | MINUS error    { synerror(@1.first_line, "invalid expression"); }  
    | NOT error    { synerror(@1.first_line, "invalid expression"); }    
    | ID LP error RP    { synerror(@1.first_line, "invalid arguments"); }
    | Exp LB error RB   { synerror(@1.first_line, "invalid array index"); }
    ;
Args : Exp COMMA Args   { $$ = addNode("Args", NULL); addChild($$, 3, $1, $2, $3); }
    | Exp   { $$ = addNode("Args", NULL); addChild($$, 1, $1); }
    ;

%%

int yyerror(const char* format){
    // syn_error++;
    // printf("Error type B at Line %d: %s.\n", yylineno, format);
    return 0;
}

int synerror(int lineno, const char* msg){
    syn_error++;
    if (errorline == lineno) 
        return 0;
    printf("Error type B at Line %d: %s.\n", lineno, msg);
    errorline = lineno;
    return 0;
}
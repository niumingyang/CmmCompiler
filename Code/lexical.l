%{
    #include <stdio.h>
    #include "syntax.tab.h"
    #include "node.h"

    extern int yyerror(const char* format);
    extern int synerror(int lineno, const char* msg);
    extern YYLTYPE yylloc;
    int lexerror(const char* msg, const char* errinfo);

    int yycolumn = 1;
    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;
%}
%option yylineno

delim       [ \t\r]
ws          {delim}+
dec         ([1-9][0-9]*)|0
oct         0[0-7]+
hex         0[Xx][0-9a-fA-F]+
int         {dec}|{oct}|{hex}
digit       [0-9]
nfloat      {digit}+\.{digit}+
sfloat      (({digit}*\.{digit}+)|({digit}+\.))[Ee][+-]?{digit}+
float       {nfloat}|{sfloat}
letter      [_a-zA-Z]
id          ({letter})({letter}|{digit})*
errid       ([1-9]{digit}*{id})|(0{digit}+{id})|(0([_a-wA-WyYzZ]({letter}|{digit})*))
erroct      0[0-9a-fA-F]*[89a-fA-F][0-9a-fA-F]*
errhex      0[Xx][0-9a-zA-Z]*[g-zG-Z][0-9a-zA-Z]*
errnfloat   (\.{digit}+)|({digit}+\.)
errsfloat   ({digit}+[eE][+-]?{digit}*)|((({digit}*\.{digit}+)|({digit}+\.))[Ee][+-]?)
errfloat    {errnfloat}|{errsfloat}
other       .

%%

";"             { yylval.node = addNode("SEMI", NULL); return SEMI; }
","             { yylval.node = addNode("COMMA", NULL); return COMMA; }
"="             { yylval.node = addNode("ASSIGNOP", NULL); return ASSIGNOP; }
">"             { yylval.node = addNode("RELOP", ">"); return RELOP; }
"<"             { yylval.node = addNode("RELOP", "<"); return RELOP; }
">="            { yylval.node = addNode("RELOP", ">="); return RELOP; }
"<="            { yylval.node = addNode("RELOP", "<="); return RELOP; }
"=="            { yylval.node = addNode("RELOP", "=="); return RELOP; }
"!="            { yylval.node = addNode("RELOP", "!="); return RELOP; }
"+"             { yylval.node = addNode("PLUS", NULL); return PLUS; }
"-"             { yylval.node = addNode("MINUS", NULL); return MINUS; }
"*"             { yylval.node = addNode("STAR", NULL); return STAR; }
"/"             { yylval.node = addNode("DIV", NULL); return DIV; }
"&&"            { yylval.node = addNode("AND", NULL); return AND; }
"||"            { yylval.node = addNode("OR", NULL); return OR; }
"."             { yylval.node = addNode("DOT", NULL); return DOT; }
"!"             { yylval.node = addNode("NOT", NULL); return NOT; }
int|float       { yylval.node = addNode("TYPE", yytext); return TYPE; }
"("             { yylval.node = addNode("LP", NULL); return LP; }
")"             { yylval.node = addNode("RP", NULL); return RP; }
"["             { yylval.node = addNode("LB", NULL); return LB; }
"]"             { yylval.node = addNode("RB", NULL); return RB; }
"{"             { yylval.node = addNode("LC", NULL); return LC; }
"}"             { yylval.node = addNode("RC", NULL); return RC; }
struct          { yylval.node = addNode("STRUCT", NULL); return STRUCT; }
return          { yylval.node = addNode("RETURN", NULL); return RETURN; }
if              { yylval.node = addNode("IF", NULL); return IF; }
else            { yylval.node = addNode("ELSE", NULL); return ELSE; }
while           { yylval.node = addNode("WHILE", NULL); return WHILE; }
"\n"            { yycolumn = 1; }
{ws}            { /* do nothing */ }
"//"            {   char c = input(); 
                    while (c != '\n' && c != 0) {
                        c = input();
                    } 
                }
"/*"            {   char c = input(); 
                    while (1){
                        if (c == 0) {
                           synerror(yylineno, "unterminated comment");
                           break;
                        }
                        if (c != '*'){
                            c = input();
                        }
                        else {
                            c = input();
                            if (c == '/') break;
                        }
                    }
                }
{int}           { yylval.node = addNode("INT", yytext); return INT; }
{float}         { yylval.node = addNode("FLOAT", yytext); return FLOAT; }
{id}            { yylval.node = addNode("ID", yytext); return ID; }
{errid}         { lexerror(yytext, "invalid id"); }
{erroct}        { lexerror(yytext, "invalid octal number"); }
{errhex}        { lexerror(yytext, "invalid hexadecimal number"); }
{errfloat}      { lexerror(yytext, "invalid floating point number"); }
{other}         { lexerror(yytext, NULL); }
"*/"            { synerror(yylineno, "block comment without start"); }

%%

int lexerror(const char* msg, const char* errinfo){
    syn_error++;
    if (errinfo == NULL)
        printf("Error type A at Line %d: Mysterious character \"%s\".\n", yylineno, msg);
    else printf("Error type A at Line %d: %s \"%s\".\n", yylineno, errinfo, msg);
}
%locations
%define parse.error verbose
%{
    #include <stdio.h>
    #include <stdarg.h>
    #include "node.h"

    int yylex();
    int yyerror(const char *msg);
    extern int yycolumn;

    Node* root;
    Node *create_terminal_node(char* name, char* text);
    Node *create_nterminal_node(char* name, int line,...);
    Node *create_value_node(char* name, SymbolType _type, char* text);
    Node *create_null_node(char* name);

    int error_flag = 0;
    int error_line = 0;
%}

/* declared types */

%union {
    Node* node;
}

/* declared tokens */
%token <node> INT
%token <node> FLOAT
%token <node> ID STRUCT RETURN IF ELSE WHILE TYPE SEMI COMMA
%right <node> ASSIGNOP
%left <node> OR
%left <node> AND
%left <node> RELOP
%left <node> PLUS MINUS
%left <node> STAR DIV
%right <node> HIGHER_THAN_MINUS
%right <node> NOT
%left <node> DOT LP RP LB RB LC RC

/* declared non-terminals */
%type <node> Program ExtDefList ExtDef ExtDecList
%type <node> Specifier StructSpecifier OptTag Tag
%type <node> VarDec FunDec VarList ParamDec
%type <node> CompSt StmtList Stmt
%type <node> DefList Def DecList Dec
%type <node> Exp Args

%%

Program : ExtDefList {
        /*建立root节点*/
        root = create_nterminal_node("Program", @1.first_line, 1, $1);
    }
    ;
ExtDefList : ExtDef ExtDefList {
        $$ = create_nterminal_node("ExtDefList", @1.first_line, 2, $1, $2);
    }
    | /* empty */ {
        $$ = create_null_node("ExtDefList");
    }
    ;
ExtDef : Specifier ExtDecList SEMI {
        $$ = create_nterminal_node("ExtDef", @1.first_line, 3, $1, $2, $3);
    }
    | Specifier SEMI {
        $$ = create_nterminal_node("ExtDef", @1.first_line, 2, $1, $2);
    }
    | Specifier FunDec CompSt {
        $$ = create_nterminal_node("ExtDef", @1.first_line, 3, $1, $2, $3);
    }
    | Specifier ExtDecList error {
        yyerrok;
    }
    | Specifier error SEMI{
        yyerrok;
    }
    | Specifier error CompSt{ 
        yyerrok; 
    }
    | error ExtDef{

    }
    | error CompSt{ 
        yyerrok; 
    }
    | error SEMI{ 
        yyerrok; 
    }
    ;
ExtDecList : VarDec {
        $$ = create_nterminal_node("ExtDecList" , @1.first_line, 1, $1);
    }
    | VarDec COMMA ExtDecList {
        $$ = create_nterminal_node("ExtDecList" , @1.first_line, 3, $1, $2, $3);
    }
    ;

Specifier : TYPE {
        $$ = create_nterminal_node("Specifier" , @1.first_line, 1, $1);
    }
    | StructSpecifier {
        $$ = create_nterminal_node("Specifier" , @1.first_line, 1, $1);
    }
    ;
StructSpecifier : STRUCT OptTag LC DefList RC {
        $$ = create_nterminal_node("StructSpecifier" , @1.first_line, 5, $1, $2, $3, $4, $5);
    }
    | STRUCT Tag {
        $$ = create_nterminal_node("StructSpecifier" , @1.first_line, 2, $1, $2);
    }
    ;
OptTag : ID {
        $$ = create_nterminal_node("OptTag" , @1.first_line, 1, $1);
    }
    | /* empty */ {
        $$ = create_null_node("OptTag");
    }
    ;
Tag : ID {
        $$ = create_nterminal_node("Tag" , @1.first_line, 1, $1);
    }
    ;

VarDec : ID {
        $$ = create_nterminal_node("VarDec" , @1.first_line, 1, $1);
    }
    | VarDec LB INT RB {
        $$ = create_nterminal_node("VarDec" , @1.first_line, 4, $1, $2, $3, $4);
    }
    ;
FunDec : ID LP VarList RP {
        $$ = create_nterminal_node("FunDec" , @1.first_line, 4, $1, $2, $3, $4);
    }
    | ID LP RP {
        $$ = create_nterminal_node("FunDec" , @1.first_line, 3, $1, $2, $3);
    }
    ;
VarList : ParamDec COMMA VarList {
        $$ = create_nterminal_node("VarList" , @1.first_line, 3, $1, $2, $3);
    }
    | ParamDec {
        $$ = create_nterminal_node("VarList" , @1.first_line, 1, $1);
    }
    ;
ParamDec : Specifier VarDec {
        $$ = create_nterminal_node("ParamDec" , @1.first_line, 2, $1, $2);
    }
    ;

CompSt : LC DefList StmtList RC {
        $$ = create_nterminal_node("CompSt" , @1.first_line, 4, $1, $2, $3, $4);
    }
    ;
StmtList : Stmt StmtList {
        $$ = create_nterminal_node("StmtList" , @1.first_line, 2, $1, $2);
    }
    | /* empty */ {
        $$ = create_null_node("StmtList");
    }
    ;
Stmt : Exp SEMI {
        $$ = create_nterminal_node("Stmt" , @1.first_line, 2, $1, $2);
    }
    | CompSt {
        $$ = create_nterminal_node("Stmt" , @1.first_line, 1, $1);
    }
    | RETURN Exp SEMI {
        $$ = create_nterminal_node("Stmt" , @1.first_line, 3, $1, $2, $3);
    }
    | IF LP Exp RP Stmt {
        $$ = create_nterminal_node("Stmt" , @1.first_line, 5, $1, $2, $3, $4, $5);
    }
    | IF LP Exp RP Stmt ELSE Stmt {
        $$ = create_nterminal_node("Stmt" , @1.first_line, 7, $1, $2, $3, $4, $5, $6, $7);
    }
    | WHILE LP Exp RP Stmt {
        $$ = create_nterminal_node("Stmt" , @1.first_line, 5, $1, $2, $3, $4, $5);
    }
    ;

DefList : Def DefList {
        $$ = create_nterminal_node("DefList" , @1.first_line, 2, $1, $2);
    }
    | /* empty */ {
        $$ = create_null_node("DefList");
    }
    ;
Def : Specifier DecList SEMI {
        $$ = create_nterminal_node("Def" , @1.first_line, 3, $1, $2, $3);
    }
    ;
DecList : Dec {
        $$ = create_nterminal_node("DecList" , @1.first_line, 1, $1);
    }
    | Dec COMMA DecList {
        $$ = create_nterminal_node("DecList" , @1.first_line, 3, $1, $2, $3);
    }
    ;
Dec  : VarDec {
        $$ = create_nterminal_node("Dec" , @1.first_line, 1, $1);
    }
    | VarDec ASSIGNOP Exp {
        $$ = create_nterminal_node("Dec" , @1.first_line, 3, $1, $2, $3);
    }
    ;

Exp : Exp ASSIGNOP Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp AND Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp OR Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp RELOP Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp PLUS Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp MINUS Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp STAR Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp DIV Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | LP Exp RP {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | MINUS Exp %prec HIGHER_THAN_MINUS {
        $$ = create_nterminal_node("Exp" , @1.first_line, 2, $1, $2);
    }
    | NOT Exp {
        $$ = create_nterminal_node("Exp" , @1.first_line, 2, $1, $2);
    }
    | ID LP Args RP {
        $$ = create_nterminal_node("Exp" , @1.first_line, 4, $1, $2, $3, $4);
    }
    | ID LP RP {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp LB Exp RB {
        $$ = create_nterminal_node("Exp" , @1.first_line, 4, $1, $2, $3, $4);
    }
    | Exp DOT ID {
        $$ = create_nterminal_node("Exp" , @1.first_line, 3, $1, $2, $3);
    }
    | ID {
        $$ = create_nterminal_node("Exp" , @1.first_line, 1, $1);
    }
    | INT  {
        $$ = create_nterminal_node("Exp" , @1.first_line, 1, $1);
    }
    | FLOAT  {
        $$ = create_nterminal_node("Exp" , @1.first_line, 1, $1);
    }
    ;
Args : Exp COMMA Args {
        $$ = create_nterminal_node("Args" , @1.first_line, 3, $1, $2, $3);
    }
    | Exp {
        $$ = create_nterminal_node("Args" , @1.first_line, 1, $1);
    }
    ;

%%

#include "lex.yy.c"
//产生式名，类型，行号，参数
//type 包含，INT类型，FLOAT类型，ID类型，非终结符类型，EMPTY类型。
//...中第一个参数为余下的参数数量。

Node *create_value_node(char* name, SymbolType _type, char* text){
    Node* ret = calloc(1,sizeof(Node));
    ret->name = name;
    ret->type = _type;

    switch(_type) {
    case INT_DEC__:
        sscanf(text, "%d", &(ret->val.type_int));
        break;
    case INT_HEX__:
        sscanf(text, "%xd", &(ret->val.type_int));
        break;
    case INT_OCT__:
        sscanf(text, "%od", &(ret->val.type_int));
        break;
    case FLOAT__:
        sscanf(text, "%f", &(ret->val.type_float));
        break;
    default:
        ;
    }
    return ret;
}

Node *create_null_node(char* name){
    Node* ret = calloc(1,sizeof(Node));
    ret->name = name;
    ret->type = NULL__;
    ret->val.type_str = NULL;
    return ret;
}

Node *create_terminal_node(char* name, char* text){
    Node* ret = calloc(1,sizeof(Node));
    ret->name = name;
    ret->type = TERMINAL__;
    ret->val.type_str = (char *)malloc(strlen(text)*2*sizeof(char));
    strcpy(ret->val.type_str, text);
    return ret;
}

Node *create_nterminal_node(char* name, int line, ...){
    Node* ret = calloc(1,sizeof(Node));
    ret->name = name;
    ret->type = NTERMINAL__;

    va_list args;
    va_start(args, line);

    ret->line = line;
    int son_num = va_arg(args, int);
    ret->child_num = son_num;
    ret->children = (Node **)malloc((son_num+1)*sizeof(Node*));

    for(int i = 0; i < son_num; i++) {
        ret->children[i] = (struct Node *)va_arg(args, Node*);
    }

    va_end(args);
    return ret;
}

int yyerror(const char *msg) {
    error_flag = 1;
    if(error_line)return 0;
    error_line=1;
    if (msg[0] == 's' && msg[1] == 'y') {
        printf("Error type B at Line %d: %s. caused by \"%s\" \n", yylineno, msg, yytext);
    }
    //else printf(" %s.\n", msg);
    return 0;
}



void print(Node *node, int depth) {
    if(NULL__ == node->type) return; 
    for(int i = 0; i < depth; i++) {
        printf("  ");
    }
    switch(node->type)
    {
        case INT_DEC__: case INT_HEX__: case INT_OCT__: {
            printf("%s: %d\n", node->name, node->val.type_int);
            break;
        }
        case FLOAT__: {
            printf("%s: %f\n", node->name, node->val.type_float);
            break;
        }
        case ID__:  case TYPE__: {
            printf("%s: %s\n", node->name, node->val.type_str);
            break;
        }
        case TERMINAL__: {
            printf("%s\n", node->name);
            break;
        }
        case NTERMINAL__: {
            printf("%s (%d)\n", node->name, node->line);
            for(int i = 0; i<node->child_num ;i++)
            print((Node *)(node->children[i]), depth + 1);
            break;
        }
        default: {
            break;
        }
    }
}
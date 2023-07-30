%{
    #include "syntax.tab.h"

    int yycolumn = 1;
    extern int error_line;

    #define YY_USER_ACTION \
        yylloc.first_line = yylloc.last_line = yylineno; \
        yylloc.first_column = yycolumn; \
        yylloc.last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;


    int test = 0;

%}

%option yylineno

digit [0-9]
letter [_a-zA-Z]
int 0|([1-9]{digit}*)
float {int}"."{digit}+

id ([_A-Za-z])([_A-Za-z0-9])*


oct 0([0-7]+)
hex 0(x|X)[0-9a-fA-F]{1,8}
erroct 0([0-9]+)
errhex 0(x|X)[0-9a-zA-Z]{1,8}

%%
[ \t] {}
[\n\r] { yycolumn = 1; error_line = 0;}

"struct" {
    yylval.node = create_terminal_node("STRUCT", yytext);
    return STRUCT;
}
"return" {
    yylval.node = create_terminal_node("RETURN", yytext);
    return RETURN;
}
"if" {
    yylval.node = create_terminal_node("IF", yytext);
    return IF;
}
"else" {
    yylval.node = create_terminal_node("ELSE", yytext);
    return ELSE;
}
"while" {
    yylval.node = create_terminal_node("WHILE", yytext);
    return WHILE;
}

";" {
    if(test)printf("SEMI at Line %d: \'%s\'\n", yylineno, yytext);
    yylval.node = create_terminal_node("SEMI", yytext);
    return SEMI;
}
"," {
    yylval.node = create_terminal_node("COMMA", yytext);
    return COMMA;
}
"=" {
    yylval.node = create_terminal_node("ASSIGNOP", yytext);
    return ASSIGNOP;
}
">"|"<"|">="|"<="|"=="|"!=" {
    yylval.node = create_terminal_node("RELOP", yytext);
    return RELOP;
}
"+" {
    yylval.node = create_terminal_node("PLUS", yytext);
    return PLUS;
}
"-" {
    yylval.node = create_terminal_node("MINUS", yytext);
    return MINUS;
}
"*" {
    yylval.node = create_terminal_node("STAR", yytext);
    return STAR;
}
"/" {
    yylval.node = create_terminal_node("DIV", yytext);
    return DIV;
}
"&&" {
    yylval.node = create_terminal_node("AND", yytext);
    return AND;
}
"||" {
    yylval.node = create_terminal_node("OR", yytext);
    return OR;
}
"." {
    yylval.node = create_terminal_node("DOT", yytext);
    return DOT;
}
"!" {
    yylval.node = create_terminal_node("NOT", yytext);
    return NOT;
}
"(" {
    yylval.node = create_terminal_node("LP", yytext);
    return LP;
}
")" {
    yylval.node = create_terminal_node("RP", yytext);
    return RP;
}
"[" {
    yylval.node = create_terminal_node("LB", yytext);
    return LB;
}
"]" {
    if(test)printf("RB at Line %d: \'%s\'\n", yylineno, yytext);
    yylval.node = create_terminal_node("RB", yytext);
    return RB;
}
"{" {
    yylval.node = create_terminal_node("LC", yytext);
    return LC;
}
"}" {
    yylval.node = create_terminal_node("RC", yytext);
    return RC;
}

"int"|"float" {
    if(test)printf("TYPE at Line %d: \'%s\'\n", yylineno, yytext);
    yylval.node = create_terminal_node("TYPE", yytext);
    yylval.node->type = TYPE__;
    return TYPE;
}

{int} {
    if(test)printf("INT at Line %d: \'%s\'\n", yylineno, yytext);
    yylval.node = create_value_node("INT", INT_DEC__, yytext);
    return INT;
}

{oct} {
    yylval.node = create_value_node("INT", INT_OCT__, yytext);
    return INT;
}



{hex} {
    yylval.node = create_value_node("INT", INT_HEX__, yytext);
    return INT;
}

{float} {
    if(test)printf("FLOAT at Line %d: \'%s\'\n", yylineno, yytext);

    yylval.node = create_value_node("FLOAT", FLOAT__, yytext);
    return FLOAT;
}

{id} {
    if(test)printf("ID at Line %d: \'%s\'\n", yylineno, yytext); 
    yylval.node = create_terminal_node("ID", yytext);
    yylval.node->type = ID__;
    return ID;
}

{digit}{id} {
    error_flag = 1;
    if(!error_line)printf("Error type A at Line %d: Illegal identifier \'%s\'\n", yylineno, yytext);
    yylval.node = create_terminal_node("ID", yytext);
    yylval.node->type = ID__;
    error_line = yylineno;
    return ID;
}

{erroct} {
    error_flag = 1;
    if(!error_line)printf("Error type A at Line %d: Illegal number \'%s\'\n", yylineno, yytext);
    yylval.node = create_value_node("INT", INT_OCT__, yytext);
    error_line = 1;
    return INT;
}

{errhex} {
    error_flag = 1;
    if(!error_line)printf("Error type A at Line %d: Illegal number \'%s\'\n", yylineno, yytext);
    yylval.node = create_value_node("INT", INT_HEX__, yytext);
    error_line = 1;
    return INT;
}

. {
    error_flag = 1;
    printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext);
    error_line = 1;
}

%%

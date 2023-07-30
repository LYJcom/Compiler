#include <stdio.h>

#include "interCode.h"

extern FILE* yyin;
extern int error_flag;
extern struct Node *root;

FILE* output;

void print(struct Node *node, int depth);

int yyrestart();
int yyparse();

int main(int argc, char** argv) {
    if (argc <= 1) {
        return 1;
    }
    FILE* f = fopen(argv[1], "r");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    
    yyrestart(f);
    yyparse();


    if(!error_flag) {
        //print(root, 0);
    }
    
    hash_init();
    // 添加 read & write
    add_read();
    add_write();
    Analysis(root);

    genIR(root);
    if(argc > 1) {
        output = fopen(argv[2], "w+");
    }
    printIc();

    fclose(f);
    if(argc > 1) {
        fclose(output);
    }
    return 0;
}
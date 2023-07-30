#include "node.h"

typedef struct Operand_* Operand;
struct Operand_
{
    enum { VARIABLE, CONSTANT, ADDRESS, FUNCCALL, TEMPVAR, LABEL,
        BLOCK, // 开空间大小
        ADDR, //取址（&nums）
        DEREFERENCE, // 解引用（*addr)
    } kind;
    union {
        char* varName;
        int value;

    } u;
};

struct InterCode
{
    enum {
        ADD, SUB, MUL, DIV,
        ASSIGN,
        FUNCDEC,
        PARAMDEC,
        RETURN,
        IFGOTO,
        GOTO,
        LABELDEC,
        ARG,
        DEC,
        READ,
        WRITE,

    } kind;
    union {
        struct {
            Operand op;
        } unop; // + - * LABEL GOTO
        struct {
            Operand right, left;
        } assign; // ASSIGN
        struct {
            Operand result, op1, op2;
        } binop; // ADD SUB ...
        struct {
            Operand op1, op2, target;
            char* relop;
        } ternop; // IFGOTO
    } u;
};

typedef struct InterCodes* IRList;

struct InterCodes
{
    struct InterCode code;
    struct InterCodes *prev, *next;
};

void genIR(Node *node);

Operand newOperand(int op_kind, ...);
void newInterCode(int ic_kind, ...);

void printIc();
void printOp(Operand op);

void transProgram(Node *node);
void transExtDefList(Node *node);
void transExtDef(Node *node);
void transFunDec(Node *node);
void transVarList(Node* node);
void transParamDec(Node* node);
void transCompSt(Node *node);
void transStmtList(Node *node);
void transStmt(Node *node);
void transDefList(Node *node);
void transDef(Node *node);
void transDecList(Node *node);
void transDec(Node *node);
Operand transVarDec(Node *node);
Operand transExp(Node *node);
void transArgs(Node *node);
int getSize(Type type);
int getOffset(Node* node, Type type);

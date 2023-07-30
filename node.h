#include<string.h>
typedef enum {
        INT_DEC__,
        INT_HEX__,
        INT_OCT__,
        FLOAT__,
        ID__,
        TYPE__,
        TERMINAL__,
        NTERMINAL__,
        NULL__
} SymbolType;

typedef struct Node Node;

struct Node {
    char *name;
    SymbolType type;
    int line;
    union {
        int         type_int;
        float       type_float;
        char*       type_str;
    } val;
    int child_num;
    struct Node **children;
};

typedef struct Type_lab2* Type;
typedef struct Symbol_lab2* Symbol;
typedef struct FieldList_lab2* FieldList;
typedef struct Func_lab2* Func;
typedef struct Struc_lab2* Struc;

struct Type_lab2{
    enum {BASIC, FUNC, STRUC, ARRAY} first;
    union{
        enum {TYPE_INT,TYPE_FLOAT} basic;
        struct {int base; int num; Type elem;} array;
        Struc structure;
        Func function;
    }second;
    int size;//大小
};

struct Symbol_lab2{
    char* name;
    Type type;
    Symbol next;
    int is_addr;
}; 

struct FieldList_lab2
{
    char* name; // 作用域名
    Type type; // 类型
    FieldList next; // 下一个域
};

struct Func_lab2
{
    char* name; // 函数名
    Type ret; // 返回类型
    FieldList parameters; // 参数列表
};

struct Struc_lab2
{
    char* name; // 结构体名
    FieldList next; // 域列表
};

void hash_add(Symbol sym);

void Analysis(Node* node);
void ExtDecList(Node* node, Type type);
Type Specifier(Node* node);
Type StructSpecifier(Node* node);
char* Tag(Node* node);
FieldList VarDec(Node* node, Type type, Struc struc);
int FunDec(Node* node, Type type);
void VarList(Node* node, Func f);
FieldList ParamDec(Node* node);
void Stmt(Node* node);
void DefList(Node* node, Struc struc);
void Def(Node* node, Struc struc);
void Dec(Node* node, Type type, Struc struc);
Type Exp(Node* node);
FieldList Args(Node* node);

int is_same(Type a, Type b);
Type Tmaker_A(Type elem, int num);
Type Tmaker_S(Struc s);
Type Tmaker_F(Func f);
Type Tmaker_B(int b);
FieldList Fmaker(char* name, Type type, FieldList next);
Symbol Smaker(char* name, Type type);
Func Funcmaker(char* name, Type type, FieldList next);
void add_to_func(Func f, Node* node);
void print_error(int idx, int line, char* msg);
FieldList get_member(Struc struc, char* ID);
char* random_name();
void prt_test(Node* node);
void delete_type(Type type);
Struc Strucmaker(char* name, FieldList next);

void hash_init();
Symbol hash_check(char* name);

void add_read();
void add_write();

int initSize(Type type);

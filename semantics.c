#include<stdio.h>
#include<string.h>
#include<stdarg.h>
#include<stdlib.h>
#include "node.h"

#define hash_size 16384
#define test_1 0
#define test_2 0
#define field_test 0
#define hash_test 0
#define alloc_test 0
Symbol hashTable[hash_size];

int alloc_cnt = 0;
static Type func_ret;

unsigned int hash_pjw(char* name)
{
    unsigned int val = 0, i;
    for (; *name; ++name)
    {
        val = (val << 2) + *name;
        if (i = val & ~0x3fff) val = (val ^ (i >> 12)) & 0x3fff;
    }
    return val;
}

void hash_init()
{
    for(int i = 0; i < hash_size; i++){
        hashTable[i] = NULL;
    }
}

void hash_add(Symbol sym)
{
    if(hash_test)printf("add %s to %d \n",sym->name, hash_pjw(sym->name));
    unsigned int index = hash_pjw(sym->name);
    sym->next = hashTable[index];
    hashTable[index] = sym;
}

Symbol hash_check(char* name)
{
    if(hash_test)printf("check %s at %d \n",name, hash_pjw(name));
    unsigned int index = hash_pjw(name);
    Symbol trav = hashTable[index];
    while(trav != NULL && strcmp(trav->name,name)){
        trav = trav->next;
    }
    if(hash_test) if(trav == NULL) printf("Not found %s\n",name);
    return trav;
}

int syntax_judge(Node* node, int n, ...){
    if(node->child_num != n) return 0;
    va_list args;
    va_start(args, n);
    for(int i = 0; i < n; i++) {
        if(strcmp(node->children[i]->name,va_arg(args, char*))) return 0;
    }
    va_end(args);
    return 1;
}

void Analysis(Node* node){
    if(node == NULL) return;
    if(test_1)prt_test(node);
    if(!strcmp(node->name,"Program")){
        if (syntax_judge(node, 1, "ExtDefList")) {
            Analysis(node->children[0]);
        }
    }
    else if(!strcmp(node->name,"ExtDefList")){
        if (syntax_judge(node, 2, "ExtDef", "ExtDefList")) // ExtDef ExtDefList
        {
            Analysis(node->children[0]);
            Analysis(node->children[1]);
        }
    }
    else if(!strcmp(node->name,"ExtDef")){
        if (syntax_judge(node, 3, "Specifier", "ExtDecList", "SEMI")) // Specifier ExtDecList SEMI
        {
            Type type = Specifier(node->children[0]);
            ExtDecList(node->children[1], type);
        }
        if (syntax_judge(node, 2, "Specifier", "SEMI")) // Specifier SEMI
        {
            Type type = Specifier(node->children[0]);
        }
        if (syntax_judge(node, 3, "Specifier", "FunDec", "CompSt")) // Specifier FunDec CompSt
        {
            Type type = Specifier(node->children[0]);
            int succ = FunDec(node->children[1], type);
            
            func_ret = type;
            Analysis(node->children[2]);
        }
    }
    else if(!strcmp(node->name,"ExtDefList")){
        if (syntax_judge(node, 2, "ExtDef", "ExtDefList")) // ExtDef ExtDefList
        {
            Analysis(node->children[0]);
            Analysis(node->children[1]);
        }
    }
    else if(!strcmp(node->name,"CompSt")){
        if(syntax_judge(node, 4, "LC", "DefList", "StmtList", "RC")){ // LC DefList StmtList RC
            DefList(node->children[1], NULL);
            Analysis(node->children[2]);
        }
    }
    else if(!strcmp(node->name,"StmtList")){
        if(syntax_judge(node, 2, "Stmt", "StmtList")){ // Stmt StmtList
            Stmt(node->children[0]);
            Analysis(node->children[1]);
        }
    }
}

void ExtDecList(Node* node, Type type)
{
    if(node == NULL) return;
    if(test_1)prt_test(node);
    if (syntax_judge(node, 1, "VarDec")) // VarDec
    {
        VarDec(node->children[0], type, NULL);
    }
    if (syntax_judge(node, 3, "VarDec", "COMMA", "ExtDecList")) //VarDec COMMA ExtDecList
    {
        VarDec(node->children[0], type, NULL);
        ExtDecList(node->children[2], type);
    }
}

Type Specifier(Node* node)
{
    if(node == NULL) return NULL;
    if(test_1)prt_test(node);
    Type type = NULL;
    if(syntax_judge(node, 1, "TYPE")){ // TYPE
        if(!strcmp(node->children[0]->val.type_str, "int")){
            type = Tmaker_B(0);
        }
        if(!strcmp(node->children[0]->val.type_str, "float")){
            type = Tmaker_B(1);
        }
    }
    if(syntax_judge(node, 1, "StructSpecifier")){ // StructSpecifier
        type = StructSpecifier(node->children[0]);
    }
    return type;
}

Type StructSpecifier(Node* node)
{
    if(node == NULL) return NULL;
    if(test_1)prt_test(node);
    Type type = NULL;
    Struc struc = NULL;
    Symbol symb = NULL;
    if(syntax_judge(node, 5, "STRUCT", "OptTag", "LC", "DefList", "RC")){ // STRUCT OptTag LC DefList RC
        char* opt_tag = Tag(node->children[1]);
        if(opt_tag != NULL){
            if(hash_check(opt_tag) != NULL){
                /*error 16*/
                print_error(16,node->line,opt_tag);
                return NULL;
            }
        }
        else opt_tag = random_name();
        struc = Strucmaker(opt_tag,NULL);
        DefList(node->children[3],struc);
        type = Tmaker_S(struc);
        symb = Smaker(opt_tag,type);
        hash_add(symb);
    }
    if(syntax_judge(node, 2, "STRUCT", "Tag")){ // STRUCT Tag
        char* tag = Tag(node->children[1]);
        symb = hash_check(tag);
        if(symb == NULL || symb->type->first != STRUC){
            /*error 17*/
            print_error(17,node->line,tag);
            return NULL;
        }
    }
    return symb->type;
}

char* Tag(Node* node)
{
    if(test_1)prt_test(node);
    if(node == NULL) return NULL; // epsilon
    if(syntax_judge(node, 1, "ID")){ // ID
        return node->children[0]->val.type_str;
    }
    else {return NULL;}
}

FieldList VarDec(Node* node, Type type, Struc struc)//变量的创建
{
    if(test_1)prt_test(node);
    if(node == NULL) return NULL;
    FieldList field = NULL;
    Symbol Symb = NULL;
    if(syntax_judge(node, 1, "ID")){ // ID
        char* ID = node->children[0]->val.type_str;
        if(struc == NULL)//全局变量 and 函数参数
        {
            if(hash_check(ID) != NULL){
                /*error 3*/
                print_error(3,node->line,ID);
            }
            else{
                field = Fmaker(ID, type, NULL);
                Symb = Smaker(ID, type);
                hash_add(Symb);
            }
            //return NULL;
        }
        else{//结构体变量
            if(get_member(struc,ID) != NULL){
                /*error 15*/
                print_error(15,node->line,ID);
            }
            else{
                field = Fmaker(ID, type, NULL);
                if(struc->next == NULL){
                    struc->next = field;
                }
                else{
                    FieldList trav = struc -> next;
                    while(trav->next != NULL) trav = trav->next;
                    trav->next = field;
                }
                Symb = Smaker(ID, type);
                hash_add(Symb);
            }
        }
    }
    if(syntax_judge(node, 4, "VarDec", "LB", "INT", "RB")){ // VarDec LB INT RB
        /*数组*/
        Type array_type = Tmaker_A(type,node->children[2]->val.type_int);
        return VarDec(node->children[0], array_type, struc);
    }
    return field;
}

int FunDec(Node* node, Type type)//函数的创建
{
    if(test_1)prt_test(node);
    if(node == NULL) return 0;
    Symbol symb = NULL;
    Type t = NULL;
    Func f = NULL;
    int flag = 1;
    char* ID = node->children[0]->val.type_str;
    if(hash_check(ID) != NULL){
        /*error 4*/
        print_error(4,node->line,ID);
        flag = 0;
    }
    if(syntax_judge(node, 4, "ID", "LP", "VarList", "RP")){ // ID LP VarList RP
        f = Funcmaker(ID,type,NULL);
        VarList(node->children[2], f);
        t = Tmaker_F(f);
        symb = Smaker(ID,t);
        hash_add(symb);
    }
    if(syntax_judge(node, 3, "ID", "LP", "RP")){ // ID LP RP
        f = Funcmaker(ID,type,NULL);
        t = Tmaker_F(f);
        symb = Smaker(ID,t);
        hash_add(symb);
    }
    return flag;
}

void VarList(Node* node, Func func)
{
    if(test_1)prt_test(node);
    if(node == NULL || func == NULL) return;
    if(syntax_judge(node, 3, "ParamDec", "COMMA", "VarList")){ // ParamDec COMMA VarList
        FieldList arg = ParamDec(node->children[0]);
        FieldList trav = func->parameters;
        if(trav == NULL) func->parameters = arg;
        else{
            while(trav->next != NULL) trav = trav->next;
            trav->next = arg;
        }
        VarList(node->children[2],func);
    }
    else if(syntax_judge(node, 1, "ParamDec")){ // ParamDec
        FieldList arg = ParamDec(node->children[0]);
        FieldList trav = func->parameters;
        if(trav == NULL) func->parameters = arg;
        else{
            while(trav->next != NULL) trav = trav->next;
            trav->next = arg;
        }
    }
}

FieldList ParamDec(Node* node)
{
    if(test_1)prt_test(node);
    if(node == NULL) return NULL;
    if(syntax_judge(node, 2, "Specifier", "VarDec")){ // Specifier VarDec
        Type type = Specifier(node->children[0]);
        if(type == NULL) return NULL;
        return VarDec(node->children[1], type, NULL);
    }
    return NULL;
}

void Stmt(Node* node)
{
    if(test_1)prt_test(node);
    if(node == NULL) return;
    Type ret = NULL;
    if(syntax_judge(node, 1, "CompSt")){ // CompSt
        Analysis(node->children[0]);
    }
    if(syntax_judge(node, 2, "Exp", "SEMI")){ // Exp SEMI
        ret = Exp(node->children[0]);
    }
    
    if(syntax_judge(node, 3, "RETURN", "Exp", "SEMI")){ // RETURN Exp SEMI
        ret = Exp(node->children[1]);
        if(ret != NULL && !is_same(ret,func_ret)){
            /*error 8*/
            print_error(8,node->line,"");
        }
    }
    if(syntax_judge(node, 5, "IF", "LP", "Exp", "RP", "Stmt")){ // IF LP Exp RP Stmt
        ret = Exp(node->children[2]);
        if(ret != NULL &&( ret->first != BASIC || ret->second.basic != TYPE_INT)){
            /*error 7*/
            print_error(7,node->line,"");
        }
        Stmt(node->children[4]);
    }
    if(syntax_judge(node, 7, "IF", "LP", "Exp", "RP", "Stmt", "ELSE", "Stmt")){ // IF LP Exp RP Stmt ELSE Stmt
        ret = Exp(node->children[2]);
        if(ret != NULL &&( ret->first != BASIC || ret->second.basic != TYPE_INT)){
            /*error 7*/
            print_error(7,node->line,"");
        }
        Stmt(node->children[4]);
        Stmt(node->children[6]);
    }
    if(syntax_judge(node, 5, "WHILE", "LP", "Exp", "RP", "Stmt")){ // WHILE LP Exp RP Stmt
        ret = Exp(node->children[2]);
        if(ret != NULL &&( ret->first != BASIC || ret->second.basic != TYPE_INT)){
            /*error 7*/
            print_error(7,node->line,"");
        }
        Stmt(node->children[4]);
    }
}

void DefList(Node* node, Struc struc)
{
    if(test_1)prt_test(node);
    if(node == NULL) return;
    if(syntax_judge(node, 2, "Def", "DefList")){ // Def DefList
        Def(node->children[0],struc);
        DefList(node->children[1],struc);
    }                   
}

void Def(Node* node, Struc struc)
{
    if(test_1)prt_test(node);
    if(node == NULL) return;
    if(syntax_judge(node, 3, "Specifier", "DecList", "SEMI")){ // Specifier DecList SEMI
        Type type = Specifier(node->children[0]);
        if(type != NULL) Dec(node->children[1],type,struc);
    }
}

void Dec(Node* node, Type type, Struc struc)
{
    if(node == NULL) return;
    if(test_1)prt_test(node);
    FieldList left = NULL;
    if(!strcmp(node->name,"DecList")){
        if(syntax_judge(node, 1, "Dec")){ // Dec
            Dec(node->children[0],type,struc);
        }
        if(syntax_judge(node, 3, "Dec", "COMMA", "DecList")){ // Dec COMMA DecList
            Dec(node->children[0],type,struc);
            Dec(node->children[2],type,struc);
        }
    }
    else if(!strcmp(node->name,"Dec")){
        if(syntax_judge(node, 1, "VarDec")){ // VarDec
            left = VarDec(node->children[0], type, struc);
        }
        if(syntax_judge(node, 3, "VarDec", "ASSIGNOP", "Exp")){ // VarDec ASSIGNOP Exp
            if (struc != NULL) { //结构体中的成员变量不能初始化
                left = VarDec(node->children[0], type, struc);
                print_error(15,node->line,"");
                /*error 15*/
            } 
            else{//全局变量初始化检查两边类型是否相等
                FieldList left = VarDec(node->children[0], type, struc);
                Type right = Exp(node->children[2]);
                if((left != NULL && right != NULL) && !is_same(left->type,right)){
                    /*error 5*/
                    print_error(5,node->line,"");
                }
            }
        }
    }
}

Type Exp(Node* node)
{
    if(test_1)prt_test(node);
    if(node == NULL) return NULL;
    Type type = NULL;
    Symbol res = NULL;
    if(syntax_judge(node, 3, "Exp", "ASSIGNOP", "Exp"))
    {
        Type left = Exp(node->children[0]);
        Type right = Exp(node->children[2]);
        /*error 6*/
        if(!syntax_judge(node->children[0],1,"ID")
        && !syntax_judge(node->children[0],3,"Exp","DOT","ID")
        && !syntax_judge(node->children[0], 4, "Exp", "LB", "Exp", "RB"))
        {
            print_error(6,node->line,"");
        }
        if((left != NULL && right != NULL) && !is_same(left,right)){
            /*error 5*/
            print_error(5,node->line,"");
        }
        type = left;
    }
    if(syntax_judge(node, 3, "Exp", "PLUS", "Exp")
    || syntax_judge(node, 3, "Exp", "MINUS", "Exp")
    || syntax_judge(node, 3, "Exp", "STAR", "Exp")
    || syntax_judge(node, 3, "Exp", "DIV", "Exp")){
        Type left = Exp(node->children[0]);
        Type right = Exp(node->children[2]);
        if(left != NULL && right != NULL && !is_same(left,right)){
            /*error 7*/
            print_error(7,node->line,"");
            type = NULL;
        }
        else type = left;
    }
    if(syntax_judge(node, 3, "Exp", "AND", "Exp")
    || syntax_judge(node, 3, "Exp", "OR", "Exp")){
        Type left = Exp(node->children[0]);
        Type right = Exp(node->children[2]);
        if(left != NULL && right != NULL 
        && (!is_same(left,right) || left->first!= BASIC || left->second.basic != TYPE_INT)){
            /*error 7*/
            print_error(7,node->line,"");
            type = NULL;
        }
        else type = Tmaker_B(0);
    }
    if(syntax_judge(node, 3, "Exp", "RELOP", "Exp")){
        Type left = Exp(node->children[0]);
        Type right = Exp(node->children[2]);
        if(left != NULL && right != NULL && !is_same(left,right)){
            /*error 7*/
            print_error(7,node->line,"");
            type = NULL;
        }
        else type = Tmaker_B(0);
    }

    if(syntax_judge(node, 3, "LP", "Exp", "RP")){
        type = Exp(node->children[1]);
    }
    if(syntax_judge(node, 2, "MINUS", "Exp")){
        type = Exp(node->children[1]);
        if(type != NULL && (type->first != BASIC)){
            /*error 7*/
            print_error(7,node->line,"");
            type = NULL;
        }
        else {
            if(type->second.basic == TYPE_INT) type = Tmaker_B(0);
            if(type->second.basic == TYPE_FLOAT) type = Tmaker_B(1);
        }
    } 
    if(syntax_judge(node, 2, "NOT", "Exp")){
        type = Exp(node->children[1]);
        if(type != NULL && (type->first != BASIC || type->second.basic != TYPE_INT)){
            /*error 7*/
            print_error(7,node->line,"");
            type = NULL;
        }
        else type = Tmaker_B(0);
    }
    if(syntax_judge(node, 4, "ID", "LP", "Args", "RP")){
        /*func*/
        char* ID = node->children[0]->val.type_str;
        res = hash_check(ID);
        if(res == NULL){
            /*error 2*/
            print_error(2,node->line,ID);
        }
        else if(res->type->first != FUNC){
            /*error 11*/
            print_error(11,node->line,ID);
        }
        else 
        {
            FieldList args = Args(node->children[2]);
            Func func = Funcmaker(random_name(),res->type->second.function->ret,args);
            if(!is_same(Tmaker_F(func),res->type)){
                /*error 9*/
                FieldList p = func->parameters;
                while(p!=NULL){
                    if(p->type == NULL) return NULL;
                    p = p->next;
                }
                print_error(9,node->line,ID);
            }
        }
        if(res != NULL && res->type->first == FUNC){
            type = res->type->second.function->ret;
        }
    }
    if(syntax_judge(node, 3, "ID", "LP", "RP")){
        char* ID = node->children[0]->val.type_str;
        res = hash_check(ID);
        if(res == NULL){
            /*error 2*/
            print_error(2,node->line,ID);
        }
        else if(res->type->first != FUNC){
            /*error 11*/
            print_error(11,node->line,ID);
        }
        else 
        {
            if(res->type->second.function->parameters!=NULL){
                /*error 9*/
                print_error(9,node->line,"");
            }
        }

        if(res != NULL && res->type->first == FUNC){
            type = res->type->second.function->ret;
        }
    }
    if(syntax_judge(node, 4, "Exp", "LB", "Exp", "RB")){
        /*array*/
        Type t = Exp(node->children[0]);
        if(t != NULL){
            if(t->first != ARRAY){
                print_error(10,node->line,"");
            }
            else{
                type = t->second.array.elem;
                
            }
        }
        t = Exp(node->children[2]);
        if(t != NULL &&( t->first != BASIC || t->second.basic != TYPE_INT )){
            print_error(12, node->line,"");
        }
    }
    if(syntax_judge(node, 3, "Exp", "DOT", "ID")){
        /*struct*/
        Type t = Exp(node->children[0]);
        if(t != NULL){
                if(test_2)printf("check!!!\n");
            if(t->first != STRUC){
                print_error(13, node->line, "");
                return NULL;
            }
            else{
                char* ID = node->children[2]->val.type_str;
                FieldList member =  get_member(t->second.structure,ID);
                if(member == NULL){
                    print_error(14, node->line, ID);
                }
                else{
                    type = member->type;
                }
            }
        }
    }
    if(syntax_judge(node, 1, "ID")){
        char* ID = node->children[0]->val.type_str;
        res = hash_check(ID);
        if(res == NULL){
            /*error 1*/
            if(test_2)printf("ID is %s ",ID);
            print_error(1, node->line, ID);
            return NULL;
        }
        else type = res->type;
    }
    if(syntax_judge(node, 1, "INT")){ 
        //printf("getint at %d\n",node->line);
        type = Tmaker_B(0);
    }
    if(syntax_judge(node, 1, "FLOAT")){
        type = Tmaker_B(1);
    }
    return type;
}

FieldList Args(Node* node)
{
    
    if(test_2)printf("add arg\n");
    if(test_1)prt_test(node);
    if(node == NULL) return NULL;
    FieldList args =  NULL;
    if(syntax_judge(node, 3, "Exp", "COMMA", "Args")){ // Exp COMMA Args
        Type type = Exp(node->children[0]);
        args =  Fmaker("arg",type,NULL);
        args->next = Args(node->children[2]);
    }
    if(syntax_judge(node, 1, "Exp")){ // Exp
        Type type = Exp(node->children[0]);
        args =  Fmaker("arg",type,NULL);

    }
    return args;
}

int is_same(Type a, Type b)
{
    if(a == NULL && b == NULL){
        return 1;
    } 
    if(a == NULL || b == NULL){
        return 0;
    }
    if(a->first != b->first){
        return 0;
    }
    if(a->first == BASIC){
        if(test_2)printf("%d vs %d\n",a->second.basic,b->second.basic);
        return a->second.basic == b->second.basic;
    }
    if(a->first == ARRAY){
        return is_same(a->second.array.elem,b->second.array.elem);
    }
    if(a->first == STRUC){
        //printf("struc compare!\n");
        FieldList sa = a->second.structure->next;
        FieldList sb = b->second.structure->next;
        while(sa != NULL || sb != NULL){
            if(sa == NULL && sb == NULL) return 1;
            if(sa == NULL || sb == NULL) return 0;
            if(!is_same(sa->type,sb->type)) return 0;
            sa = sa->next;
            sb = sb->next;
        }
    }
    if(a->first == FUNC){
        //if(test_2)printf("func compare!\n");
        Func fa = a->second.function;
        Func fb = b->second.function;
        if(!is_same(fa->ret,fb->ret)) return 0;
        FieldList sa = fa->parameters;
        FieldList sb = fb->parameters;
        if(test_2)printf("%s vs %s \n",fa->name,fb->name);
        while(sa != NULL || sb != NULL){
            if(sa == NULL || sb == NULL) return 0;
            if(!is_same(sa->type,sb->type)) return 0;
            sa = sa->next;
            sb = sb->next;
        }
    }
    if(test_2)printf("is same.\n");
    return 1;
}

Type Tmaker_A(Type elem,int num)
{
    Type ret = (Type)malloc(sizeof(struct Type_lab2));
    if(alloc_test) printf("alloc_cnt = %d\n", ++alloc_cnt);
    ret->first = ARRAY;
    ret->second.array.elem = elem;
    ret->second.array.num = num;
    ret->size = 0;
    initSize(ret);
    return ret;
}

Type Tmaker_S(Struc s)
{
    Type ret = (Type)malloc(sizeof(struct Type_lab2));
    if(alloc_test) printf("alloc_cnt = %d\n", ++alloc_cnt);
    ret->first = STRUC;
    ret->second.structure = s;
    ret->size = 0;
    initSize(ret);
    return ret;
}

Type Tmaker_F(Func f)
{
    Type ret = (Type)malloc(sizeof(struct Type_lab2));
    if(alloc_test) 
    if(alloc_test) printf("alloc_cnt = %d\n", ++alloc_cnt);
    ret->first = FUNC;
    ret->second.function = f;
    ret->size = 0;
    initSize(ret);
    return ret;
}

Type Tmaker_B(int b)
{
    Type ret = (Type)malloc(sizeof(struct Type_lab2));
    if(alloc_test) printf("alloc_cnt = %d\n", ++alloc_cnt);
    ret->first = BASIC;
    ret->second.basic = b;
    ret->size = 0;
    initSize(ret);
    return ret;
}
 
FieldList Fmaker(char* name, Type type, FieldList next)
{
    FieldList ret = (FieldList)malloc(sizeof(struct FieldList_lab2));
    if(alloc_test) printf("alloc_cnt = %d\n", ++alloc_cnt);
    ret->name = name;
    ret->type = type;
    ret->next = next;
    return ret;
}

Struc Strucmaker(char* name, FieldList next){
    Struc ret = (Struc)malloc(sizeof(struct Struc_lab2));
    ret->name = name;
    ret->next = next;
    return ret;
}

Symbol Smaker(char* name, Type type)
{
    Symbol ret = (Symbol)malloc(sizeof(struct Symbol_lab2));
    if(alloc_test) printf("alloc_cnt = %d\n", ++alloc_cnt);
    ret->name = name;
    ret->type = type;
    ret->next = NULL;
    return ret;
}

Func Funcmaker(char* name, Type type, FieldList next)
{
    Func ret = (Func)malloc(sizeof(struct Func_lab2));
    if(alloc_test) printf("alloc_cnt = %d\n", ++alloc_cnt);
    ret->name = name;
    ret->ret = type;
    ret->parameters = next;
    return ret;
}

void print_error(int idx, int line, char* msg){
    printf("Error type %d at Line %d: ",idx ,line);
    if(idx == 1){printf("Undefined variable \"%s\".", msg);}
    if(idx == 2){printf("Undefined function \"%s\".", msg);}
    if(idx == 3){printf("Redefined variable \"%s\".", msg);}
    if(idx == 4){printf("Redefined function \"%s\".", msg);}
    if(idx == 5){printf("Type mismatched for assignment.");}
    if(idx == 6){printf("The left-hand side of an assignment must be a variable.");}
    if(idx == 7){printf("Type mismatched for operands.");}
    if(idx == 8){printf("Type mismatched for return.");}
    if(idx == 9){printf("Mismatch argument type");}
    if(idx == 10){printf("\"%s\" is not an array.", msg);}
    if(idx == 11){printf("\"%s\" is not a function.", msg);}
    if(idx == 12){printf("\"%s\" is not a integer.", msg);}
    if(idx == 13){printf("Illegal use of \".\".");}
    if(idx == 14){printf("Non-existent field \"%s\".", msg);}
    if(idx == 15){printf("Redefined field \"%s\".", msg);}
    if(idx == 16){printf("Duplicated name \"%s\".", msg);}
    if(idx == 17){printf(" Undefined structure \"%s\".", msg);}
    printf("\n");
}

FieldList get_member(Struc struc, char* ID)
{
    if(field_test) printf("search %s in %s !\n",ID,struc->name);
    FieldList f = struc->next;
    while(f!=NULL && strcmp(f->name,ID)){
        f = f->next;
    }
    return f;
}

char* random_name(){
    char* name = calloc(30, sizeof(char));
    static int seed = 0;
    seed++;
    sprintf(name, "RAND%d", seed);
    return name;
}

void prt_test(Node* node){
    printf("%d : %s \n",node->line,node->name);
}

void delete_type(Type type)
{
    if(type == NULL) return;
    switch(type->first){
        case ARRAY:
            delete_type(type->second.array.elem);
            break;
        case STRUC:{
            FieldList field = type->second.structure->next;
            while(field != NULL) {
                FieldList pre_field = field;
                delete_type(field->type);
                field = field->next;
                free(pre_field);
            }
            break;
        }
        case FUNC: {
            FieldList field = type->second.function->parameters;
            delete_type(type->second.function->ret);
            while(field != NULL) {
                FieldList pre_field = field;
                delete_type(field->type);
                field = field->next;
                free(pre_field);
            }
            break;
        }
        case BASIC: default:
            break;
    }
    free(type);
}

void add_read()
{
    Func f = Funcmaker("read",Tmaker_B(0),NULL);
    Type t = Tmaker_F(f);
    Symbol symb = Smaker("read",t);
    hash_add(symb);
}

void add_write()
{
    Func f = Funcmaker("write",Tmaker_B(0),NULL);

    FieldList arg = Fmaker("write_arg", Tmaker_B(0), NULL);
    Symbol arg_symb = Smaker("write_arg", Tmaker_B(0));
    hash_add(arg_symb);
    f->parameters = arg;

    Type t = Tmaker_F(f);
    Symbol symb = Smaker("write",t);
    hash_add(symb);
}

int initSize(Type type) {
    if(type->size != 0) return type->size;
    int cnt = 0;
    switch(type->first) {
    case STRUC: {
        Struc st = type->second.structure;
        FieldList fieldList = st->next;
        while(fieldList != NULL) {
            cnt += initSize(fieldList->type);
            fieldList = fieldList->next;
        }
        break;
    }
    case ARRAY: {
        Type kind = type->second.array.elem;
        cnt = initSize(kind) * type->second.array.num;
        break;
    }
    default:
        cnt++;
        break;
    }
    type->size = cnt;
    return cnt;
}
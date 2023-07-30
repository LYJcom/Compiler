#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include "interCode.h"

extern FILE* output;

IRList head = NULL, tail = NULL;
int tmpCnt = 0;
int labelCnt = 0;

void listInsert(IRList node) {
    if(!head) {
        head = node;
        tail = node;
        node->prev = NULL;
        node->next = NULL;
        return;
    }
    node->prev = tail;
    node->next = NULL;
    tail->next = node;
    tail = node;
}

Operand newOperand(int op_kind, ...) {
    va_list args;
    va_start(args, op_kind);
    
    Operand op = (Operand)(malloc(sizeof(struct Operand_)));
    op->kind = op_kind;
    switch(op_kind) {
    case VARIABLE:
    case FUNCCALL:
    case ADDR:
        op->u.varName = va_arg(args, char*);
        break;
    case TEMPVAR:
    case CONSTANT:
    case LABEL:
    case BLOCK:
    case DEREFERENCE:
        op->u.value = va_arg(args, int);
        break;
    default:
        break;
    }
    return op;
}

void newInterCode(int ic_kind, ...) {
    va_list args;
    va_start(args, ic_kind);
    IRList interCode = (IRList)(malloc(sizeof(struct InterCodes)));
    interCode->code.kind = ic_kind;

    switch (ic_kind) {
    case ADD: case SUB: case MUL: case DIV: // x := y + z —— newInterCode(ADD, x, y, z);
        interCode->code.u.binop.result = va_arg(args, Operand);
        interCode->code.u.binop.op1 = va_arg(args, Operand);
        interCode->code.u.binop.op2 = va_arg(args, Operand);
        break;
    case ASSIGN: // x := y —— newInterCode(ASSIGN, x, y);
    case DEC: // DEC nums 8 —— left(nums) & right(8)
        interCode->code.u.assign.left = va_arg(args, Operand);
        interCode->code.u.assign.right = va_arg(args, Operand);
        break;
    case FUNCDEC:
    case PARAMDEC:
    case RETURN:
    case ARG:
    case READ:
    case WRITE:
        interCode->code.u.unop.op = va_arg(args, Operand);
        break;
    case IFGOTO:
        interCode->code.u.ternop.op1 = va_arg(args, Operand);
        interCode->code.u.ternop.op2 = va_arg(args, Operand);
        interCode->code.u.ternop.target = va_arg(args, Operand);
        interCode->code.u.ternop.relop = va_arg(args, char*);
        break;
    case GOTO:
        interCode->code.u.unop.op = va_arg(args, Operand);
        break;
    case LABELDEC:
        interCode->code.u.unop.op = va_arg(args, Operand);
        break;

    default:
        break;
    }

    listInsert(interCode);
}

void genIR(Node *node) {
    
    transProgram(node);

}

void transProgram(Node *node) {
    if(!node) return;
    transExtDefList(node->children[0]);
}

void transExtDefList(Node *node) {
    if(!node || !node->child_num) return;
    // ExtDefList -> ExtDef ExtDefList
    // | empty
    transExtDef(node->children[0]);
    transExtDefList(node->children[1]);
}

void transExtDef(Node *node) {
    if(!node || !node->child_num) return;
    // ExtDefList -> ExtDef ExtDefList (无全局变量)
    // | Specifier SEMI (无中间代码)
    // | Specifier FunDec CompSt .
    // | Specifier FunDec SEMI (无函数声明)
    Node *funDec = node->children[1];
    if(!strcmp(funDec->name, "FunDec")) {
        Node *compSt = node->children[2];
        if(!strcmp(compSt->name, "CompSt")) {
            transFunDec(funDec);
            transCompSt(compSt);
        }
    }
}

void transFunDec(Node *node) {
    if(!node || !node->child_num) return;
    // FunDec -> ID LP VarList RP .
    // | ID LP RP .
    Node *id = node->children[0];
    Operand funcOp = newOperand(VARIABLE, id->val.type_str);
    newInterCode(FUNCDEC, funcOp);
    if(node->child_num == 4) { // 函数含参
        Node *param = node->children[2];
        transVarList(node->children[2]);
    }
}

void transVarList(Node* node) {
    if(!node || !node->child_num) return;
    // VarList -> ParamDec COMMA VarList
    // | ParamDec
    transParamDec(node->children[0]);
    if(node->child_num == 3) {
        transVarList(node->children[2]);
    }
}

void transParamDec(Node* node) {
    if(!node || !node->child_num) return;
    // ParamDec -> Specifier VarDec
    Node *param = node->children[1];
    if(param->child_num != 1) { // ban 数组参数
        printf("Cannot translate: Code contains variables of multi-dimensional array type or parameters of array type.\n");
        exit(-1);
    }
    Operand var = newOperand(VARIABLE, param->children[0]->val.type_str);
    newInterCode(PARAMDEC, var);
    Symbol symb = hash_check(param->children[0]->val.type_str);
    symb->is_addr = 1;
}

void transCompSt(Node *node) {
    if(!node || !node->child_num) return;
    // CompSt -> LC DefList StmtList RC
    transDefList(node->children[1]);
    transStmtList(node->children[2]);
}

void transStmtList(Node *node) {
    if(!node || !node->child_num) return;
    // StmtList -> Stmt StmtList
    // | empty
    transStmt(node->children[0]);
    transStmtList(node->children[1]);
}

void transStmt(Node *node) {
    if(!node || !node->child_num) return;
    // Stmt -> Exp SEMI .
    // | CompSt .
    // | RETURN Exp SEMI .
    // | IF LP Exp RP Stmt .
    // | IF LP Exp RP Stmt ELSE Stmt .
    // | WHILE LP Exp RP Stmt .
    if(!strcmp(node->children[0]->name, "IF")) {
        // if(x) stmt1 else stmt2 
        // IF x != 0 GOTO L1
        // GOTO L3
        // LABEL L1 :
        // stmt1
        // GOTO L2（无 else 则省略）
        // LABEL L3 :（无 else 则省略）
        // stmt2（无 else 则省略）
        // LABEL L2

        // if(x) stmt1
        // IF x != 0 GOTO L1
        // GOTO L2
        // LABEL L1 :
        // stmt1
        // LABEL L2
        Operand f = newOperand(CONSTANT, 0);
        Operand t1 = transExp(node->children[2]);
        Operand l1 = newOperand(LABEL, ++labelCnt);
        Operand l2 = newOperand(LABEL, ++labelCnt);
        newInterCode(IFGOTO, t1, f, l1, "!=");
        if(node->child_num == 5) {
            newInterCode(GOTO, l2);
            newInterCode(LABELDEC, l1);
            transStmt(node->children[4]);
        }
        else {
            Operand l3 = newOperand(LABEL, ++labelCnt);
            newInterCode(GOTO, l3);
            newInterCode(LABELDEC, l1);
            transStmt(node->children[4]);
            newInterCode(GOTO, l2);
            newInterCode(LABELDEC, l3);
            transStmt(node->children[6]);
        }
        newInterCode(LABELDEC, l2);
        return;
    }

    if(!strcmp(node->children[0]->name, "WHILE")) {
        // while(x) stmt
        // LABEL L1:
        // IF x != 0 GOTO L2
        // GOTO L3
        // LABEL L2 :
        // stmt
        // GOTO L1
        // LABEL L3:
        // ...
        Operand f = newOperand(CONSTANT, 0);
        Operand l1 = newOperand(LABEL, ++labelCnt);
        Operand l2 = newOperand(LABEL, ++labelCnt);
        Operand l3 = newOperand(LABEL, ++labelCnt);
        newInterCode(LABELDEC, l1);
        Operand t1 = transExp(node->children[2]);
        newInterCode(IFGOTO, t1, f, l2, "!=");
        newInterCode(GOTO, l3);
        newInterCode(LABELDEC, l2);
        transStmt(node->children[4]);
        newInterCode(GOTO, l1);
        newInterCode(LABELDEC, l3);
        return;
    }

    switch(node->child_num) {
    case 1:
        transCompSt(node->children[0]);
        break;
    case 2:
        transExp(node->children[0]);
        break;
    case 3:
        newInterCode(RETURN, transExp(node->children[1]));
    default:
        break;
    }
}

void transDefList(Node *node) {
    if(!node || !node->child_num) return;
    // DefList -> Def DefList
    // | empty
    transDef(node->children[0]);
    transDefList(node->children[1]);
}

void transDef(Node *node) {
    if(!node || !node->child_num) return;
    // Def -> Specifier DecList SEMI
    transDecList(node->children[1]);
}

void transDecList(Node *node) {
    if(!node || !node->child_num) return;
    // DecList -> Dec
    // | Dec COMMA DecList
    transDec(node->children[0]);
    if(node->child_num == 3) {
        transDecList(node->children[2]);
    }
}

void transDec(Node *node) {
    if(!node || !node->child_num) return;
    // Dec -> VarDec
    // | VarDec ASSIGNOP Exp
    Operand t1 = transVarDec(node->children[0]);
    if(node->child_num == 3) { // 注意 read 函数的处理
        if(!strcmp(node->children[2]->children[0]->name, "ID") && !strcmp(node->children[2]->children[0]->val.type_str, "read")) {
            Operand param = transExp(node->children[0]);
            newInterCode(READ, param);
            return;
        }
        Operand t2 = transExp(node->children[2]);
        newInterCode(ASSIGN, t1, t2);
    }
}

/*
    1. 查询 id 是否为结构体类型
    2. 若为结构体，则 DEC 空间
*/
Operand transVarDec(Node *node) {
    if(!node || !node->child_num) return NULL;
    // VarDec -> ID
    // | VarDec LB INT RB
    Operand op = NULL;
    if(node->child_num == 1) {
        op = newOperand(VARIABLE, node->children[0]->val.type_str);
        Type type = Exp(node);
        if(type->first == STRUC) {
            Struc st = type->second.structure;
            FieldList fieldList = st->next;
            int cnt = getSize(type);
            Operand num = newOperand(BLOCK, cnt);
            newInterCode(DEC, op, num);
        }
    }
    else { //数组
        if(node->children[0]->child_num != 1) { // ban 高维数组
            printf("Cannot translate: Code contains variables of multi-dimensional array type or parameters of array type.\n");
            exit(-1);
        }
        op = transVarDec(node->children[0]);
        Type array = hash_check(op->u.varName)->type;
        int size = getSize(array->second.array.elem);
        Operand num = newOperand(BLOCK, size * node->children[2]->val.type_int);
        newInterCode(DEC, op, num);

        // 在 Type 中记录数组的长度，以供后续计算数组空间时使用
        // array->second.array.num = node->children[2]->val.type_int;
    }
    
    return op;
}

Operand transExp(Node *node) {
    if(!node || !node->child_num) return NULL;
    // Exp -> Exp ASSIGNOP Exp .
    // | Exp AND Exp（非 IFGOTO）.
    // | Exp OR Exp（非 IFGOTO）.
    // | Exp RELOP Exp（非 IFGOTO）.
    // | Exp PLUS Exp .
    // | Exp MINUS Exp .
    // | Exp STAR Exp .
    // | Exp DIV Exp .
    // | LP Exp RP .
    // | MINUS Exp .
    // | NOT Exp .
    // | ID LP Args RP（含参函数调用）.
    // | ID LP RP .
    // | Exp LB Exp RB（数组取值）.
    // | Exp DOT ID（结构体取成员）
    // | ID .
    // | INT .
    // | FLOAT （无浮点数）
    Operand op = NULL;
    if(node->child_num == 1) { // ID
        if(node->children[0]->type == INT_DEC__) {
            op = newOperand(CONSTANT, node->children[0]->val.type_int);
        }
        else if(node->children[0]->type == ID__) {
            op = newOperand(VARIABLE, node->children[0]->val.type_str);
        }
        return op;
    }
    else if(node->child_num == 2) {
        if(!strcmp(node->children[0]->name, "MINUS")) { // Exp -> MINUS Exp
            Operand zero = newOperand(CONSTANT, 0);
            Operand t1 = transExp(node->children[1]);
            op = newOperand(TEMPVAR, ++tmpCnt);
            newInterCode(SUB, op, zero, t1);
            return op;
        }
        else { // Exp -> NOT Exp
            // int x = !a;
            // IF a != 0 GOTO L1
            // GOTO L2
            // LABEL L1 :
            // t1 := #0
            // GOTO L3
            // LABEL L2 :
            // t1 := #1
            // LABEL L3 :
            // x := t1
            Operand f = newOperand(CONSTANT, 0);
            Operand t = newOperand(CONSTANT, 1);
            op = newOperand(TEMPVAR, ++tmpCnt);
            Operand t1 = transExp(node->children[1]);
            Operand l1 = newOperand(LABEL, ++labelCnt);
            Operand l2 = newOperand(LABEL, ++labelCnt);
            Operand l3 = newOperand(LABEL, ++labelCnt);
            newInterCode(IFGOTO, t1, f, l1, "!=");
            newInterCode(GOTO, l2);
            newInterCode(LABELDEC, l1);
            newInterCode(ASSIGN, op, f);
            newInterCode(GOTO, l3);
            newInterCode(LABELDEC, l2);
            newInterCode(ASSIGN, op, t);
            newInterCode(LABELDEC, l3);
            return op;
        }
    }
    else if(node->child_num == 3) {
        if(!strcmp(node->children[0]->name, "LP")) { // Exp -> LP Exp RP
            return transExp(node->children[1]);
        }
        else if(!strcmp(node->children[0]->name, "ID")) { // 无参函数调用：Exp -> ID LP RP; CALL ID即可
            op = newOperand(TEMPVAR, ++tmpCnt);
            newInterCode(ASSIGN, op, newOperand(FUNCCALL, node->children[0]->val.type_str));
            return op;
        }
        else if(!strcmp(node->children[1]->name, "RELOP")) { // Exp -> Exp RELOP Exp
            // int x = a > b;
            // IF a > b GOTO L1
            // GOTO L2
            // LABEL L1 :
            // t1 := #1
            // GOTO L3
            // LABEL L2 :
            // t1 := #0
            // LABEL L3 :
            // x := t1
            Operand f = newOperand(CONSTANT, 0);
            Operand t = newOperand(CONSTANT, 1);
            op = newOperand(TEMPVAR, ++tmpCnt);
            Operand t1 = transExp(node->children[0]);
            Operand t2 = transExp(node->children[2]);
            Operand l1 = newOperand(LABEL, ++labelCnt);
            Operand l2 = newOperand(LABEL, ++labelCnt);
            Operand l3 = newOperand(LABEL, ++labelCnt);
            newInterCode(IFGOTO, t1, t2, l1, node->children[1]->val.type_str);
            newInterCode(GOTO, l2);
            newInterCode(LABELDEC, l1);
            newInterCode(ASSIGN, op, t);
            newInterCode(GOTO, l3);
            newInterCode(LABELDEC, l2);
            newInterCode(ASSIGN, op, f);
            newInterCode(LABELDEC, l3);
            return op;
        }
        else if(!strcmp(node->children[1]->name, "OR")) { // Exp -> Exp OR Exp
            // int x = a || b;
            // IF a != #0 GOTO L1
            // GOTO L2
            // LABEL L2 :
            // IF b != #0 GOTO L1
            // GOTO L3
            // LABEL L1 :
            // t1 := #1
            // GOTO L4
            // LABEL L3 :
            // t1 := #0
            // LABEL L4 :
            // x := t1
            Operand f = newOperand(CONSTANT, 0);
            Operand t = newOperand(CONSTANT, 1);
            op = newOperand(TEMPVAR, ++tmpCnt);
            Operand t1 = transExp(node->children[0]);
            
            Operand l1 = newOperand(LABEL, ++labelCnt);
            Operand l2 = newOperand(LABEL, ++labelCnt);
            Operand l3 = newOperand(LABEL, ++labelCnt);
            Operand l4 = newOperand(LABEL, ++labelCnt);
            newInterCode(IFGOTO, t1, f, l1, "!=");
            newInterCode(GOTO, l2);
            newInterCode(LABELDEC, l2);
            Operand t2 = transExp(node->children[2]);
            newInterCode(IFGOTO, t2, f, l1, "!=");
            newInterCode(GOTO, l3);
            newInterCode(LABELDEC, l1);
            newInterCode(ASSIGN, op, t);
            newInterCode(GOTO, l4);
            newInterCode(LABELDEC, l3);
            newInterCode(ASSIGN, op, f);
            newInterCode(LABELDEC, l4);
            return op;
        }
        else if(!strcmp(node->children[1]->name, "AND")) { // Exp -> Exp AND Exp
            // int x = a && b;
            // IF a != #0 GOTO L1
            // GOTO L2
            // LABEL L1 :
            // IF b != #0 GOTO L3
            // GOTO L2
            // LABEL L3 :
            // t1 := #1
            // GOTO L4
            // LABEL L2 :
            // t1 := #0
            // LABEL L4 :
            // x := t1
            Operand f = newOperand(CONSTANT, 0);
            Operand t = newOperand(CONSTANT, 1);
            op = newOperand(TEMPVAR, ++tmpCnt);
            Operand t1 = transExp(node->children[0]);
            
            Operand l1 = newOperand(LABEL, ++labelCnt);
            Operand l2 = newOperand(LABEL, ++labelCnt);
            Operand l3 = newOperand(LABEL, ++labelCnt);
            Operand l4 = newOperand(LABEL, ++labelCnt);
            newInterCode(IFGOTO, t1, f, l1, "!=");
            newInterCode(GOTO, l2);
            newInterCode(LABELDEC, l1);
            Operand t2 = transExp(node->children[2]);
            newInterCode(IFGOTO, t2, f, l3, "!=");
            newInterCode(GOTO, l2);
            newInterCode(LABELDEC, l3);
            newInterCode(ASSIGN, op, t);
            newInterCode(GOTO, l4);
            newInterCode(LABELDEC, l2);
            newInterCode(ASSIGN, op, f);
            newInterCode(LABELDEC, l4);
            return op;
        }
        else if(!strcmp(node->children[1]->name, "DOT")) { // Exp -> Exp DOT ID（结构体取成员）
            Type type = Exp(node->children[0]);
            
            Struc st = type->second.structure;
            FieldList fieldList = st->next;
            int cnt = 0;
            while(strcmp(fieldList->name, node->children[2]->val.type_str)) {
                cnt += getSize(fieldList->type);
                fieldList = fieldList->next;
            }
            // x = st.y;
            // t1 = &st + (len*4)
            // x := *t1
            Operand id = transExp(node->children[0]);
            Operand len = newOperand(CONSTANT, 4*cnt);
            Operand t1 = newOperand(TEMPVAR, ++tmpCnt);
            op = newOperand(DEREFERENCE, t1->u.value);
            if(id->kind == VARIABLE) {
                Symbol symb = hash_check(id->u.varName);
                if(symb != NULL && symb->is_addr) { // 结构体变量作为参数，即其本身为地址
                    // x = st.y;
                    // t1 = st + (len*4)
                    // x := *t1
                    newInterCode(ADD, t1, id, len);
                    return op;
                }
                Operand addr = newOperand(ADDR, id->u.varName);
                newInterCode(ADD, t1, addr, len);
            }
            else { // id 为临时变量
                Operand addr = newOperand(TEMPVAR, id->u.value);
                newInterCode(ADD, t1, addr, len);
            }
            return op;
        }
        else { // Exp -> Exp Operator Exp
            char* type = node->children[1]->name;
            if(!strcmp(type, "ASSIGNOP") && !strcmp(node->children[2]->children[0]->name, "ID") && !strcmp(node->children[2]->children[0]->val.type_str, "read")) {
                Operand param = transExp(node->children[0]);
                if(param->kind == DEREFERENCE) {
                    Operand t1 = newOperand(TEMPVAR, ++tmpCnt);
                    newInterCode(READ, t1);
                    newInterCode(ASSIGN, param, t1);
                }
                else {
                    newInterCode(READ, param);
                }
                return op;
            }
            Operand t1 = transExp(node->children[0]);
            Operand t2 = transExp(node->children[2]);
            if(!strcmp(type, "ASSIGNOP")) {
                newInterCode(ASSIGN, t1, t2);
                return t1;
            }
            else if(!strcmp(type, "PLUS")) {
                op = newOperand(TEMPVAR, ++tmpCnt);
                newInterCode(ADD, op, t1, t2);
            }
            else if(!strcmp(type, "MINUS")) {
                op = newOperand(TEMPVAR, ++tmpCnt);
                newInterCode(SUB, op, t1, t2);
            }
            else if(!strcmp(type, "STAR")) {
                op = newOperand(TEMPVAR, ++tmpCnt);
                newInterCode(MUL, op, t1, t2);
            }
            else if(!strcmp(type, "DIV")) {
                op = newOperand(TEMPVAR, ++tmpCnt);
                newInterCode(DIV, op, t1, t2);
            }
        }
        return op;
    }
    else if(node->child_num == 4) {
        if(!strcmp(node->children[0]->name, "ID")) { // Exp -> ID LP Args RP（含参函数调用）
            // func(a, b);
            // ARG b
            // ARG a
            // CALL func

            // struct St st;
            // func(st);
            // ARG &st
            // CALL func
            
            // write(x);
            // WRITE x
            if(!strcmp(node->children[0]->val.type_str, "write")) {
                Operand param = transExp(node->children[2]->children[0]);
                newInterCode(WRITE, param);
                return op;
            }
            transArgs(node->children[2]);
            op = newOperand(TEMPVAR, ++tmpCnt);
            newInterCode(ASSIGN, op, newOperand(FUNCCALL, node->children[0]->val.type_str));
            return op;
        }
        else if(!strcmp(node->children[0]->name, "Exp")) { // Exp -> Exp LB Exp RB（数组取值）
            // x = nums[y];
            // t1 = y * #4
            // t2 = &nums + t1
            // x := *t2

            // nums[y] = x;
            // t1 = y * #4
            // t2 = &nums + t1
            // *t2 := x
            Operand id = transExp(node->children[0]);
            Operand num = transExp(node->children[2]);
            Operand len, arr;
            if(id->kind == VARIABLE) {
                int size = getSize(hash_check(id->u.varName)->type->second.array.elem);
                len = newOperand(CONSTANT, 4*size);
                arr = newOperand(ADDR, id->u.varName);
            }
            else { // 上一层为结构体 Exp1 -> Exp DOT ID ，id 为地址
                Node *n = node->children[0];
                char *name = n->children[n->child_num-1]->val.type_str; // 结构体名
                Symbol symb = hash_check(name);
                int size = getSize(symb->type->second.array.elem);
                len = newOperand(CONSTANT, 4*size);
                arr = newOperand(TEMPVAR, id->u.value);
                
            }
            Operand t1 = newOperand(TEMPVAR, ++tmpCnt);
            Operand t2 = newOperand(TEMPVAR, ++tmpCnt);
            newInterCode(MUL, t1, num, len);
            newInterCode(ADD, t2, arr, t1);
            op = newOperand(DEREFERENCE, t2->u.value);
            return op;
        }
    }
    return op;
}

/*
    1. id 查询符号表
    2. 若为结构体类型，则传入地址
    3. 若传入的为 DEREFERENCE ，则转换为地址
*/
void transArgs(Node *node) {
    if(!node || !node->child_num) return;
    // Args -> Exp COMMA Args
    // | Exp
    if(node->child_num == 3) {
        transArgs(node->children[2]);
    }
    Operand op = transExp(node->children[0]);
    if(op->kind == DEREFERENCE) {
        Operand addr = newOperand(TEMPVAR, op->u.value);
        newInterCode(ARG, addr);
        return;
    }

    Type type = Exp(node->children[0]);
    if(type->first == STRUC) {
        Symbol symb = hash_check(op->u.varName);
        Operand addr = NULL;
        if(symb->is_addr) {
            addr = newOperand(VARIABLE, op->u.varName);
        }
        else {
            addr = newOperand(ADDR, op->u.varName);
        }
        newInterCode(ARG, addr);
        return;
    }
    
    newInterCode(ARG, op);
}

void printIc() {
    for(IRList n = head; n != NULL; n = n->next) {
        switch (n->code.kind)
        {
        case ASSIGN: {
            Operand left = n->code.u.assign.left, right = n->code.u.assign.right;
            printOp(left);
        if(!output)
            printf(" := ");
        else
            fprintf(output, " := ");

            printOp(right);
            break;
        }
        case ADD: case SUB: case MUL: case DIV: {
            Operand result = n->code.u.binop.result;
            Operand op1 = n->code.u.binop.op1;
            Operand op2 = n->code.u.binop.op2;
            printOp(result);
        if(!output)
            printf(" := ");
        else
            fprintf(output, " := ");

            printOp(op1);
        if(!output)
            printf(" %c ", "+-*/"[n->code.kind]);
        else
            fprintf(output, " %c ", "+-*/"[n->code.kind]);

            printOp(op2);
            break;
        }
        case FUNCDEC:
        if(!output)
            printf("FUNCTION %s :", n->code.u.unop.op->u.varName);
        else
            fprintf(output, "FUNCTION %s :", n->code.u.unop.op->u.varName);

            break;
        case PARAMDEC:
        if(!output)
            printf("PARAM %s", n->code.u.unop.op->u.varName);
        else
            fprintf(output, "PARAM %s", n->code.u.unop.op->u.varName);

            break;
        case RETURN:
        if(!output)
            printf("RETURN ");
        else
            fprintf(output, "RETURN ");

            printOp(n->code.u.unop.op);
            break;
        case IFGOTO:
        if(!output)
            printf("IF ");
        else
            fprintf(output, "IF ");

            printOp(n->code.u.ternop.op1);
        if(!output)
            printf(" %s ", n->code.u.ternop.relop);
        else
            fprintf(output, " %s ", n->code.u.ternop.relop);

            printOp(n->code.u.ternop.op2);
        if(!output)
            printf(" GOTO ");
        else
            fprintf(output, " GOTO ");

            printOp(n->code.u.ternop.target);
            break;
        case GOTO:
        if(!output)
            printf("GOTO ");
        else
            fprintf(output, "GOTO ");

            printOp(n->code.u.unop.op);
            break;
        case LABELDEC:
        if(!output)
            printf("LABEL ");
        else
            fprintf(output, "LABEL ");

            printOp(n->code.u.unop.op);
        if(!output)
            printf(" :");
        else
            fprintf(output, " :");

            break;
        case ARG:
        if(!output)
            printf("ARG ");
        else
            fprintf(output, "ARG ");
        
            printOp(n->code.u.unop.op);
            break;
        case DEC: {
            Operand id = n->code.u.assign.left, num = n->code.u.assign.right;
        if(!output)
            printf("DEC ");
        else
            fprintf(output, "DEC ");

            printOp(id);
            printOp(num);
            break;
        }
        case READ:
        if(!output)
            printf("READ ");
        else
            fprintf(output, "READ ");

            printOp(n->code.u.unop.op);
            break;
        case WRITE:
        if(!output)
            printf("WRITE ");
        else
            fprintf(output, "WRITE ");

            printOp(n->code.u.unop.op);
            break;

        default:
            break;
        }
        if(!output)
            printf("\n");
        else
            fputc('\n', output);

    }
}

void printOp(Operand op) {
    switch (op->kind)
    {
    case CONSTANT:
        if(!output)
            printf("#%d", op->u.value);
        else
            fprintf(output, "#%d", op->u.value);

        break;
    case VARIABLE:
        if(!output)
            printf("%s", op->u.varName);
        else
            fprintf(output, "%s", op->u.varName);

        break;
    case TEMPVAR:
        if(!output)
            printf("TEMP%d", op->u.value);
        else
            fprintf(output, "TEMP%d", op->u.value);

        break;
    case FUNCCALL:
        if(!output)
            printf("CALL %s", op->u.varName);
        else
            fprintf(output, "CALL %s", op->u.varName);

        break;
    case LABEL:
        if(!output)
            printf("L%d", op->u.value);
        else
            fprintf(output, "L%d", op->u.value);
    
        break;
    case BLOCK:
        if(!output)
            printf(" %d", op->u.value*4);
        else
            fprintf(output, " %d", op->u.value*4);

        break;
    case ADDR:
        if(!output)
            printf("&%s", op->u.varName);
        else
            fprintf(output, "&%s", op->u.varName);

        break;
    case DEREFERENCE:
        if(!output)
            printf("*TEMP%d", op->u.value);
        else
            fprintf(output, "*TEMP%d", op->u.value);

        break;

    default:
        break;
    }
}

int getSize(Type type) {
    int cnt = 0;
    switch(type->first) {
    case STRUC: {
        Struc st = type->second.structure;
        FieldList fieldList = st->next;
        while(fieldList != NULL) {
            cnt += getSize(fieldList->type);
            fieldList = fieldList->next;
        }
        break;
    }
    case ARRAY: {
        Type kind = type->second.array.elem;
        cnt = getSize(kind) * type->second.array.num;
        break;
    }
    default:
        cnt++;
        break;
    }
    return cnt;
}

int getOffset(Node* node, Type type) {
    int cnt = 0;
    if(node->child_num == 1) {
        printf("num = 1\n");
        char *name_ = node->children[0]->val.type_str; // 结构体名
        printf("num = 1\n");
        Symbol symb_ = hash_check(name_);
        type = symb_->type;
    }
    else if(node->child_num == 3) { // Exp -> Exp DOT ID
        printf("num = 3\n");
        char *name_ = node->children[2]->val.type_str; // field名
        Type type_ = NULL;
        cnt += getOffset(node->children[0], type_); // 拿到上层结构体的Type
        Struc st = type_->second.structure;
        FieldList fieldList = st->next;
        while(strcmp(fieldList->name, name_)) {
            cnt += getSize(fieldList->type);
            fieldList = fieldList->next;
        }
        type = fieldList->type; // 将本层Type传给下一层
    }
    else if(node->child_num == 4) { // Exp -> Exp LB Exp RB
        printf("num = 4\n");

    }
    return cnt;
}


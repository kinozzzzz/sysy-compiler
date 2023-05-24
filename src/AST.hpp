#pragma once
#include<string>
#include<memory>
#include<iostream>
#include<vector>

using namespace std;

#define KOOPA_MODE 0
#define RISCV_MODE 1

// 关于操作的宏定义
#define NoOperation 0
#define Invert 1
#define EqualZero 2
#define Return 3
#define Add 4
#define Sub 5
#define Mul 6
#define Div 7
#define Mod 8
#define Less 9
#define Greater 10
#define LessEq 11
#define GreaterEq 12
#define Equal 13
#define NotEqual 14
#define And 15
#define Or 16
#define NotEqualZero 17

class BaseAST;
class Expr;

#define Expr2(x) *(Expr*)(x)

int genTempVar();

class BaseAST
{
    public:
        virtual ~BaseAST() = default;
        virtual void print_koopa() = 0;
        virtual int ifNumber(){return 0;};
        virtual void genInstr(int instrType){};
        virtual void output(){};
        virtual int ifBoolean(){return 0;}
};

class ID : public BaseAST
{
    public:
        std::string id_name;

        virtual void print_koopa()
        {
            std::cout<<id_name;
        }
};

class Number : public BaseAST
{
    public:
        int num;

        virtual void print_koopa(){};
        virtual void output()
        {
            cout<<num;
        }
        virtual int ifNumber(){return 1;}
};

class Expr : public BaseAST
{
    public:
        BaseAST* left_expr;
        BaseAST* right_expr;
        int operation;
        int var;

        Expr(BaseAST *right,BaseAST *left = NULL,int oper = NoOperation){
            left_expr = left;
            right_expr = right;
            operation = oper;
            if(oper != NoOperation)
                var = genTempVar();
            else
                var = -1;
        }

        virtual void print_koopa()
        {
            if(left_expr)
                left_expr->print_koopa();
            right_expr->print_koopa();
            genInstr(operation);
        }

        void putChildExpr()
        {
            left_expr->output();
            cout<<", ";
            right_expr->output();
        }

        virtual void genInstr(int instrType)
        {
            if(instrType == NoOperation)
                return; 
            cout<<"  ";
            output();
            cout<<" = ";
            if(instrType == EqualZero)
            {
                cout<<"eq ";
                right_expr->output();
                cout<<", 0";
            }
            else if(instrType == Invert)
            {
                cout<<"sub 0, ";
                right_expr->output();
            }
            else if(instrType == Return)
            {
                cout<<"wrong ret";
            }
            else if(instrType == NotEqualZero)
            {
                cout<<"ne ";
                right_expr->output();
                cout<<", 0";
            }

            if(instrType == Add)
            {
                cout<<"add ";
            }
            else if(instrType == Sub)
            {
                cout<<"sub ";
            }
            else if(instrType == Mul)
            {
                cout<<"mul ";
            }
            else if(instrType == Div)
            {
                cout<<"div ";
            }
            else if(instrType == Mod)
            {
                cout<<"mod ";
            }
            else if(instrType == Less)
            {
                cout<<"lt ";
            }
            else if(instrType == LessEq)
            {
                cout<<"le ";
            }
            else if(instrType == Greater)
            {
                cout<<"gt ";
            }
            else if(instrType == GreaterEq)
            {
                cout<<"ge ";
            }
            else if(instrType == Equal)
            {
                cout<<"eq ";
            }
            else if(instrType == NotEqual)
            {
                cout<<"ne ";
            }
            else if(instrType == And)
            {
                cout<<"and ";
            }
            else if(instrType == Or)
            {
                cout<<"or ";
            }
            if(instrType >= Add && instrType <= Or)
            {
                putChildExpr();
            }
            cout<<endl;
        }

        virtual void output()
        {
            if(right_expr->ifNumber())
            {
                right_expr->output();
            }
            else
            {
                cout<<"%"<<var;
            }
        }

        virtual int ifBoolean()
        {
            if(operation >= Less && operation <= NotEqualZero)
            {
                return 1;
            }
            return 0;
        }
};

class Stmt : public BaseAST
{
    public:
        BaseAST* expr;

        virtual void print_koopa()
        {
            expr->print_koopa();
            genInstr(Return);
        }

        virtual void genInstr(int instrType)
        {
            if(instrType == Return)
            {
                cout<<"  ret ";
                expr->output();
            }
            cout<<endl;
        }
};

class Block : public BaseAST
{
    public:
        BaseAST* stmt;

        virtual void print_koopa()
        {
            cout<<"{\n%"<<"entry:\n";
            stmt->print_koopa();
            cout<<"}";
        }
};

class FuncType : public BaseAST
{
    public:
        std::string type_name;

        virtual void print_koopa()
        {
            cout<<type_name<<" ";
        }
};

class FuncDef : public BaseAST
{
    public:
        BaseAST* func_type;
        std::string* id;
        BaseAST* block;

        virtual void print_koopa()
        {
            std::cout<<"fun @";
            std::cout<<*id<<"(): ";
            func_type->print_koopa();
            block->print_koopa();
        }
};

class CompUnit : public BaseAST
{
    public:
        BaseAST* func_def;

        virtual void print_koopa()
        {
            func_def->print_koopa();
        }
};

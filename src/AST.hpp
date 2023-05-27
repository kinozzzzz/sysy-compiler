#pragma once
#include<string>
#include<memory>
#include<iostream>
#include<vector>
#include<map>

using namespace std;

//#define DEBUG 0     //总体流程debug
//#define DEBUG1 0    // bison文件debug
//#define DEBUG2 0    // koopa输出debug

#define KOOPA_MODE 0
#define RISCV_MODE 1

// 指令级操作
#define NoOperation 0
#define Invert 1
#define EqualZero 2
#define Add 3
#define Sub 4
#define Mul 5
#define Div 6
#define Mod 7
#define Less 8
#define Greater 9
#define LessEq 10
#define GreaterEq 11
#define Equal 12
#define NotEqual 13
#define And 14
#define Or 15
#define NotEqualZero 16

// 声明类型
#define VarDecl 0
#define ConstDecl 1

// 变量类型
#define TypeInt 0

// 语句类型
#define Assign 0
#define Return 1
#define Other 2

class BaseAST;
class Expr;
class Symbol
{
    public:
        int declType;
        int type;
        int value;
        int temp_var=-1;
        int ifAlloc = 0;
        string name;

        Symbol(int dtp,string n,int tp = TypeInt,int v=0)
        {
            type = tp;
            declType = dtp;
            value = v;
            name = n;
        }
};

typedef map<string,Symbol*> SymbolMap;

static int temp_var_range=0;
static int block_range=0;
static int now_block_id;
static vector<SymbolMap> symbol_table;
static int ifReturn = 0;    //暂时处理无跳转情况的重复跳转语句
static int genTempVar()
{
    // 生成临时变量的下标
    return temp_var_range++;
}
static int genBlockId()
{
    return block_range++;
}

static void Error()
{
    cout<<"error";
    exit(0);
}

static void delSymbolMap(SymbolMap &map)
{
    for(auto iter = map.begin();iter != map.end();iter++)
    {
        delete iter->second;
    }
}

static Symbol* findSymbol(string &id)
{
    for(int i=symbol_table.size()-1;i >= 0;i--)
    {
        if(symbol_table[i].count(id))
            return symbol_table[i][id];
    }
    Error();
    return NULL;
}

class BlockItems
{
    public:
        vector<BaseAST*> vec;
};

class BaseAST
{
    public:
        virtual ~BaseAST() = default;
        virtual void print_koopa() = 0;
        virtual int ifNumber(){return 0;};
        virtual void genInstr(int instrType){};
        virtual void output(){};
        virtual int ifBoolean(){return 0;}
        virtual int ifDecls(){return 0;}
        virtual int eval(){return 0;}
        virtual int ifStmt(){return 0;}
        virtual int ifExpr(){return 0;}
        virtual int ifVar(){return 0;}
};

class Var : public BaseAST    // 为了string变量的传输
{
    public:
        string id_name;

        Var(string a)
        {
            id_name = a;
        }

        virtual void print_koopa()
        {
            // 变量的print_koopa函数对应将变量移入临时变量(寄存器)的过程
            Symbol *symbol = findSymbol(id_name);
            //cout<<symbol->temp_var<<symbol->declType<<endl;
            if(symbol->temp_var == -1 && symbol->declType != ConstDecl)
            {
                int temp_var = genTempVar();
                cout<<"  %"<<temp_var<<" = load "<<symbol->name<<endl;
                symbol->temp_var = temp_var;
            }
        }

        virtual void output()
        {
            Symbol *symbol = findSymbol(id_name);
            if(symbol->temp_var == -1)
                cout<<symbol->value;
            else
                cout<<"%"<<symbol->temp_var;
        }

        virtual int eval()
        {
            return findSymbol(id_name)->value;
        }
        virtual int ifVar(){return 1;}
};

class Number : public BaseAST
{
    public:
        int num;

        virtual void print_koopa()
        {

        };
        virtual void output()
        {
            cout<<num;
        }
        virtual int ifNumber(){return 1;}
        virtual int eval()
        {
            return num;
        }
};

class DeclareDef : BaseAST
{
    public:
        BaseAST *expr;
        string id;
        int declType;
        int type;

        DeclareDef(string varId,BaseAST *e=NULL)
        {
            id = varId;
            expr = e;
        }
        virtual void print_koopa()
        {
            #ifdef DEBUG2
            cout<<"koopa: DeclareDef"<<endl;
            cout<<"     "<<declType<<endl;
            #endif
            Symbol *symbol;
            int depth = symbol_table.size() - 1;
            string name = string("@_") + to_string(now_block_id) + id;
            if(declType == VarDecl)
            {
                symbol = new Symbol(VarDecl,name,type);
                // 如果这个变量还没有分配过内存，那么就分配内存
                if(!(symbol_table[depth].count(id) && symbol_table[depth][id]->ifAlloc))
                {
                    cout<<"  "<<name<<" = alloc ";
                    if(type == TypeInt)
                        cout<<"i32";
                    cout<<endl;
                    symbol->ifAlloc = 1;
                }
                if(expr != NULL)
                {
                    int value = expr->eval();
                    cout<<"  store "<<value<<", "<<name<<endl;
                    symbol->value = value;
                }
            }
            else if(declType == ConstDecl)
            {
                int value = expr->eval();
                symbol = new Symbol(ConstDecl,name,type,value);
            }

            if(symbol_table[depth].count(id))
            {
                symbol->ifAlloc = symbol_table[depth][id]->ifAlloc;
                symbol_table[depth][id] = symbol;
            }
            else
            {
                symbol_table[depth].emplace(id,symbol);
            }
            
        }
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

        virtual int ifExpr(){return 1;}

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
            cout<<"%"<<var;
        }

        virtual int ifBoolean()
        {
            if(operation >= Less && operation <= NotEqualZero)
            {
                return 1;
            }
            return 0;
        }

        virtual int eval()
        {
            if(operation >= Add && operation <= Or)
            {
                int temp1 = left_expr->eval();
                int temp2 = right_expr->eval();
                if(operation == Add)
                    return temp1 + temp2;
                else if(operation == Sub)
                    return temp1 - temp2;
                else if(operation == Mul)
                    return temp1 * temp2;
                else if(operation == Div)
                    return temp1 / temp2;
                else if(operation == Mod)
                    return temp1 % temp2;
                else if(operation == Less)
                    return temp1 < temp2;
                else if(operation == Greater)
                    return temp1 > temp2;
                else if(operation == LessEq)
                    return temp1 <= temp2;
                else if(operation == GreaterEq)
                    return temp1 >= temp2;
                else if(operation == Equal)
                    return temp1 == temp2;
                else if(operation == NotEqual)
                    return temp1 != temp2;
                else if(operation == And)
                    return temp1 & temp2;
                else if(operation == Or)
                    return temp1 | temp2;
            }
            int temp = right_expr->eval();
            if(operation == NoOperation)
                return temp;
            else if(operation == Invert)
                return -temp;
            else if(operation == EqualZero)
                return temp == 0;
            else if(operation == NotEqualZero)
                return temp != 0;
            return 0;
        }
};

class Stmt : public BaseAST
{
    public:
        BaseAST* expr;
        Var *var;
        int stmt_type;

        Stmt(BaseAST *e,int type,Var *name = NULL)
        {
            expr = e;
            var = name;
            stmt_type = type;
        }

        virtual void print_koopa()
        {
            expr->print_koopa();
            genInstr();
        }

        virtual void genInstr(int instrType = NoOperation)
        {
            if(stmt_type == Other)
                return;
            
            if(stmt_type == Return)
            {
                ifReturn = 1;
                cout<<"  ret ";
                expr->output();
            }
            else if(stmt_type == Assign)
            {
                cout<<"  ";
                Symbol* symbol = findSymbol(var->id_name);
                symbol->value = expr->eval();
                if(expr->ifExpr())
                    symbol->temp_var = ((Expr*)expr)->var;
                else if(expr->ifVar())
                {
                    string id = ((Var*)expr)->id_name;
                    symbol->temp_var = findSymbol(id)->temp_var;
                }
                cout<<"store ";
                expr->output();
                cout<<", "<<symbol->name;
            }
            cout<<endl;
        }

        virtual int ifStmt(){return 1;}
};

class Decls : public BaseAST
{
    public:
        vector<DeclareDef*> defs;
        int type;
        Decls(int t)
        {
            type = t;
        }
        virtual int ifDecls(){return 1;}
        virtual void print_koopa(){}
};

class Block : public BaseAST
{
    public:
        vector<BaseAST*> stmts;

        virtual void print_koopa()
        {
            #ifdef DEBUG2
            //cout<<"size of stmts:"<<stmts.size()<<endl;
            #endif
            SymbolMap new_map;
            symbol_table.push_back(new_map);
            int block_id = genBlockId();
            for(int i=0;i < stmts.size();i++)
            {
                now_block_id = block_id;
                if(ifReturn)
                    break;
                stmts[i]->print_koopa();
                if(stmts[i]->ifStmt() && ((Stmt*)stmts[i])->stmt_type == Return)
                {
                    break;
                }
            }
            delSymbolMap(*(symbol_table.rbegin()));
            symbol_table.pop_back();
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
        string id;
        BaseAST* block;

        virtual void print_koopa()
        {
            cout<<"fun @";
            cout<<id<<"(): ";
            func_type->print_koopa();
            cout<<"{\n%"<<"entry:\n";
            block->print_koopa();
            cout<<"}";
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
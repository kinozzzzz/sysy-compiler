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
#define ParamDecl 2
#define ArrayDecl 3

// 变量类型,如int或string等
#define TypeInt 0
#define TypeVoid 1
#define TypePointer 2    // 通俗的指针类型，包括指针数组

// 语句类型
#define Assign 0
#define Return 1
#define Break 2
#define Continue 3
#define Other 4



class BaseAST;
class Expr;
class Stmt;
class JumpStmt;
class InitVal;

class Symbol
{
    public:
        int declType;
        int type;
        int value;
        int ifAlloc = 0;
        int len = 0;
        string name;

        Symbol(int dtp,string n,int tp = TypeInt,int v=0)
        {
            type = tp;
            declType = dtp;
            value = v;
            name = n;
        }
};

class FuncInfo
{
    public:
        int func_type;
        vector<int> param_type;
        FuncInfo(){}
        FuncInfo(int type)
        {
            func_type = type;
        }
        FuncInfo(int type,vector<int> param)
        {
            func_type = type;
            param_type = param;
        }
        FuncInfo(const FuncInfo &a)
        {
            func_type = a.func_type;
            param_type = a.param_type;
        }
};

typedef map<string,Symbol*> SymbolMap;

static int temp_var_range=0;
static int block_range=0;
static int now_block_id;
static int jump_stmt_id=0;
static vector<SymbolMap> symbol_table(1);
static map<string,FuncInfo*> func_table;
static int ifReturn = 0;    //处理重复跳转语句
static vector<int> while_stmt_id;
static int genTempVar()
{
    // 生成临时变量的下标
    return temp_var_range++;
}
static int genBlockId()
{
    return block_range++;
}
static int genJumpStmtId()
{
    return jump_stmt_id++;
}
static void Error(const char *message)
{
    cout<<"error: "<<message<<endl;
    exit(0);
}
static inline void genBlock(const char *name,int id)
{
    cout<<"%"<<name<<"_"<<id<<":"<<endl;
    ifReturn = 0;   //对于每个block允许return一次
}
static int ifJumpInstr(BaseAST *s);
static int result_id = 0;
static int genResultId()
{
    return result_id++;
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
    Error("Can't find the symbol in symbol table");
    return NULL;
}

static void printType(int type,vector<int> offset)
{
    if(offset.size() == 0)
    {
        if(type == TypeInt || type == TypePointer)
            cout<<"i32";
    }
    else
    {
        cout<<"[";
        int o = offset[0];
        offset.erase(offset.begin());
        printType(type,offset);
        cout<<", "<<o<<"]";
    }
}

static void printConstInit(InitVal *init,vector<int> offset);
static void printVarInit(InitVal *init,vector<int> offset,string id,int now_num);

static int ptr_id=0;
static int genPtrId()
{
    return ptr_id++;
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
        virtual int ifBlock(){return 0;}
        virtual int ifEmptyBlock(){return 0;}
        virtual int ifJumpStmt(){return 0;}
        virtual int ifInitVal(){return 0;}
        virtual void load_array(){}
};

class Var : public BaseAST    // 表示变量
{
    public:
        string id;
        vector<BaseAST*> offset;
        int ptr_id = -1;
        int temp_var = -1;

        Var(string a)
        {
            id = a;
        }

        virtual void print_koopa()
        {
            // 变量的print_koopa函数对应将变量移入临时变量(寄存器)的过程
            // 由于koopa奇特的规则，决定每次使用变量时都将变量的值移入temp_var中
            Symbol *symbol = findSymbol(id);
            if(symbol->declType == VarDecl)
            {
                temp_var = genTempVar();
                cout<<"  %"<<temp_var<<" = load "<<symbol->name<<endl;
            }
            else if(symbol->declType == ArrayDecl)
            {
                load_array();
                temp_var = genTempVar();
                cout<<"  %"<<temp_var<<" = load %ptr_"<<ptr_id<<endl;
            }
        }

        virtual void load_array()
        {
            if(offset.size() == 0)
                return;
            Symbol *symbol = findSymbol(id);
            int old_ptr_id;
            ptr_id = genPtrId();
            offset[0]->print_koopa();
            if(symbol->type != TypePointer)
                cout<<"  %ptr_"<<ptr_id<<" = getelemptr "<<symbol->name<<", ";
            else
            {
                cout<<"  %ptr_"<<ptr_id<<" = load "<<symbol->name<<endl;
                old_ptr_id = ptr_id;
                ptr_id = genPtrId();
                cout<<"  %ptr_"<<ptr_id<<" = getptr %ptr_"<<old_ptr_id<<", ";
            }
            offset[0]->output();
            cout<<endl;
            for(int i=1;i < offset.size();i++)
            {
                old_ptr_id = ptr_id;
                ptr_id = genPtrId();
                offset[i]->print_koopa();
                cout<<"  %ptr_"<<ptr_id<<" = getelemptr %ptr_"<<old_ptr_id<<", ";
                offset[i]->output();
                cout<<endl;
            }
        }

        virtual void output()
        {
            // var： 输出此时存在哪个临时变量中
            // const： 输出const的值
            Symbol *symbol = findSymbol(id);
            if(symbol->declType == ConstDecl)
                cout<<symbol->value;
            else if(symbol->declType == VarDecl)
            {
                if(temp_var != -1)
                    cout<<"%"<<temp_var;    // 每次用之前都会先取一下
                else
                    cout<<symbol->name;
            }
            else if(symbol->declType == ArrayDecl)
            {
                if(offset.size() == 0)
                {
                    cout<<symbol->name;
                    return;
                }
                if(temp_var != -1)
                    cout<<"%"<<temp_var;
                else
                    cout<<"%ptr_"<<ptr_id;
            }
        }

        virtual int eval()
        {
            return findSymbol(id)->value;
        }
        virtual int ifVar(){return 1;}
};

class InitVal : public BaseAST
{
    public:
        vector<BaseAST*> inits;
        virtual void print_koopa()
        {
            for(int i=0;i < inits.size();i++)
            {
                if(inits[i] == NULL)
                    continue;
                inits[i]->print_koopa();
            }
        }
        virtual int ifInitVal(){return 1;}
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

class DeclareDef : public BaseAST
{
    public:
        string name;
        BaseAST *init;
        int declType=VarDecl;
        int type=TypeInt;
        vector<BaseAST*> offset;

        DeclareDef(string n,BaseAST *e=NULL)
        {
            name = n;
            init = e;
        }
        virtual void print_koopa()
        {
            // 非全局变量的声明
            #ifdef DEBUG2
            cout<<"koopa: DeclareDef"<<endl;
            #endif
            Symbol *symbol;
            int depth = symbol_table.size() - 1;
            string id = string("@_") + to_string(now_block_id) + name;
            if(declType == ConstDecl && offset.size() == 0)
            {
                // 不是数组的常量
                int value = init->eval();
                symbol = new Symbol(ConstDecl,id,type,value);
            }

            else if(declType == VarDecl && offset.size() == 0)
            {
                symbol = new Symbol(VarDecl,id,type);
                // 如果这个变量还没有分配过内存，那么就分配内存
                if(!(symbol_table[depth].count(name) && symbol_table[depth][name]->ifAlloc))
                {
                    cout<<"  "<<id<<" = alloc ";
                    if(type == TypeInt)
                        cout<<"i32";
                    cout<<endl;
                    symbol->ifAlloc = 1;
                }
                if(init != NULL)
                {
                    init->print_koopa();
                    int value = init->eval();
                    cout<<"  store ";
                    init->output();
                    cout<<", "<<id<<endl;
                    symbol->value = value;
                }
            }

            else if(declType == ParamDecl)
            {
                if(type == TypePointer)
                {
                    symbol = new Symbol(ArrayDecl,id,type);
                    symbol->len = offset.size();
                }
                else
                    symbol = new Symbol(VarDecl,id,type);
                cout<<"  "<<id<<" = alloc ";
                vector<int> temp;
                for(int i=0;i < offset.size();i++)
                    temp.push_back(offset[i]->eval());
                if(type == TypePointer) cout<<"*";
                printType(type,temp);
                cout<<endl;
                cout<<"  store @"<<name<<", "<<id<<endl;
            }

            else if(offset.size() != 0)
            {
                vector<int> temp;
                for(int i=0;i < offset.size();i++)
                    temp.push_back(offset[i]->eval());
                symbol = new Symbol(ArrayDecl,id,type);
                symbol->len = offset.size();
                cout<<"  "<<id<<" = alloc ";
                printType(type,temp);
                cout<<endl;
                cout<<"  store zeroinit, "<<id<<endl;
                if(init != NULL)
                {
                    vector<int> vec;
                    vec.push_back(1);
                    int num=1;
                    for(int j=int(temp.size())-1;j >= 0;j--)
                    {
                        num = num * temp[j];
                        vec.push_back(num);
                    }
                    vec.pop_back();
                    printVarInit((InitVal*)init,vec,id,0);
                }
            }

            if(symbol_table[depth].count(name))
            {
                symbol->ifAlloc = symbol_table[depth][name]->ifAlloc;
                symbol_table[depth][name] = symbol;
            }
            else
            {
                symbol_table[depth].emplace(name,symbol);
            }
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
            //cout<<stmt_type<<" "<<expr<<endl;
            if(expr)
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
                if(expr)
                    expr->output();
            }
            else if(stmt_type == Assign)
            {
                var->load_array();
                cout<<"  ";
                Symbol* symbol = findSymbol(var->id);
                symbol->value = expr->eval();
                cout<<"store ";
                expr->output();
                cout<<", ";
                var->output();
            }
            else if(stmt_type == Break)
            {
                ifReturn = 1;
                cout<<"  jump %break_"<<*(while_stmt_id.rbegin());
            }
            else if(stmt_type == Continue)
            {
                ifReturn = 1;
                cout<<"  jump %while_"<<*(while_stmt_id.rbegin());
            }
            cout<<endl;
            
        }

        virtual int ifStmt(){return 1;}
};

class JumpStmt : public BaseAST
{
    public:
        BaseAST *expr;
        BaseAST *then_stmt;
        BaseAST *else_stmt;
        int id;
        //BaseAST *end_stmt;
        JumpStmt(BaseAST *e,BaseAST *s1,BaseAST *s2=NULL)
        {
            expr = e;
            then_stmt = s1;
            else_stmt = s2;
        }
        virtual void print_koopa()
        {
            id = genJumpStmtId();
            expr->print_koopa();
            int then_block,else_block;  // 表示是否要生成该基本块
            if(then_stmt)
                then_block = !then_stmt->ifEmptyBlock();  
            else
                then_block = 0;
            if(else_stmt)
                else_block = !else_stmt->ifEmptyBlock();
            else
                else_block = 0;
            if(!then_block && !else_block)  //两个基本块都不需要生成
                return;

            int temp_var = genTempVar();
            if(else_block && !then_block)
            {
                cout<<"  %"<<temp_var<<" = eq ";
                expr->output();
                cout<<", 0"<<endl;
                cout<<"  br ";
                cout<<"%"<<temp_var;
            }
            else
            {
                cout<<"  br ";
                expr->output();
            }
            int ret1=0,ret2=0;
            if(then_block && else_block)
            {
                cout<<", %then_"<<id<<", %else_"<<id<<endl;
                genBlock("then",id);
                then_stmt->print_koopa();
                ret1=ifJumpInstr(then_stmt);
                if(!ret1)
                    cout<<"  jump %end_"<<id<<endl;
                genBlock("else",id);
                else_stmt->print_koopa();
                ret2=ifJumpInstr(else_stmt);
                if(!ret2)
                    cout<<"  jump %end_"<<id<<endl;
            }
            else if(then_block)
            {
                cout<<", %then_"<<id<<", %end_"<<id<<endl;
                genBlock("then",id);
                then_stmt->print_koopa();
                ret1=ifJumpInstr(then_stmt);
                //cout<<ret1<<endl;
                if(!ret1)
                    cout<<"  jump %end_"<<id<<endl;
            }
            else if(else_block)
            {
                cout<<", %else_"<<id<<", %end_"<<id<<endl;
                genBlock("else",id);
                else_stmt->print_koopa();
                ret2=ifJumpInstr(else_stmt);
                if(!ret2)
                    cout<<"  jump %end_"<<id<<endl;
            }
            if(ret1 && ret2)
                return;
            genBlock("end",id);
        }
        virtual int ifJumpStmt(){return 1;}
};

class WhileStmt : public BaseAST
{
    public:
        BaseAST *expr;
        BaseAST *stmt;
        int id;
        WhileStmt(BaseAST *e,BaseAST *s=NULL)
        {
            expr = e;
            stmt = s;
        }
        virtual void print_koopa()
        {
            id = genBlockId();
            while_stmt_id.push_back(id);
            cout<<"  jump %while_"<<id<<endl;
            genBlock("while",id);
            expr->print_koopa();
            cout<<"  br ";
            expr->output();
            cout<<", %stmt_"<<id<<", %break_"<<id<<endl;
            genBlock("stmt",id);
            if(stmt)
            {
                stmt->print_koopa();
                if(!ifJumpInstr(stmt) && !ifReturn)
                    cout<<"  jump %while_"<<id<<endl;
            }
            else
                cout<<"  jump %while_"<<id<<endl;
            genBlock("break",id);
            while_stmt_id.pop_back();
        }
};

class Expr : public BaseAST
{
    public:
        BaseAST* left_expr;
        BaseAST* right_expr;
        int operation;
        int var=-1;

        Expr(BaseAST *right,BaseAST *left = NULL,int oper = NoOperation)
        {
            left_expr = left;
            right_expr = right;
            operation = oper;
        }

        virtual void print_koopa()
        {
            if(operation == Or)
            {
                Number num = Number();
                num.num = 1;
                string name = string("result") + to_string(genResultId());
                // 为每个result增加id，避免result重名出现冲突
                Var *v = new Var(name);
                DeclareDef def = DeclareDef(name,(BaseAST*)&num);
                def.declType = VarDecl;
                Stmt s1 = Stmt(right_expr,Assign,v);
                JumpStmt stmt = JumpStmt(left_expr,NULL,(BaseAST*)&s1);
                def.print_koopa();
                stmt.print_koopa();
                v->print_koopa();

                var = v->temp_var;
                return;
            }
            if(operation == And)
            {
                Number num = Number();
                num.num = 0;
                string name = string("result") + to_string(genResultId());
                // 为每个result增加id，避免result重名出现冲突
                Var *v = new Var(name);
                DeclareDef def = DeclareDef(name,(BaseAST*)&num);
                def.declType = VarDecl;
                Stmt s1 = Stmt(right_expr,Assign,v);
                JumpStmt stmt = JumpStmt(left_expr,(BaseAST*)&s1,NULL);
                def.print_koopa();
                stmt.print_koopa();
                v->print_koopa();

                var = v->temp_var;
                return;
            }
            var = genTempVar();
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
            // 输出expr执行后对应的temp_var
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

class Decls : public BaseAST
{
    /*
        传递变量定义的中间类
        现用来表达全局变量
    */
    public:
        vector<DeclareDef*> defs;
        int type;
        Decls(int t=TypeInt)
        {
            type = t;
        }
        virtual int ifDecls(){return 1;}
        virtual void print_koopa()
        {
            for(int i=0;i < defs.size();i++)
            {
                Symbol *symbol;
                if(defs[i]->declType == ConstDecl && defs[i]->offset.size() == 0)
                {
                    int value = (defs[i]->init)->eval();
                    symbol = new Symbol(ConstDecl,defs[i]->name,defs[i]->type,value);
                    symbol_table[0].emplace(defs[i]->name,symbol);
                    continue;
                }
                else if(defs[i]->declType == VarDecl && defs[i]->offset.size() == 0)
                {
                    string id = string("@global_") + defs[i]->name;
                    cout<<"global "<<id<<" = alloc i32, ";
                    if(defs[i]->init)
                    {
                        int value = (defs[i]->init)->eval();
                        symbol = new Symbol(VarDecl,id,defs[i]->type,value);
                        cout<<value;
                    }
                    else
                    {
                        symbol = new Symbol(VarDecl,id,defs[i]->type);
                        cout<<0;
                    }
                }
                else if(defs[i]->offset.size() != 0)
                {
                    vector<int> temp;
                    for(int j=0;j < defs[i]->offset.size();j++)
                        temp.push_back((defs[i]->offset[j])->eval());
                    string id = string("@global_") + defs[i]->name;
                    cout<<"global "<<id<<" = alloc ";
                    printType(defs[i]->type,temp);
                    vector<int> vec;
                    int num=1;
                    for(int j=int(temp.size())-1;j >= 0;j--)
                    {
                        num = num * temp[j];
                        vec.push_back(num);
                    }
                    cout<<", ";
                    symbol = new Symbol(ArrayDecl,id,type);
                    symbol->len = defs[i]->offset.size();
                    printConstInit((InitVal*)defs[i]->init,vec);
                }
                symbol_table[0].emplace(defs[i]->name,symbol);
                cout<<endl;
            }
            cout<<endl;
        }
};

class Block : public BaseAST
{
    public:
        vector<BaseAST*> stmts;

        virtual void print_koopa()
        {
            #ifdef DEBUG2
            //cout<<"stmts in block: "<<stmts.size()<<endl;
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
            }
            delSymbolMap(*(symbol_table.rbegin()));
            symbol_table.pop_back();
        }
        virtual int ifBlock(){return 1;}
        virtual int ifEmptyBlock()
        {
            return stmts.size() == 0;
        }
};

class FuncDef : public BaseAST
{
    public:
        int func_type;
        string id;
        BaseAST* block;
        vector<DeclareDef*> params;

        virtual void print_koopa()
        {
            FuncInfo *func_info = new FuncInfo();
            func_info->func_type = func_type;

            ifReturn = 0;
            // 函数声明
            cout<<"fun @";
            cout<<id<<"(";
            for(int i=0;i < int(params.size());i++)
            {
                func_info->param_type.push_back(params[i]->type);

                vector<int> vec;
                cout<<"@"<<params[i]->name<<": ";
                for(int j=0;j < params[i]->offset.size();j++)
                    vec.push_back(params[i]->offset[j]->eval());
                if(params[i]->type == TypePointer) cout<<"*";
                printType(params[i]->type,vec);
                if(i != int(params.size())-1)
                    cout<<", ";
            }
            cout<<")";

            func_table.emplace(id,func_info);
            if(func_type == TypeInt)
                cout<<": i32";
            // 函数体
            cout<<" {\n%"<<"entry:\n";
            for(int i=params.size()-1;i >= 0;i--)
            {
                ((Block*)block)->stmts.insert(((Block*)block)->stmts.begin(),params[i]);
            }
            // 处理没有返回语句的情况
            if(func_type==TypeInt)
            {
                Number *num = new Number();
                num->num = 0;
                Stmt *stmt = new Stmt((BaseAST*)num,Return);
                ((Block*)block)->stmts.push_back((BaseAST*)stmt);
            }
            else if(func_type == TypeVoid)
            {
                Stmt *stmt = new Stmt(NULL,Return);
                ((Block*)block)->stmts.push_back((BaseAST*)stmt);
            }
            block->print_koopa();
            cout<<"}\n\n";
        }
};

class Program : public BaseAST
{
    public:
        vector<BaseAST*> units;

        virtual void print_koopa()
        {
            FuncInfo *func_info;
            cout<<"decl @getint(): i32"<<endl;
            func_info = new FuncInfo(TypeInt);
            func_table.emplace(string("getint"),func_info);

            cout<<"decl @getch(): i32"<<endl;
            func_info = new FuncInfo(TypeInt);
            func_table.emplace(string("getch"),func_info);

            cout<<"decl @getarray(*i32): i32"<<endl;
            func_info = new FuncInfo(TypeInt,vector<int>({TypePointer}));
            func_table.emplace(string("getarray"),func_info);

            cout<<"decl @putint(i32)"<<endl;
            func_info = new FuncInfo(TypeVoid,vector<int>({TypeInt}));
            func_table.emplace(string("putint"),func_info);

            cout<<"decl @putch(i32)"<<endl;
            func_info = new FuncInfo(TypeVoid,vector<int>({TypeInt}));
            func_table.emplace(string("putch"),func_info);

            cout<<"decl @putarray(i32, *i32)"<<endl;
            func_info = new FuncInfo(TypeVoid,vector<int>({TypeInt,TypePointer}));
            func_table.emplace(string("putarray"),func_info);

            cout<<"decl @starttime()"<<endl;
            func_info = new FuncInfo(TypeVoid);
            func_table.emplace(string("starttime"),func_info);

            cout<<"decl @stoptime()"<<endl;
            func_info = new FuncInfo(TypeVoid);
            func_table.emplace(string("stoptime"),func_info);
            cout<<endl;

            for(int i=0;i < units.size();i++)
            {
                if(units[i]->ifDecls())
                    units[i]->print_koopa();
            }
            for(int i=0;i < units.size();i++)
            {
                if(!units[i]->ifDecls())
                    units[i]->print_koopa();
            }
        }
};

class FuncCall : public BaseAST
{
    public:
        string name;
        vector<BaseAST*> params;
        int temp_var;

        virtual void print_koopa()
        {
            #ifdef DEBUG2
            cout<<"FuncCall "<<name<<endl;
            #endif
            FuncInfo *func_info = func_table[name];
            int return_type = func_info->func_type;
            temp_var = genTempVar();
            // 先加载每个param
            for(int i=0;i < params.size();i++)
            {
                switch (func_info->param_type[i])
                {
                    case TypeInt:
                        params[i]->print_koopa();
                        break;
                    case TypePointer:
                        Number *num = new Number();
                        num->num = 0;
                        ((Var*)params[i])->offset.push_back(num);
                        params[i]->load_array();
                        break;
                }
            }
            
            cout<<"  ";
            if(return_type == TypeInt)
            {   
                // 如果函数有返回值，那么赋予一个临时变量
                cout<<"%"<<temp_var<<" = ";
            }
            cout<<"call @"<<name<<"(";
            for(int i=0;i < int(params.size())-1;i++)
            {
                params[i]->output();
                cout<<",";
            }
            if(params.size() != 0)
                (*params.rbegin())->output();
            cout<<")"<<endl;
        }

        virtual void output()
        {
            cout<<"%"<<temp_var;
        }
};

// 返回0表示需要生成jump语句
// 返回1表示基本块的最后是ret语句或其他不需要再生成jump的语句
static int ifJumpInstr(BaseAST *s)
{
    //cout<<s->ifStmt()<<s->ifBlock()<<endl;
    if(s->ifStmt())
    {
        Stmt *stmt = (Stmt*)s;
        //cout<<stmt->stmt_type<<endl;
        if(stmt->stmt_type == Return)
            return 1;
        if(stmt->stmt_type == Break || stmt->stmt_type == Continue)
            return 1;
    }
    else if(s->ifBlock())
    {
        Block *block = (Block*)s;
        if(block->ifEmptyBlock()) return 0;
        return ifJumpInstr(*((block->stmts).rbegin()));
    }
    else if(s->ifJumpStmt())
    {
        // 处理branch的情况
        JumpStmt *stmt = (JumpStmt*)s;
        int ret1=0,ret2=0;
        if(stmt->then_stmt)
            ret1 = ifJumpInstr(stmt->then_stmt);
        if(stmt->else_stmt)
            ret2 = ifJumpInstr(stmt->else_stmt);
        return ret1 && ret2;
    }
    return 0;
}

static int findArraySize(vector<int> offset,int now_num)
{
    int num=1;
    for(int i=0;i < offset.size();i++)
    {
        if(now_num % offset[i] == 0)
            num = offset[i];
        else
            break;
    }
    return num;
}

static void printConstInit(InitVal *init,vector<int> offset)
{
    if(init == NULL)
    {
        cout<<"zeroinit";
        return;
    }
    int need_num = *(offset.rbegin());
    offset.pop_back();
    if(offset.size() == 0)
    {
        int i=0;
        cout<<"{";
        for(;i < init->inits.size();i++)
        {
            cout<<init->inits[i]->eval();
            if(i != need_num-1)
                cout<<",";
        }
        for(;i < need_num;i++)
        {
            cout<<0;
            if(i != need_num-1)
                cout<<",";
        }
        cout<<"}";
        return;
    }
    else
    {
        int child_num = *(offset.rbegin());
        int now_num = 0;
        InitVal *val = new InitVal();
        cout<<"{";
        for(int i=0;i < init->inits.size();i++)
        {
            if(init->inits[i] == NULL || init->inits[i]->ifInitVal())
            {
                int rule_num = findArraySize(offset,now_num);
                now_num += rule_num;
                if(rule_num == child_num)
                {
                    printConstInit((InitVal*)init->inits[i],offset);
                    if(now_num != need_num)
                        cout<<",";
                    continue;
                }
                val->inits.push_back(init->inits[i]);
                if(now_num % child_num == 0)
                {
                    printConstInit(val,offset);
                    val->inits.clear();
                    if(now_num != need_num)
                        cout<<",";
                }
            }
            else
            {
                val->inits.push_back(init->inits[i]);
                now_num++;
                if(now_num%child_num == 0)
                {
                    printConstInit(val,offset);
                    val->inits.clear();
                    if(now_num != need_num)
                        cout<<",";
                }
           }
        }
        if(val->inits.size() != 0)
        {
            printConstInit(val,offset);
            now_num = (now_num/child_num+1)*child_num;
            if(now_num != need_num)
                cout<<",";
        }
        delete val;
        while(now_num < need_num)
        {
            cout<<"zeroinit";
            now_num += child_num;
            if(now_num != need_num)
                cout<<",";
        }
        cout<<"}";
    }
}

static void printVarInit(InitVal *init,vector<int> offset,string id,int now_num)
{
    for(int i=0;i < init->inits.size();i++)
    {
        if(init->inits[i] == NULL)
        {
            int rule_num = findArraySize(offset,now_num);
            now_num += rule_num;
        }
        else if(init->inits[i]->ifInitVal())
        {
            int rule_num = findArraySize(offset,now_num);
            printVarInit((InitVal*)init->inits[i],offset,id,now_num);
            now_num += rule_num;
        }
        else
        {
            int ptr_id;
            int temp_num = now_num;
            int old_ptr_id;
            ptr_id = genPtrId();
            (init->inits[i])->print_koopa();
            cout<<"  %ptr_"<<ptr_id<<" = getelemptr "<<id<<", "<<temp_num / (*(offset.rbegin()));
            temp_num = temp_num % (*(offset.rbegin()));
            cout<<endl;
            for(int j=int(offset.size())-2;j >= 0;j--)
            {
                old_ptr_id = ptr_id;
                ptr_id = genPtrId();
                //cout<<"ptr_id:"<<genPtrId()<<endl;
                cout<<"  %ptr_"<<ptr_id<<" = getelemptr %ptr_"<<old_ptr_id<<", "<<temp_num / offset[j];
                temp_num = temp_num % offset[j];
                cout<<endl;
            }
            cout<<"  store ";
            (init->inits[i])->output();
            cout<<", %ptr_"<<ptr_id<<endl;
            now_num++;
        }
    }
}
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

// 变量类型
#define TypeInt 0
#define TypeVoid 1

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
static int jump_stmt_id=0;
static vector<SymbolMap> symbol_table(1);
static map<string,int> func_table;
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
    cout<<"error"<<message<<endl;
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
};

class Var : public BaseAST    // 表示变量
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
            // 由于koopa奇特的规则，决定每次使用变量时都将变量的值移入temp_var中
            Symbol *symbol = findSymbol(id_name);
            if(symbol->declType != ConstDecl)
            {
                symbol->temp_var = genTempVar();
                cout<<"  %"<<symbol->temp_var<<" = load "<<symbol->name<<endl;
            }
        }

        virtual void output()
        {
            // var： 输出此时存在哪个临时变量中
            // const： 输出const的值
            Symbol *symbol = findSymbol(id_name);
            //cout<<symbol->declType<<endl;
            if(symbol->declType == ConstDecl)
            {

                cout<<symbol->value;
            }
            else
            {
                cout<<"%"<<symbol->temp_var;
            }
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
        int declType=VarDecl;
        int type=TypeInt;

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
                    expr->print_koopa();
                    int value = expr->eval();
                    cout<<"  store ";
                    expr->output();
                    cout<<", "<<name<<endl;
                    symbol->value = value;
                }
            }
            else if(declType == ConstDecl)
            {
                int value = expr->eval();
                symbol = new Symbol(ConstDecl,name,type,value);
            }
            else if(declType == ParamDecl)
            {
                symbol = new Symbol(VarDecl,name,type);
                cout<<"  "<<name<<" = alloc i32"<<endl;
                cout<<"  store @"<<id<<", "<<name<<endl;
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
                cout<<"  ";
                Symbol* symbol = findSymbol(var->id_name);
                symbol->value = expr->eval();
                cout<<"store ";
                expr->output();
                cout<<", "<<symbol->name;
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

            cout<<"  br ";
            expr->output();
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
                cout<<", %end_"<<id<<", %else_"<<id<<endl;
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

                var = findSymbol(name)->temp_var;
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

                var = findSymbol(name)->temp_var;
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
                if(defs[i]->declType == ConstDecl)
                {
                    int value = (defs[i]->expr)->eval();
                    symbol = new Symbol(ConstDecl,defs[i]->id,defs[i]->type,value);
                }
                else if(defs[i]->declType == VarDecl)
                {
                    string id = string("@global_") + defs[i]->id;
                    cout<<"global "<<id<<" = alloc i32, ";
                    if(defs[i]->expr)
                    {
                        int value = (defs[i]->expr)->eval();
                        symbol = new Symbol(VarDecl,id,defs[i]->type,value);
                        cout<<value<<endl;
                    }
                    else
                    {
                        symbol = new Symbol(VarDecl,id,defs[i]->type);
                        cout<<0<<endl;
                    }
                }
                symbol_table[0].emplace(defs[i]->id,symbol);
            }
            if(defs[0]->declType == VarDecl)
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
            func_table.emplace(id,func_type);
            ifReturn = 0;
            cout<<"fun @";
            cout<<id<<"(";
            for(int i=0;i < int(params.size())-1;i++)
            {
                cout<<"@"<<params[i]->id<<": ";
                if(params[i]->type == TypeInt)
                    cout<<"i32, ";
            }
            if(params.size() != 0)
            {
                cout<<"@"<<(*params.rbegin())->id<<": ";
                if((*params.rbegin())->type == TypeInt)
                    cout<<"i32";
            }
            cout<<")";
            if(func_type == TypeInt)
                cout<<": i32";
            cout<<" {\n%"<<"entry:\n";
            for(int i=params.size()-1;i >= 0;i--)
                ((Block*)block)->stmts.insert(((Block*)block)->stmts.begin(),(BaseAST*)params[i]);
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
            cout<<"decl @getint(): i32"<<endl;
            func_table.emplace(string("getint"),TypeInt);
            cout<<"decl @getch(): i32"<<endl;
            func_table.emplace(string("getch"),TypeInt);
            cout<<"decl @getarray(*i32): i32"<<endl;
            func_table.emplace(string("getarray"),TypeInt);
            cout<<"decl @putint(i32)"<<endl;
            func_table.emplace(string("putint"),TypeVoid);
            cout<<"decl @putch(i32)"<<endl;
            func_table.emplace(string("putch"),TypeVoid);
            cout<<"decl @putarray(i32, *i32)"<<endl;
            func_table.emplace(string("putarray"),TypeVoid);
            cout<<"decl @starttime()"<<endl;
            func_table.emplace(string("starttime"),TypeVoid);
            cout<<"decl @stoptime()"<<endl;
            func_table.emplace(string("stoptime"),TypeVoid);
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
            int type = func_table[name];
            temp_var = genTempVar();
            for(int i=0;i < params.size();i++)
                params[i]->print_koopa();
            //cout<<type<<endl;
            cout<<"  ";
            if(type == TypeInt)
            {
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
%code requires{
#include <memory>
#include <string>
#include "AST.hpp"
}

%{
#include "AST.hpp"
#include <memory>
#include <string>
#include <iostream>

using namespace std;

extern int yylex();
extern void yyerror(Program **program, const char *s);
%}


%union{
    string *str_val;
    int int_val;
    BaseAST* ast_val;
    DeclareDef *def_val;
    BlockItems *items_val;
    Decls *decl_val;
    Program *pro_val;
    Var *var_val;
}


%parse-param {Program **program}
%token T_Int T_Ret T_Logic_And T_Logic_Or T_Const T_If T_Else T_While T_Break T_Continue
%token T_Void
%token <str_val> T_Ident
%token <int_val> T_Int_Const

%type <ast_val> FuncDef Block Stmt Number Expr AtomExpr AndExpr AddSubExpr MulDivExpr EqualExpr CompareExpr UnaryExpr
%type <ast_val> BlockItem MS UMS IfExpr FuncRealParams CompUnit
%type <ast_val> InitVal InitVals
%type <int_val> UnaryOp AddSubOp MulDivOp CompareOp EqualOp 
%type <int_val> VarType
%type <def_val> ConstDef VarDef FuncParam FuncArrayParam
%type <items_val> BlockItems 
%type <decl_val> VarDecl Decl ConstDecl FuncParams
%type <pro_val> CompUnits
%type <var_val> Var

%%
Program
    :   CompUnits
    {
        #ifdef DEBUG1
        cout<<"CompUnits -> Program"<<endl;
        #endif
        *program = $1;
    }

CompUnits
    :   CompUnit
    {
        #ifdef DEBUG1
        cout<<"CompUnit -> CompUnits"<<endl;
        #endif
        Program *pro = new Program();
        pro->units.push_back($1);
        $$ = pro;
    }
    |   CompUnits CompUnit
    {
        #ifdef DEBUG1
        cout<<"CompUnits CompUnit -> CompUnits"<<endl;
        #endif
        ($1->units).push_back($2);
        $$ = $1;
    }

CompUnit
    :   Decl
    {
        $$ = $1;
    }
    |   FuncDef
    {
        $$ = $1;
    }

FuncDef
    :   VarType T_Ident '(' ')' Block
    {
        #ifdef DEBUG1
        cout<<"VarType T_Ident () Block -> FuncDef"<<endl;
        #endif
        FuncDef* ast = new FuncDef();
        ast->func_type = $1;
        ast->id = *($2);
        delete $2;
        ast->block = $5;
        $$ = (BaseAST*)ast;
    }
    |   VarType T_Ident '(' FuncParams ')' Block
    {
        #ifdef DEBUG1
        cout<<"VarType T_Ident (FuncParams) Block"<<endl;
        #endif
        FuncDef* ast = new FuncDef();
        ast->func_type = $1;
        ast->id = *($2);
        delete $2;
        ast->block = $6;
        ast->params = $4->defs;
        $$ = (BaseAST*)ast;
    }

FuncParams
    :   FuncParam
    {
        #ifdef DEBUG1
        cout<<"T_Int T_Ident -> FuncParams"<<endl;
        #endif
        Decls *decl = new Decls();
        (decl->defs).push_back($1);
        $$ = decl;
    }
    |   FuncParams ',' FuncParam
    {
        ($1->defs).push_back($3);
        $$ = $1;
    }

FuncParam
    :   VarType T_Ident
    {
        DeclareDef *def = new DeclareDef(*($2));
        def->declType = ParamDecl;
        delete $2;
        $$ = def;
    }
    |   FuncArrayParam
    {
        $$ = $1;
    }

FuncArrayParam
    :   VarType T_Ident '[' ']' 
    {
        DeclareDef *def = new DeclareDef(*($2));
        def->declType = ParamDecl;
        def->type = TypePointer;
        delete $2;
        $$ = def;
    }
    |   FuncArrayParam '[' Expr ']'
    {
        ($1->offset).push_back($3);
        $$ = $1;
    }

FuncRealParams
    :   Expr
    {
        FuncCall *call = new FuncCall();
        (call->params).push_back($1);
        $$ = (BaseAST*)call;
    }
    |   FuncRealParams ',' Expr
    {
        (((FuncCall*)$1)->params).push_back($3);
        $$ = $1;
    }

VarType
    :   T_Int
    {
        $$ = TypeInt;
        //printf("VarType\n");
    }
    |   T_Void
    {
        $$ = TypeVoid;
    }


Block
    :   '{' BlockItems '}'
    {
        Block* ast = new Block();
        ast->stmts = $2->vec;
        delete $2;
        $$ = (BaseAST*)ast;
        //printf("block\n");
    };

BlockItems
    :   BlockItems BlockItem
    {
        //BaseAST *ast = $1->vec[($1->vec).size()-1];
        if($2 != NULL)
        {
            if($2->ifDecls())
            {
                Decls *decl = (Decls*)$2;
                for(int i=0;i < decl->defs.size();i++)
                {
                    ($1->vec).insert(($1->vec).end(),(BaseAST*)decl->defs[i]);
                }
            }
            else
                ($1->vec).insert(($1->vec).end(),$2);
        }
        $$ = $1;
    }
    |
    {
        BlockItems *bt = new BlockItems();
        $$ = bt;
    }

BlockItem
    :   Decl
    {
        $$ = (BaseAST*)$1;
    }
    |   Stmt
    {
        $$ = $1;
    }

Decl
    :   ConstDecl ';'
    {
        #ifdef DEBUG1
        cout<<"ConstDecl; -> Decl"<<endl;
        #endif
        $$ = $1;
    }
    |   VarDecl ';'
    {
        #ifdef DEBUG1
        cout<<"VarDecl; -> Decl"<<endl;
        #endif
        $$ = $1;
    }

ConstDecl
    :   T_Const VarType ConstDef
    {
        Decls *decl = new Decls($2);
        $3->type = $2;
        $3->declType = ConstDecl;
        decl->defs.insert((decl->defs).end(),$3);
        $$ = decl;

        #ifdef DEBUG1
        cout<<"T_Const VarType ConstDef -> ConstDecl"<<endl;
        #endif
    }
    |   ConstDecl ',' ConstDef
    {
        $3->type = $1->type;
        $3->declType = ConstDecl;
        ($1->defs).insert(($1->defs).end(),$3);
        $$ = $1;

        #ifdef DEBUG1
        cout<<"ConstDecl , ConstDef -> ConstDecl"<<endl;
        #endif
    }

VarDecl
    :   VarType VarDef
    {
        #ifdef DEBUG1
        cout<<"VarType VarDef -> VarDecl"<<endl;
        #endif

        Decls *decl = new Decls($1);
        $2->type = $1;
        $2->declType = VarDecl;
        decl->defs.insert((decl->defs).end(),$2);
        $$ = decl;
    }
    |   VarDecl ',' VarDef
    {
        #ifdef DEBUG1
        cout<<"VarDecl , VarDef -> VarDecl"<<endl;
        #endif

        $3->type = $1->type;
        $3->declType = VarDecl;
        ($1->defs).insert(($1->defs).end(),$3);
        $$ = $1;
    }

VarDef
    :   Var
    {
        #ifdef DEBUG1
        cout<<"T_Ident -> VarDef"<<endl;
        #endif

        DeclareDef *def = new DeclareDef($1->id);
        def->offset = $1->offset;
        delete $1;
        $$ = def;
    }
    |   Var '=' InitVal
    {
        #ifdef DEBUG1
        cout<<"T_Ident = Expr -> VarDef"<<endl;
        #endif

        DeclareDef *def = new DeclareDef($1->id,$3);
        def->offset = $1->offset;
        delete $1;
        $$ = def;
    }

ConstDef
    :  Var '=' InitVal
    {
        #ifdef DEBUG1
        cout<<"T_Ident = Expr -> ConstDef"<<endl;
        #endif

        DeclareDef *def = new DeclareDef($1->id,$3);
        def->offset = $1->offset;
        delete $1;
        $$ = def;
    }

InitVals
    :   InitVal
    {
        InitVal *init = new InitVal();
        init->inits.push_back($1);
        $$ = init;

        #ifdef DEBUG1
        cout<<"InitVal -> InitVals"<<endl;
        #endif
    }
    |   InitVals ',' InitVal
    {
        ((InitVal*)$1)->inits.push_back($3);
        $$ = $1;
        #ifdef DEBUG1
        cout<<"InitVals , InitVal -> InitVals"<<endl;
        #endif
    }

InitVal
    :   Expr
    {
        #ifdef DEBUG1
        cout<<"Expr -> InitVal"<<endl;
        #endif
        $$ = $1;
    }
    |   '{' InitVals '}'
    {
        #ifdef DEBUG1
        cout<<"{ InitVals } -> InitVal"<<endl;
        #endif
        $$ = $2;
    }
    |   '{' '}'
    {
        #ifdef DEBUG1
        cout<<"{} -> InitVal"<<endl;
        #endif
        $$ = NULL;
    }

Var
    :   T_Ident
    {
        #ifdef DEBUG1
        cout<<"T_Ident -> Var"<<endl;
        #endif
        Var *var = new Var(*($1));
        delete $1;
        $$ = var;
    }
    |   Var '[' Expr ']'
    {
        #ifdef DEBUG1
        cout<<"Var[Expr] -> Var"<<endl;
        #endif
        $1->offset.push_back($3);
        $$ = $1;
    }

Stmt
    :   MS
    {
        $$ = $1;
    }
    |   UMS
    {
        $$ = $1;
    }

MS
    :   T_Ret Expr ';'
    {
        #ifdef DEBUG1
        cout<<"T_Ret Expr; -> Stmt"<<endl;
        #endif

        Stmt* ast = new Stmt($2,Return);
        $$ = (BaseAST*)ast;
        //printf("stmt\n");
    }
    |   T_Ret ';'
    {
        Stmt* ast = new Stmt(NULL,Return);
        $$ = (BaseAST*)ast;
    }
    |   Var '=' Expr ';'   //前面的Expr只能是
    {
        #ifdef DEBUG1
        cout<<"T_Ident = Expr; -> Stmt"<<endl;
        #endif
        Stmt *ast = new Stmt($3,Assign,(Var*)$1);
        $$ = (BaseAST*)ast;
    }
    |   Block
    {
        #ifdef DEBUG1
        cout<<"Block -> Stmt"<<endl;
        #endif
        $$ = $1;
    }
    |   Expr ';'
    {
        #ifdef DEBUG1
        cout<<"Expr; -> Stmt"<<endl;
        #endif
        Stmt *ast = new Stmt($1,Other);
        $$ = (BaseAST*)ast;
    }
    |   ';'
    {
        #ifdef DEBUG1
        cout<<"Stmt; -> Stmt"<<endl;
        #endif
        $$ = NULL;
    }
    |   IfExpr MS T_Else MS
    {
        JumpStmt *ast = new JumpStmt($1,$2,$4);
        $$ = (BaseAST*)ast;
    }
    |   T_While '(' Expr ')' Stmt
    {
        #ifdef DEBUG1
        cout<<"T_While (Expr) Stmt -> Stmt"<<endl;
        #endif
        WhileStmt *stmt = new WhileStmt($3,$5);
        $$ = (BaseAST*)stmt;
    }
    |   T_Break ';'
    {
        #ifdef DEBUG1
        cout<<"T_Break; -> Stmt"<<endl;
        #endif
        Stmt *stmt = new Stmt(NULL,Break,NULL);
        $$ = (BaseAST*)stmt;
    }
    |   T_Continue ';'
    {
        Stmt *stmt = new Stmt(NULL,Continue,NULL);
        $$ = (BaseAST*)stmt;
    }

UMS
    :   IfExpr Stmt
    {
        JumpStmt *ast = new JumpStmt($1,$2);
        $$ = (BaseAST*)ast;
    }
    |   IfExpr MS T_Else UMS
    {
        JumpStmt *ast = new JumpStmt($1,$2,$4);
        $$ = (BaseAST*)ast;
    }

IfExpr
    :   T_If '(' Expr ')'
    {
        $$ = $3;
    }

Expr
    :   AndExpr
    {
        #ifdef DEBUG1
        cout<<"AndExpr -> Expr"<<endl;
        #endif
        $$ = $1;
    }
    |   Expr T_Logic_Or AndExpr    //最低的运算符优先级: ||
    {
        BaseAST *ast1,*ast2;
        if(!($1->ifBoolean()))
            ast1 = (BaseAST*)new Expr($1,NULL,NotEqualZero);
        else
            ast1 = $1;
        if(!($3->ifBoolean()))
            ast2 = (BaseAST*)new Expr($3,NULL,NotEqualZero);
        else
            ast2 = $3;
        Expr *ast = new Expr(ast2,ast1,Or);
        $$ = (BaseAST*)ast;
    }

AndExpr
    :   EqualExpr
    {
        $$ = $1;
    }
    |   AndExpr T_Logic_And EqualExpr
    {
        BaseAST *ast1,*ast2;
        if(!($1->ifBoolean()))
            ast1 = (BaseAST*)new Expr($1,NULL,NotEqualZero);
        else
            ast1 = $1;
        if(!($3->ifBoolean()))
            ast2 = (BaseAST*)new Expr($3,NULL,NotEqualZero);
        else
            ast2 = $3;
        Expr *ast = new Expr(ast2,ast1,And);
        $$ = (BaseAST*)ast;
    }

EqualExpr
    :   CompareExpr
    {
        $$ = $1;
    }
    |   EqualExpr EqualOp CompareExpr
    {
        Expr *ast = new Expr($3,$1,$2);
        $$ = (BaseAST*)ast;
    }

CompareExpr
    :   AddSubExpr
    {
        $$ = $1;
    }
    |   CompareExpr CompareOp AddSubExpr
    {
        Expr *ast = new Expr($3,$1,$2);
        $$ = (BaseAST*)ast;
    }

AddSubExpr
    :   MulDivExpr
    {
        $$ = $1;
    }
    |   AddSubExpr AddSubOp MulDivExpr
    {
        Expr *ast = new Expr($3,$1,$2);
        $$ = (BaseAST*)ast;
    }

MulDivExpr
    :   UnaryExpr
    {
        $$ = $1;
    }
    |   MulDivExpr MulDivOp UnaryExpr
    {
        Expr *ast = new Expr($3,$1,$2);
        $$ = (BaseAST*)ast;
    };

UnaryExpr
    :   AtomExpr
    {
        $$ = $1;
    }
    |   UnaryOp UnaryExpr
    {
        //cout<<"UnaryOp AtomExpr -> AtomExpr"<<endl;
        if($1 == NoOperation)
            $$ = $2;
        else
        {
            Expr* ast = new Expr($2,NULL,$1);
            $$ = (BaseAST*)ast;
        }
    }

AtomExpr
    :   '(' Expr ')'
    {
        $$ = $2;
    }
    |   Number
    {
        #ifdef DEBUG1
        cout<<"Number -> AtomExpr"<<endl;
        #endif
        $$ = $1;
    }
    |   Var
    {
        #ifdef DEBUG1
        cout<<"T_Ident -> Expr"<<endl;
        #endif
        $$ = $1;
    }
    |   T_Ident '(' FuncRealParams ')'
    {
        ((FuncCall*)$3)->name = *($1);
        delete $1;
        $$ = $3;
    }
    |   T_Ident '(' ')'
    {
        FuncCall *call = new FuncCall();
        call->name = *($1);
        delete $1;
        $$ = (BaseAST*)call;
    }
    
UnaryOp
    :   '+'
    {
        $$ = NoOperation;
    }
    |   '-'
    {
        $$ = Invert;
    }
    |   '!'
    {
        $$ = EqualZero;
    };

AddSubOp
    :   '+'
    {
        $$ = Add;
    }
    |   '-'
    {
        $$ = Sub;
    };

MulDivOp
    :   '*'
    {
        $$ = Mul;
    }
    |   '/'
    {
        $$ = Div;
    }
    |   '%'
    {
        $$ = Mod;
    }

CompareOp
    :   '<' '='
    {
        $$ = LessEq;
    }
    |   '<'
    {
        $$ = Less;
    }
    |   '>'
    {
        $$ = Greater;
    }
    |   '>' '='
    {
        $$ = GreaterEq;
    }

EqualOp
    :   '=' '='
    {
        $$ = Equal;
    }
    |   '!' '='
    {
        $$ = NotEqual;
    }

Number
    :   T_Int_Const
    {
        Number* ast = new Number();
        ast->num = $1;
        $$ = (BaseAST*)ast;
        //printf("number\n");
    };
%%

extern void yyerror(Program **program,const char* s)
{
    cout<<"error: "<<s<<endl;
}
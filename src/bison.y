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
extern void yyerror(std::unique_ptr<CompUnit> &comp, const char *s);
%}


%union{
    std::string* str_val;
    int int_val;
    BaseAST* ast_val;
}


%parse-param {std::unique_ptr<CompUnit> &comp}
%token T_Int T_Ret T_Logic_And T_Logic_Or
%token <str_val> T_ID
%token <int_val> T_Int_Const 

%type <ast_val> FuncDef FuncType Block Stmt Number Expr AtomExpr AndExpr AddSubExpr MulDivExpr EqualExpr CompareExpr UnaryExpr
%type <int_val> UnaryOp AddSubOp MulDivOp CompareOp EqualOp

%%
CompUnit
    :   FuncDef
    {
        auto ast = std::make_unique<CompUnit>();
        ast->func_def = $1;
        comp = std::move(ast);
        //printf("Comp\n");
    };

FuncDef
    :   FuncType T_ID '(' ')' Block
    {
        //printf("FuncDef\n");
        FuncDef* ast = new FuncDef();
        ast->func_type = $1;
        ast->id = $2;
        ast->block = $5;
        $$ = (BaseAST*)ast;
    };

FuncType
    :   T_Int
    {
        FuncType* ast = new FuncType();
        ast->type_name = "i32";
        $$ = (BaseAST*)ast;
        //printf("FuncType\n");
    };

Block
    :   '{' Stmt '}'
    {
        Block* ast = new Block();
        ast->stmt = $2;
        $$ = (BaseAST*)ast;
        //printf("block\n");
    };

Stmt
    :   T_Ret Expr ';'
    {
        Stmt* ast = new Stmt();
        ast->expr = $2;
        $$ = (BaseAST*)ast;
        //printf("stmt\n");
    };

Expr
    :   AndExpr
    {
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
        //cout<<"Number -> AtomExpr"<<endl;
        Expr* ast = new Expr($1);
        $$ = (BaseAST*)ast;
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

extern void yyerror(unique_ptr<CompUnit> &comp,const char* s)
{
    cout<<"error:"<<s<<endl;
}
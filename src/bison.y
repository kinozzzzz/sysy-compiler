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
    string *str_val;
    int int_val;
    BaseAST* ast_val;
    DeclareDef *def_val;
    BlockItems *items_val;
    Decls *decl_val;
}


%parse-param {std::unique_ptr<CompUnit> &comp}
%token T_Int T_Ret T_Logic_And T_Logic_Or T_Const T_If T_Else
%token <str_val> T_Ident
%token <int_val> T_Int_Const

%type <ast_val> FuncDef FuncType Block Stmt Number Expr AtomExpr AndExpr AddSubExpr MulDivExpr EqualExpr CompareExpr UnaryExpr
%type <ast_val> BlockItem MS UMS IfExpr
%type <int_val> UnaryOp AddSubOp MulDivOp CompareOp EqualOp 
%type <int_val> VarType
%type <def_val> ConstDef VarDef
%type <items_val> BlockItems
%type <decl_val> VarDecl Decl ConstDecl

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
    :   FuncType T_Ident '(' ')' Block
    {
        //printf("FuncDef\n");
        FuncDef* ast = new FuncDef();
        ast->func_type = $1;
        ast->id = *($2);
        delete $2;
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

VarType
    :   T_Int
    {
        $$ = TypeInt;
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
        $$ = $1;
    }
    |   VarDecl ';'
    {
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
        cout<<$3->id<<symbol_table.size()<<symbol_table[$3->id]->value<<endl;
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
        cout<<$3->id<<symbol_table.size()<<symbol_table[$3->id]->value<<endl;
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
    :   T_Ident
    {
        #ifdef DEBUG1
        cout<<"T_Ident -> VarDef"<<endl;
        #endif

        DeclareDef *def = new DeclareDef(*($1));
        delete $1;
        $$ = def;
    }
    |   T_Ident '=' Expr
    {
        #ifdef DEBUG1
        cout<<"T_Ident = Expr -> VarDef"<<endl;
        #endif

        DeclareDef *def = new DeclareDef(*($1),$3);
        delete $1;
        $$ = def;
    }

ConstDef
    :   T_Ident '=' Expr
    {
        #ifdef DEBUG1
        cout<<"T_Ident = Expr -> ConstDef"<<endl;
        #endif

        DeclareDef *def = new DeclareDef(*($1),$3);
        delete $1;
        $$ = def;
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
    |   T_Ident '=' Expr ';'
    {
        #ifdef DEBUG1
        cout<<"T_Ident = Expr; -> Stmt"<<endl;
        #endif
        Stmt *ast = new Stmt($3,Assign,new Var(*($1)));
        delete $1;
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
        $$ = $1;
    }
    |   T_Ident
    {
        Var *ast = new Var(*($1));
        delete $1;
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
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
%token T_Int T_Ret
%token <str_val> T_ID
%token <int_val> T_Int_Const 

%type <ast_val> FuncDef FuncType Block Stmt Number


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
    :   T_Ret Number ';'
    {
        Stmt* ast = new Stmt();
        ast->number = $2;
        $$ = (BaseAST*)ast;
        //printf("stmt\n");
    };

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
#include "AST.hpp"
#include <memory>
#include <string.h>
#include <stdio.h>

extern FILE *yyin;
extern int yyparse(std::unique_ptr<CompUnit> &comp);
extern void yyerror(std::unique_ptr<CompUnit> &comp,const char*);

int main(int argc,char* argv[])
{
    char *in_file = (char*)"../source.cpp";
    char *out_file = (char*)"../output.txt";
    int omode = KOOPA_MODE;
    for(int i=1;i < argc;i++)
    {
        printf("%d %s\n",i,argv[i]);
        if(!memcmp(argv[i],"-koopa",6))
        {
            omode = KOOPA_MODE;
        }
        else if(!memcmp(argv[i],"-riscv",6))
        {
            omode = RISCV_MODE;
        }
        else if(!memcmp(argv[i],"-o",2))
        {
            out_file = argv[++i];
        }
        else
        {
            in_file = argv[i];
        }
    }
    freopen(in_file,"r",stdin);
    freopen(out_file,"w",stdout);

    auto answer = std::make_unique<CompUnit>();
    yyparse(answer);
    answer->print();
}
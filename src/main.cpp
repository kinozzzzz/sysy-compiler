#include "AST.hpp"
#include <memory>
#include <string>
#include <stdio.h>

extern FILE *yyin;
extern int yyparse(std::unique_ptr<CompUnit> &comp);
extern void yyerror(std::unique_ptr<CompUnit> &comp,const char*);

int main(int argc,char* argv[])
{
    if(argc%2 != 1)
    {
        cout<<"unmatched parameters\n";
        return 0;
    }
    for(int i=1;i < argc;i++)
    {
        if(string(argv[i]) == string("-koopa"))
        {
            freopen(argv[++i], "r", stdin); 
        }
        if(string(argv[i]) == string("-o"))
        {
            freopen(argv[++i], "w", stdout); 
        }
    }
    auto answer = std::make_unique<CompUnit>();
    yyparse(answer);
    answer->print();
}
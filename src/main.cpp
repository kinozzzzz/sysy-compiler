#include "AST.hpp"
#include "riscv.hpp"
#include <memory>
#include <string.h>
#include <stdio.h>

extern FILE *yyin;
extern int yyparse(Program **program);
extern void yyerror(Program **program,const char*);


int main(int argc,char* argv[])
{
    char *in_file = (char*)"../source.cpp";
    char *out_file = (char*)"../output.txt";
    int omode = RISCV_MODE;
    
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
    Program * answer;
    yyparse(&answer);
    #ifdef DEBUG
    cout<< "finish yyparse()"<<endl;
    #endif
    if(omode == KOOPA_MODE)
    {
        #ifndef DEBUG
        freopen(out_file,"w",stdout);
        #endif
        answer->print_koopa();
    }
    if(omode == RISCV_MODE)
    {
        freopen("../koopa.txt","w",stdout);
        answer->print_koopa();
        cout<<endl;
        FILE* ff=fopen("../koopa.txt","r");
        char *buf=(char *)malloc(1000000);
        fread(buf, 1,1000000, ff);
        freopen(out_file,"w",stdout);
        print_riscv(buf);
    }
}
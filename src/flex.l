%option noyywrap
%option nounput
%option noinput

%{
#include <string>
#include "bison.tab.hpp"

using namespace std;
%}

WhiteSpace      [ \t\n\r]*
Comment         "//"[^\n]*
Identifier      [a-zA-Z_][a-zA-Z0-9_]*
Decimal         [1-9][0-9]*
Octal           0[0-7]*
Hexadecimal     0[xX][0-9a-fA-F]+
olc             [/][*]([^*]|[*][^/])*[*]?[*][/]


%%
{WhiteSpace}    {/* do nothing */}
{Comment}       {/* do nothing */}
{olc}           {/* do nothing */}

"int"           {return T_Int;}
"return"        {return T_Ret;}

{Identifier}    {yylval.str_val = new string(yytext);return T_ID;}
{Decimal}       {yylval.int_val = strtol(yytext,nullptr,0);return T_Int_Const;}
{Octal}         {yylval.int_val = strtol(yytext,nullptr,0);return T_Int_Const;}
{Hexadecimal}   {yylval.int_val = strtol(yytext,nullptr,0);return T_Int_Const;}

.               {return yytext[0];}
%%
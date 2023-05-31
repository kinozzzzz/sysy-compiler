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
CommentLines    \/\*(?:[^\*]|\*+[^\/\*])*\*+\/


%%
{WhiteSpace}    {/* do nothing */}
{Comment}       {/* do nothing */}
{CommentLines}  {/* do nothing */}

"int"           {return T_Int;}
"return"        {return T_Ret;}
"||"            {return T_Logic_Or;}
"&&"            {return T_Logic_And;}
"const"         {return T_Const;}
"if"            {return T_If;}
"else"          {return T_Else;}
"while"         {return T_While;}
"break"         {return T_Break;}
"continue"      {return T_Continue;}
"void"          {return T_Void;}

{Identifier}    {yylval.str_val = new string(yytext);return T_Ident;}
{Decimal}       {yylval.int_val = strtol(yytext,nullptr,0);return T_Int_Const;}
{Octal}         {yylval.int_val = strtol(yytext,nullptr,0);return T_Int_Const;}
{Hexadecimal}   {yylval.int_val = strtol(yytext,nullptr,0);return T_Int_Const;}

.               {return yytext[0];}
%%
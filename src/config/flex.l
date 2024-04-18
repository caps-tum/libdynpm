%{
#include "bison.h"
#include "assert.h"
#include "dynpm_config.h"
%}

%option noyywrap

%%
[0-9]+       { yylval.value = atoi(yytext); return number; }
[a-zA-Z0-9_]*    { assert(strlen(yytext) < CONFIG_MAX_STR_LEN); strcpy(yylval.conf_string, yytext); return string_val; }
"="          { yylval.symbol = yytext[0]; return equals; }
"{"          { yylval.symbol = yytext[0]; return left; }
"}"          { yylval.symbol = yytext[0]; return right; }
","          { yylval.symbol = yytext[0]; return comma; }
":"          { yylval.symbol = yytext[0]; return colon; }
<<EOF>>      { return 0; }
[ \t\n]+     { }
"#"[^\n]*    { }
.            { printf("Unrecognized token!\n"); exit(1); }
%%

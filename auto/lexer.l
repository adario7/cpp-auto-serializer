/* yywrap can be used for scanning mutliple files, see:
 * https://stackoverflow.com/questions/1811125/undefined-reference-to-yywrap
*/
%option noyywrap
%option yylineno

/* multiline comment state */
%x COMMENT

%{
#include <string>
#include <node.hh>
#include <parser.hh>

int yycolumn = 1;
int prev_yylineno = yylineno;

#define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
#define TOKEN(t) (yylval.token = t)

#define YY_USER_ACTION                                                   \
  yylloc.first_line = prev_yylineno; yylloc.first_column = yycolumn;     \
  if (yylineno == prev_yylineno) yycolumn += yyleng;                     \
  else {                                                                 \
    for (yycolumn = 1; yytext[yyleng - yycolumn] != '\n'; ++yycolumn) {} \
    prev_yylineno = yylineno;                                            \
  }                                                                      \
  yylloc.last_line = yylineno; yylloc.last_column = yycolumn-1;

%}

%%

[ \t\n]                 { }
	
	/* shortcuts to avoid writing `` */
"public:"                   { SAVE_TOKEN; return T_CODE; }
"protected:"                { SAVE_TOKEN; return T_CODE; }
"private:"                  { SAVE_TOKEN; return T_CODE; }
"public"                    { SAVE_TOKEN; return T_CODE; }
"protected"                 { SAVE_TOKEN; return T_CODE; }
"private"                   { SAVE_TOKEN; return T_CODE; }

"struct"                    { return TOKEN(T_STRUCT); }
"class"                     { return TOKEN(T_CLASS); }
"polymorphic"               { return TOKEN(T_POLYM); }
"virtual"                   { return TOKEN(T_VIRTUAL); }

	/* put `::` inside the identifiers */
[a-zA-Z_](::|[a-zA-Z0-9_])* { SAVE_TOKEN; return T_IDENTIFIER; }
`([^`]|``)*`                { yylval.string = new std::string(yytext+1, yyleng-2); return T_CODE; }

"{"                         { return TOKEN(T_L_BRACE); }
"}"                         { return TOKEN(T_R_BRACE); }
"<"                         { return TOKEN(T_L_ACUTE); }
">"                         { return TOKEN(T_R_ACUTE); }
";"                         { return TOKEN(T_SEMIC); }
":"                         { return TOKEN(T_COLONS); }
","                         { return TOKEN(T_COMMA); }
"="                         { return TOKEN(T_EQ); }
"*"                         { return TOKEN(T_STAR); }
"["                         { return TOKEN(T_OPEN_SQ); }
"]"                         { return TOKEN(T_CLOSE_SQ); }

"//".*                      { /* single line comment */ }
"/*"                        { BEGIN(COMMENT); }
<COMMENT>"*/"               { BEGIN(INITIAL); }
<COMMENT>.                  { }
<COMMENT>\n                 { }

.                           { throw std::runtime_error(std::string("Unknown token: '") + yytext + "' at line " + std::to_string(prev_yylineno)); yyterminate(); }

%%

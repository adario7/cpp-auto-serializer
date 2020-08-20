%{
    #include <iostream>
    #include <node.hh>
    #include <parser.hh>

    extern void compileRoot(NCode*);
    extern void compileRoot(NStruct*);
    extern void compileRoot(NPolym*);
    extern void compileRoot(NAlias*);
    extern int yylineno; 

    int yylex();
    void yyerror(const char *s) {
        std::cout << "ERROR from " << yylloc.first_line << "/" << yylloc.first_column << " to " << yylloc.last_line << "/" << yylloc.last_column << ": " << s << std::endl;
        exit(1);
    }

    // @$ is expanded by bison, so it cannot be within the macro
    #define _P(x) segment_t{{x.first_line, x.first_column}, {x.last_line, x.last_column}}
%}

%locations
%define parse.error verbose

/* Represents the many different ways we can access our data */
%union {
    Node* node;

    NIdentifier* ident;
    NVarDeclaration* var_decl;
    NVarBlock* vars_block;
    NType* type;
    NCode* code;
    NStruct* nstruct;
    NRoot* root;
    NBodyElem* body_elem;
    NPolym* polym;
	NPolymElem* polym_elem;
    NParent* parent_class;
    NAlias* alias;

    TypeSuffixList* type_suffixes;
    ArraySuffixList* array_suff;
    VarDeclList* vars_list;
    GenericsList* generics_list;
    BodyList* body_list;
    ParentsList* parents_list;
	PolymList* polym_list;

    std::string* string;
    bool boolean;
    int token;
}

/* Define our terminal symbols (tokens). should
   match our lex file. We also define the node type
   they represent. */
%token END 0 "end of file"
%token <string> T_CODE "code literal (`...`)" T_IDENTIFIER "identifier"
%token <string> T_LOCAL_INC "local include" T_SYSTEM_INC "system include"
%token <token> T_STRUCT "struct" T_CLASS "class"
%token <token> T_POLYM "polymorphic" T_VIRTUAL "virtual" T_USING "using" T_ALIAS "alias"
%token <token> T_COMMA "," T_DOT "." T_SEMIC ";" T_AT "@" T_EQ "=" T_STAR "*" T_COLONS ":" 
%token <token> T_L_BRACE "{" T_R_BRACE "}" T_L_ACUTE "<" T_R_ACUTE ">" T_OPEN_SQ "[" T_CLOSE_SQ "]"

/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above.  */
%type <token> type_suffix
%type <type_suffixes> type_suffixes type_suff_0
%type <type> decl_type compl_type
%type <ident> ident
%type <vars_block> vars_block
%type <var_decl> var_decl
%type <code> root_code 
%type <string> code_block code_block_0
%type <boolean> cls_or_struct virtual_as_bool using_or_alias
%type <body_elem> body_elem
%type <nstruct> nstruct
%type <polym> polym
%type <polym_elem> polym_elem
%type <alias> alias
%type <parent_class> parent_class
%type <array_suff> array_suff array_suff_0
%type <generics_list> generics_content
%type <body_list> struct_body struct_body_0
%type <vars_list> var_decls
%type <parents_list> parents_list parents_list_c
%type <polym_list> polym_list

%start program

%%

program : rootelems { } ;
        
rootelems : rootelem
          | rootelems rootelem
          ;

rootelem : nstruct { compileRoot($1); delete $1; }
         | root_code { compileRoot($1); delete $1; }
         | polym { compileRoot($1);  /* don't delete, as the data is persistent */ }
         | alias { compileRoot($1); }
         ;

/* at least one block of code */
code_block : T_CODE { $$ = $1; }
           | code_block T_CODE { $$ = $1; *$$ += " " + *$2; delete $2; }
           ;

/* 0 or more blocks */
code_block_0 : %empty { $$ = nullptr; }
             | code_block { $$ = $1; }
             ;

/* don't use `code_block` as this would generate a parser conflict:
 * shall the codes be merged by `code_block` or be separate `root_code`s?*/
root_code : T_CODE { $$ = new NCode(_P(@$), $1); } ;

type_suffix : "*" ;  /* references not supported */

type_suffixes : type_suffix { $$ = new TypeSuffixList(); $$->push_back($1); } 
              | type_suffixes type_suffix { $1->push_back($2); } 
              ;

type_suff_0 : type_suffixes
            | %empty { $$ = new TypeSuffixList(); }
            ;

array_suff : "[" T_CODE "]" { $$ = new ArraySuffixList(); $$->push_back($2); }
           | array_suff "[" T_CODE "]"  { $$ = $1; $$->push_back($3); }
           ;

array_suff_0 : %empty { $$ = new ArraySuffixList(); }
             | array_suff { $$ = $1; }
             ;

generics_content : compl_type { $$ = new GenericsList(); $$->push_back($1); }
                 | generics_content "," compl_type { $1->push_back($3); }
                 ;

/* a complete type, it may include '*' and '[]' */
compl_type : ident "<" generics_content ">" { $$ = new NType(_P(@$), $1, $3); }
           | ident { $$ = new NType(_P(@$), $1, new GenericsList()); }
           | compl_type "*" { $$ = NType::pointerTo($1, _P(@$)); }
           | compl_type "[" T_CODE "]" { $$ = NType::arrayOf($1, $3, _P(@$)); }
           ;

/* type for declarations: '*' and '[]' are bound to the variable name */
decl_type : ident "<" generics_content ">" { $$ = new NType(_P(@$), $1, $3); }
          | ident { $$ = new NType(_P(@$), $1, new GenericsList()); }
          ;

ident : T_IDENTIFIER { $$ = new NIdentifier(_P(@$), std::move(*$1)); delete $1; }
      ;

vars_block : decl_type var_decls T_SEMIC { $$ = new NVarBlock(_P(@$), $1, $2); }
          ;

var_decls : var_decl { $$ = new VarDeclList(); $$->push_back($<var_decl>1); }
          | var_decls T_COMMA var_decl { $1->push_back($<var_decl>3); }
          ;

var_decl : type_suff_0 ident array_suff_0 { $$ = new NVarDeclaration(_P(@$), $1, $2, $3, nullptr); }
         | type_suff_0 ident array_suff_0 "=" code_block { $$ = new NVarDeclaration(_P(@$), $1, $2, $3, $5); }
         ;

cls_or_struct : T_CLASS { $$ = true; }
              | T_STRUCT { $$ = false; }
              ;

/* the body of a class has vars declarations and copied-over code blocks */
body_elem : root_code { $$ = $1; }
          | vars_block { $$ = $1; }
          ;

struct_body : body_elem { $$ = new BodyList(); $$->push_back($1); }
            | struct_body body_elem { $1->push_back($2); }

struct_body_0 : struct_body 
              | %empty { $$ = new BodyList(); }
              ;

/* assume not virtual if there is no attribute */
virtual_as_bool : "virtual" { $$ = true; }
                | %empty { $$ = false; } ;

parent_class : code_block_0 decl_type { $$ = new NParent(_P(@$), $1, $2); }

parents_list_c : parent_class { $$ = new ParentsList(); $$->push_back($1); }
               | parents_list_c "," parent_class { $1->push_back($3); }
               ;

parents_list : ":" parents_list_c { $$ = $2; }  
             | %empty { $$ = new ParentsList(); }
             ;

nstruct : virtual_as_bool cls_or_struct decl_type parents_list code_block_0 T_L_BRACE struct_body_0 T_R_BRACE T_SEMIC
                { $$ = new NStruct(_P(@$), $1, $2, $3, $4, $5, $7); } ; 

polym_elem : compl_type { $$ = new NPolymElem(_P(@$), $1, false, nullptr); }
		   | compl_type "local include" { $$ = new NPolymElem(_P(@$), $1, true, $2); }
		   | compl_type "system include" { $$ = new NPolymElem(_P(@$), $1, false, $2); }
		   ;

polym_list : polym_elem { $$ = new PolymList(); $$->push_back($1); }
           | polym_list "," polym_elem { $1->push_back($3); }
           ;

polym : T_POLYM decl_type ":" polym_list T_SEMIC { $$ = new NPolym(_P(@$), $2, $4); }
      ;

using_or_alias : "using" { $$ = true; }
               | "alias" { $$ = false; }
               ;

alias : using_or_alias decl_type "=" compl_type T_SEMIC { $$ = new NAlias(_P(@$), $1, $2, $4); }
      ;

%%

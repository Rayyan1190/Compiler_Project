
%{
  #include <cstdio>
  #include <cstdlib>
  // Bison will call these:
  int yylex(void);
  void yyerror(const char* s) { std::fprintf(stderr, "Parse error: %s\n", s); }
%}

%define parse.error verbose

/* ---------- Tokens (names MUST match your TokenType) ---------- */
%token T_FUNCTION T_RETURN T_IF T_ELSE T_FOR T_WHILE
%token T_INT T_FLOAT T_BOOL T_STRING T_CHAR
%token T_ANDAND T_OROR
%token T_EQUALSOP T_NOTEQ
%token T_LE T_GE T_SHL T_SHR
%token T_ASSIGNOP T_LT T_GT T_NOT
%token T_PLUS T_MINUS T_STAR T_SLASH T_PERCENT
%token T_AMP T_PIPE T_CARET T_TILDE
%token T_PARENL T_PARENR T_BRACEL T_BRACER T_BRACKETL T_BRACKETR
%token T_COMMA T_SEMICOLON T_DOT
%token T_INTLIT T_FLOATLIT T_STRINGLIT T_CHARLIT T_IDENTIFIER

/* ---------- Precedence (same as your recursive-descent parser) ---------- */
%right T_ASSIGNOP
%left  T_OROR
%left  T_ANDAND
%left  T_PIPE
%left  T_CARET
%left  T_AMP
%left  T_EQUALSOP T_NOTEQ
%left  T_LT T_LE T_GT T_GE
%left  T_SHL T_SHR
%left  T_PLUS T_MINUS
%left  T_STAR T_SLASH T_PERCENT
%right T_NOT T_TILDE
%precedence UMINUS UPLUS

%start program

%%  /* ======================= Grammar ======================= */

/* program and top-level declarations */
program
    : top_list
    ;

top_list
    : /* empty */
    | top_list top
    ;

top
    : function
    | var_decl T_SEMICOLON
    ;

/* functions: fn ident '(' params? ')' block */
function
    : T_FUNCTION T_IDENTIFIER T_PARENL param_list_opt T_PARENR block
    ;

param_list_opt
    : /* empty */
    | param_list
    ;

param_list
    : param
    | param_list T_COMMA param
    ;

param
    : type T_IDENTIFIER
    ;

/* types: int|float|bool|string|char */
type
    : T_INT
    | T_FLOAT
    | T_BOOL
    | T_STRING
    | T_CHAR
    ;

/* blocks and statements */
block
    : T_BRACEL stmt_list_opt T_BRACER
    ;

stmt_list_opt
    : /* empty */
    | stmt_list
    ;

stmt_list
    : stmt
    | stmt_list stmt
    ;

stmt
    : block
    | T_IF T_PARENL expr T_PARENR stmt else_opt
    | T_WHILE T_PARENL expr T_PARENR stmt
    | T_FOR T_PARENL for_init_opt T_SEMICOLON for_cond_opt T_SEMICOLON for_incr_opt T_PARENR stmt
    | T_RETURN expr_opt T_SEMICOLON
    | var_decl T_SEMICOLON
    | expr T_SEMICOLON
    ;

else_opt
    : /* empty */
    | T_ELSE stmt
    ;

for_init_opt
    : /* empty */
    | var_decl
    | expr
    ;

for_cond_opt
    : /* empty */
    | expr
    ;

for_incr_opt
    : /* empty */
    | expr
    ;

expr_opt
    : /* empty */
    | expr
    ;

/* variable declarations: type ident ('[' expr? ']')* ('=' expr)? */
var_decl
    : type T_IDENTIFIER array_decl_list_opt init_opt
    ;

array_decl_list_opt
    : /* empty */
    | array_decl_list
    ;

array_decl_list
    : array_decl
    | array_decl_list array_decl
    ;

array_decl
    : T_BRACKETL expr_opt T_BRACKETR
    ;

init_opt
    : /* empty */
    | T_ASSIGNOP expr
    ;

/* expressions */
expr
    : assignment
    ;

assignment
    : or_expr
    | or_expr T_ASSIGNOP assignment
    ;

or_expr
    : and_expr
    | or_expr T_OROR and_expr
    ;

and_expr
    : bitor_expr
    | and_expr T_ANDAND bitor_expr
    ;

bitor_expr
    : bitxor_expr
    | bitor_expr T_PIPE bitxor_expr
    ;

bitxor_expr
    : bitand_expr
    | bitxor_expr T_CARET bitand_expr
    ;

bitand_expr
    : equality
    | bitand_expr T_AMP equality
    ;

equality
    : rel_expr
    | equality T_EQUALSOP rel_expr
    | equality T_NOTEQ  rel_expr
    ;

rel_expr
    : shift_expr
    | rel_expr T_LT shift_expr
    | rel_expr T_LE shift_expr
    | rel_expr T_GT shift_expr
    | rel_expr T_GE shift_expr
    ;

shift_expr
    : add_expr
    | shift_expr T_SHL add_expr
    | shift_expr T_SHR add_expr
    ;

add_expr
    : mul_expr
    | add_expr T_PLUS  mul_expr
    | add_expr T_MINUS mul_expr
    ;

mul_expr
    : unary_expr
    | mul_expr T_STAR    unary_expr
    | mul_expr T_SLASH   unary_expr
    | mul_expr T_PERCENT unary_expr
    ;

unary_expr
    : T_NOT   unary_expr
    | T_TILDE unary_expr
    | T_MINUS unary_expr %prec UMINUS
    | T_PLUS  unary_expr %prec UPLUS
    | postfix
    ;

postfix
    : primary
    | postfix T_PARENL arg_list_opt T_PARENR
    | postfix T_BRACKETL expr T_BRACKETR
    ;

arg_list_opt
    : /* empty */
    | arg_list
    ;

arg_list
    : expr
    | arg_list T_COMMA expr
    ;

primary
    : T_INTLIT
    | T_FLOATLIT
    | T_STRINGLIT
    | T_CHARLIT
    | T_IDENTIFIER           /* 'true'/'false' arrive as identifiers in your lexer */
    | T_PARENL expr T_PARENR
    ;

%%  /* ==================== User code / adapter ==================== */

/* Stand-alone mode: use your C++ Lexer to feed tokens to Bison.
   Compile with -DBUILD_STANDALONE to enable main() and yylex() below. */

#ifdef BUILD_STANDALONE
  #include "lexer.hpp"
  #include "token.hpp"
  #include <vector>
  #include <string>
  #include <fstream>
  #include <sstream>
  #include <iostream>

  static std::vector<Token> GTOKS;
  static std::size_t GIDX = 0;

  static int mapToken(const Token& t) {
    switch (t.type) {
      case TokenType::T_FUNCTION:   return T_FUNCTION;
      case TokenType::T_RETURN:     return T_RETURN;
      case TokenType::T_IF:         return T_IF;
      case TokenType::T_ELSE:       return T_ELSE;
      case TokenType::T_FOR:        return T_FOR;
      case TokenType::T_WHILE:      return T_WHILE;

      case TokenType::T_INT:        return T_INT;
      case TokenType::T_FLOAT:      return T_FLOAT;
      case TokenType::T_BOOL:       return T_BOOL;
      case TokenType::T_STRING:     return T_STRING;
      case TokenType::T_CHAR:       return T_CHAR;

      case TokenType::T_ANDAND:     return T_ANDAND;
      case TokenType::T_OROR:       return T_OROR;
      case TokenType::T_EQUALSOP:   return T_EQUALSOP;
      case TokenType::T_NOTEQ:      return T_NOTEQ;
      case TokenType::T_LE:         return T_LE;
      case TokenType::T_GE:         return T_GE;
      case TokenType::T_SHL:        return T_SHL;
      case TokenType::T_SHR:        return T_SHR;

      case TokenType::T_ASSIGNOP:   return T_ASSIGNOP;
      case TokenType::T_LT:         return T_LT;
      case TokenType::T_GT:         return T_GT;
      case TokenType::T_NOT:        return T_NOT;

      case TokenType::T_PLUS:       return T_PLUS;
      case TokenType::T_MINUS:      return T_MINUS;
      case TokenType::T_STAR:       return T_STAR;
      case TokenType::T_SLASH:      return T_SLASH;
      case TokenType::T_PERCENT:    return T_PERCENT;

      case TokenType::T_AMP:        return T_AMP;
      case TokenType::T_PIPE:       return T_PIPE;
      case TokenType::T_CARET:      return T_CARET;
      case TokenType::T_TILDE:      return T_TILDE;

      case TokenType::T_PARENL:     return T_PARENL;
      case TokenType::T_PARENR:     return T_PARENR;
      case TokenType::T_BRACEL:     return T_BRACEL;
      case TokenType::T_BRACER:     return T_BRACER;
      case TokenType::T_BRACKETL:   return T_BRACKETL;
      case TokenType::T_BRACKETR:   return T_BRACKETR;

      case TokenType::T_COMMA:      return T_COMMA;
      case TokenType::T_SEMICOLON:  return T_SEMICOLON;
      case TokenType::T_DOT:        return T_DOT;

      case TokenType::T_INTLIT:     return T_INTLIT;
      case TokenType::T_FLOATLIT:   return T_FLOATLIT;
      case TokenType::T_STRINGLIT:  return T_STRINGLIT;
      case TokenType::T_CHARLIT:    return T_CHARLIT;
      case TokenType::T_IDENTIFIER: return T_IDENTIFIER;
      default:                      return 0; /* should not happen */
    }
  }

  int yylex(void) {
    if (GIDX >= GTOKS.size()) return 0;          // EOF
    int code = mapToken(GTOKS[GIDX]);
    ++GIDX;
    return code;
  }

  static std::string readAll(std::istream& in) {
    std::ostringstream ss; ss << in.rdbuf(); return ss.str();
  }

  int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    std::string src;
    if (argc >= 2) {
      std::ifstream fin(argv[1], std::ios::binary);
      if (!fin) { std::perror("open"); return 1; }
      src = readAll(fin);
    } else {
      src = readAll(std::cin);
    }

    try {
      Lexer L(src);
      GTOKS = L.tokenize();
      GIDX = 0;
    } catch (const std::exception& e) {
      std::fprintf(stderr, "Lex error: %s\n", e.what());
      return 2;
    }

    int r = yyparse();
    if (r == 0) {
      // Success: stay quiet (like most course autograders expect)
      // std::puts("OK");
    }
    return r;
  }
#endif /* BUILD_STANDALONE */

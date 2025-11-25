/* A Bison parser, made by GNU Bison 3.7.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_MINI_LANG_TAB_H_INCLUDED
# define YY_YY_MINI_LANG_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    T_FUNCTION = 258,              /* T_FUNCTION  */
    T_RETURN = 259,                /* T_RETURN  */
    T_IF = 260,                    /* T_IF  */
    T_ELSE = 261,                  /* T_ELSE  */
    T_FOR = 262,                   /* T_FOR  */
    T_WHILE = 263,                 /* T_WHILE  */
    T_INT = 264,                   /* T_INT  */
    T_FLOAT = 265,                 /* T_FLOAT  */
    T_BOOL = 266,                  /* T_BOOL  */
    T_STRING = 267,                /* T_STRING  */
    T_CHAR = 268,                  /* T_CHAR  */
    T_ANDAND = 269,                /* T_ANDAND  */
    T_OROR = 270,                  /* T_OROR  */
    T_EQUALSOP = 271,              /* T_EQUALSOP  */
    T_NOTEQ = 272,                 /* T_NOTEQ  */
    T_LE = 273,                    /* T_LE  */
    T_GE = 274,                    /* T_GE  */
    T_SHL = 275,                   /* T_SHL  */
    T_SHR = 276,                   /* T_SHR  */
    T_ASSIGNOP = 277,              /* T_ASSIGNOP  */
    T_LT = 278,                    /* T_LT  */
    T_GT = 279,                    /* T_GT  */
    T_NOT = 280,                   /* T_NOT  */
    T_PLUS = 281,                  /* T_PLUS  */
    T_MINUS = 282,                 /* T_MINUS  */
    T_STAR = 283,                  /* T_STAR  */
    T_SLASH = 284,                 /* T_SLASH  */
    T_PERCENT = 285,               /* T_PERCENT  */
    T_AMP = 286,                   /* T_AMP  */
    T_PIPE = 287,                  /* T_PIPE  */
    T_CARET = 288,                 /* T_CARET  */
    T_TILDE = 289,                 /* T_TILDE  */
    T_PARENL = 290,                /* T_PARENL  */
    T_PARENR = 291,                /* T_PARENR  */
    T_BRACEL = 292,                /* T_BRACEL  */
    T_BRACER = 293,                /* T_BRACER  */
    T_BRACKETL = 294,              /* T_BRACKETL  */
    T_BRACKETR = 295,              /* T_BRACKETR  */
    T_COMMA = 296,                 /* T_COMMA  */
    T_SEMICOLON = 297,             /* T_SEMICOLON  */
    T_DOT = 298,                   /* T_DOT  */
    T_INTLIT = 299,                /* T_INTLIT  */
    T_FLOATLIT = 300,              /* T_FLOATLIT  */
    T_STRINGLIT = 301,             /* T_STRINGLIT  */
    T_CHARLIT = 302,               /* T_CHARLIT  */
    T_IDENTIFIER = 303,            /* T_IDENTIFIER  */
    UMINUS = 304,                  /* UMINUS  */
    UPLUS = 305                    /* UPLUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_MINI_LANG_TAB_H_INCLUDED  */

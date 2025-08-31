#pragma once
#include <string>
using namespace std;

enum class TokenType {
    T_FUNCTION, T_RETURN, T_IF, T_ELSE, T_FOR, T_WHILE,
    T_INT, T_FLOAT, T_BOOL, T_STRING, T_CHAR,
    T_IDENTIFIER, T_INTLIT, T_FLOATLIT, T_STRINGLIT, T_CHARLIT,
    T_PARENL, T_PARENR, T_BRACEL, T_BRACER, T_BRACKETL, T_BRACKETR,
    T_COMMA, T_SEMICOLON,
    T_ASSIGNOP, T_EQUALSOP, T_NOTEQ, T_LE, T_GE, T_LT, T_GT,
    T_ANDAND, T_OROR, T_NOT,
    T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_AMP, T_PIPE, T_CARET, T_TILDE,
    T_SHL, T_SHR
};

struct Token {
    TokenType type;
    string lexeme;
    string value;
    size_t startPos;
};

string toString(const Token& t);

#pragma once

#include "token.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
using namespace std;

class Lexer {
public:
    explicit Lexer(string src);
    vector<Token> tokenize();
private:
    string s;
    size_t n;
    size_t i;
    unordered_map<string, TokenType> kw, ty;

    static bool isAlpha(char c);
    static bool isDigit(char c);
    static bool isAlnum(char c);
    bool eof() const;
    char peek(size_t off = 0) const;
    char advance();
    bool match(const char* lit);
    void skipSpaceAndComments();
    Token tok(TokenType t, const string& lex, const string& val, size_t start);
    Token tok(TokenType t, const string& lex, const string& val = "");
    Token scanString();
    Token scanChar();
    Token scanIdent();
    Token scanNumber();
    void initTables();
};

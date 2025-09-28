// lexer.hpp
#pragma once
#include <string>
#include <vector>
#include "token.hpp"
using namespace std;

class Lexer {
public:
    explicit Lexer(string src);
    vector<Token> tokenize();
private:
    string input;
    size_t pos;
    void buildRules();
    void skipSpaceAndComments();
};

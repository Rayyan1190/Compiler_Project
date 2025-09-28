#include <iostream>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>
#include <string>
#include "token.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "ast.hpp"
using namespace std;

static const char* parse_error_name(ParseError e){
    switch (e){
        case ParseError::UnexpectedEOF: return "UnexpectedEOF";
        case ParseError::FailedToFindToken: return "FailedToFindToken";
        case ParseError::ExpectedTypeToken: return "ExpectedTypeToken";
        case ParseError::ExpectedIdentifier: return "ExpectedIdentifier";
        case ParseError::UnexpectedToken: return "UnexpectedToken";
        case ParseError::ExpectedFloatLit: return "ExpectedFloatLit";
        case ParseError::ExpectedIntLit: return "ExpectedIntLit";
        case ParseError::ExpectedStringLit: return "ExpectedStringLit";
        case ParseError::ExpectedBoolLit: return "ExpectedBoolLit";
        case ParseError::ExpectedExpr: return "ExpectedExpr";
        default: return "ParseError";
    }
}

int main(){
    ifstream fin("input.fn", ios::in | ios::binary);
    if (!fin){
        cerr << "Error: could not open 'input.fn' in the current folder.\n";
        return 2;
    }
    string src((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
    if (src.empty()){
        cerr << "Error: 'input.fn' is empty.\n";
        return 3;
    }

    try {
        Lexer lex(src);
        vector<Token> tokens = lex.tokenize();

        cout << "[";
        for (size_t i = 0; i < tokens.size(); ++i){
            cout << toString(tokens[i]);
            if (i + 1 < tokens.size()) cout << ", ";
        }
        cout << "]\n";

        ofstream fout("tokens.txt", ios::out | ios::trunc);
        fout << "[";
        for (size_t i = 0; i < tokens.size(); ++i){
            fout << toString(tokens[i]);
            if (i + 1 < tokens.size()) fout << ", ";
        }
        fout << "]\n";

        Parser p(tokens, src);
        auto prog = p.parse();
        prog->print(cout);
    }
    catch (const ParseException& ex){
        cerr << "Parse error [" << parse_error_name(ex.kind) << "]: " << ex.what() << "\n";
        if (ex.offending) cerr << "Offending token: " << toString(*ex.offending) << "\n";
        return 1;
    }
    catch (const exception& ex){
        cerr << "Lexer error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

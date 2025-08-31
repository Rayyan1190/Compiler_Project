#include <iostream>
#include <fstream>
#include <iterator>
#include "lexer.hpp"
#include "token.hpp"
using namespace std;

int main() {
    ifstream fin("input.fn", ios::in | ios::binary);
    if (!fin) {
        cerr << "Error: could not open 'input.fn' in the current folder.\n";
        return 2;
    }
    string src((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
    if (src.empty()) {
        cerr << "Error: 'input.fn' is empty.\n";
        return 3;
    }
    try {
        Lexer lex(src);
        auto tokens = lex.tokenize();

        cout << "[";
        for (size_t i = 0; i < tokens.size(); ++i) {
            cout << toString(tokens[i]);
            if (i + 1 < tokens.size()) cout << ", ";
        }
        cout << "]\n";

        ofstream fout("tokens.txt", ios::out | ios::trunc);
        fout << "[";
        for (size_t i = 0; i < tokens.size(); ++i) {
            fout << toString(tokens[i]);
            if (i + 1 < tokens.size()) fout << ", ";
        }
        fout << "]\n";
    } catch (const exception& ex) {
        cerr << "Lexer error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}

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
#include "scope.hpp"
#include "typechk.hpp"

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

static const char* scope_error_name(ScopeError e){
    switch (e){
        case ScopeError::UndeclaredVariableAccessed:    return "UndeclaredVariableAccessed";
        case ScopeError::UndefinedFunctionCalled:       return "UndefinedFunctionCalled";
        case ScopeError::VariableRedefinition:          return "VariableRedefinition";
        case ScopeError::FunctionPrototypeRedefinition: return "FunctionPrototypeRedefinition";
        default: return "ScopeError";
    }
}

static const char* typechk_error_name(TypeChkError e){
    switch (e){
        case TypeChkError::ErroneousVarDecl: return "ErroneousVarDecl";
        case TypeChkError::FnCallParamCount: return "FnCallParamCount";
        case TypeChkError::FnCallParamType: return "FnCallParamType";
        case TypeChkError::ErroneousReturnType: return "ErroneousReturnType";
        case TypeChkError::ExpressionTypeMismatch: return "ExpressionTypeMismatch";
        case TypeChkError::ExpectedBooleanExpression: return "ExpectedBooleanExpression";
        case TypeChkError::ErroneousBreak: return "ErroneousBreak";
        case TypeChkError::NonBooleanCondStmt: return "NonBooleanCondStmt";
        case TypeChkError::EmptyExpression: return "EmptyExpression";
        case TypeChkError::AttemptedBoolOpOnNonBools: return "AttemptedBoolOpOnNonBools";
        case TypeChkError::AttemptedBitOpOnNonNumeric: return "AttemptedBitOpOnNonNumeric";
        case TypeChkError::AttemptedShiftOnNonInt: return "AttemptedShiftOnNonInt";
        case TypeChkError::AttemptedAddOpOnNonNumeric: return "AttemptedAddOpOnNonNumeric";
        case TypeChkError::AttemptedExponentiationOfNonNumeric: return "AttemptedExponentiationOfNonNumeric";
        case TypeChkError::ReturnStmtNotFound: return "ReturnStmtNotFound";
        default: return "TypeChkError";
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

        ScopeAnalyzer sa;
        sa.analyzeProgram(*prog);

        if (sa.hasErrors()) {
            cerr << "Scope analysis reported errors:\n";
            for (const auto& d : sa.getDiagnostics()) {
                cerr << "  [" << scope_error_name(d.kind) << "] "
                     << (d.name.empty() ? "<anon>" : d.name)
                     << ": " << d.message << "\n";
            }
            return 4;
        }

        TypeChecker tc(sa);
        tc.analyzeProgram(*prog);

        if (tc.hasErrors()) {
            cerr << "Type checking reported errors:\n";
            for (const auto& d : tc.getDiagnostics()) {
                cerr << "  [" << typechk_error_name(d.kind) << "] "
                     << d.message << "\n";
            }
            return 5;
        }

        cout << "\n[Scope OK]\n[TypeCheck OK]\n\n";
        prog->print(cout);
    }
    catch (const ParseException& ex){
        cerr << "Parse error [" << parse_error_name(ex.kind) << "]: " << ex.what() << "\n";
        if (ex.offending) cerr << "Offending token: " << toString(*ex.offending) << "\n";
        return 1;
    }
    catch (const exception& ex){
        cerr << "Lexer/Scope/Type error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}

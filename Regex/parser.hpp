#pragma once
#include <vector>
#include <string>
#include <stdexcept>
#include <optional>
#include <unordered_map>
#include <initializer_list>
#include "token.hpp"
#include "ast.hpp"

using namespace std;

enum class ParseError {
    UnexpectedEOF,
    FailedToFindToken,
    ExpectedTypeToken,
    ExpectedIdentifier,
    UnexpectedToken,
    ExpectedFloatLit,
    ExpectedIntLit,
    ExpectedStringLit,
    ExpectedBoolLit,
    ExpectedExpr,
};

struct ParseException : runtime_error {
    ParseError kind;
    optional<Token> offending;
    explicit ParseException(ParseError k, const string& msg, optional<Token> tok = nullopt)
        : runtime_error(msg), kind(k), offending(move(tok)) {}
};

class Parser {
public:
    Parser(vector<Token> toks, string source = "");
    shared_ptr<Program> parse();

private:
    vector<Token> tokens;
    string source;
    size_t i = 0;

    vector<unordered_map<string, TypeKind>> scopes;
    void pushScope();
    void popScope();
    void declareVar(const string& name, TypeKind k);
    optional<TypeKind> lookupVar(const string& name) const;
    void checkLiteralAgainst(TypeKind expected, const ExprPtr& rhs, const char* contextMsg);

    bool atEnd() const;
    const Token& peek() const;
    const Token& prev() const;
    const Token& advance();
    bool check(TokenType t) const;
    bool match(initializer_list<TokenType> list);
    const Token& expect(TokenType t, ParseError errKind, const char* msg);

    DeclPtr parseTopLevel();
    shared_ptr<FunctionDecl> parseFunction();
    shared_ptr<VarDeclStmt> parseVarDeclStmt();
    vector<Param> parseParams();
    Param parseParam();
    Type parseType();

    StmtPtr parseStmt();
    shared_ptr<BlockStmt> parseBlock();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseFor();
    StmtPtr parseReturn();
    StmtPtr parseExprStmt();

    ExprPtr parseExpr();
    ExprPtr parseAssignment();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseBitOr();
    ExprPtr parseBitXor();
    ExprPtr parseBitAnd();
    ExprPtr parseEquality();
    ExprPtr parseRel();
    ExprPtr parseShift();
    ExprPtr parseAdd();
    ExprPtr parseMul();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
};

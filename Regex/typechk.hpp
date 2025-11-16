#pragma once
#include <vector>
#include <string>
#include "scope.hpp"

using namespace std;

enum class TypeChkError {
    ErroneousVarDecl,
    FnCallParamCount,
    FnCallParamType,
    ErroneousReturnType,
    ExpressionTypeMismatch,
    ExpectedBooleanExpression,
    ErroneousBreak,
    NonBooleanCondStmt,
    EmptyExpression,
    AttemptedBoolOpOnNonBools,
    AttemptedBitOpOnNonNumeric,
    AttemptedShiftOnNonInt,
    AttemptedAddOpOnNonNumeric,
    AttemptedExponentiationOfNonNumeric,
    ReturnStmtNotFound,
};

struct TypeChkDiagnostic {
    TypeChkError kind;
    string message;
    const Node* where;
};

class TypeChecker {
public:
    TypeChecker(const ScopeAnalyzer& scopeInfo);
    void analyzeProgram(const Program& program);
    bool hasErrors() const;
    const vector<TypeChkDiagnostic>& getDiagnostics() const;

private:
    const ScopeAnalyzer& scope;
    vector<TypeChkDiagnostic> diagnostics;
    Type currentFunctionReturnType;
    bool functionHasReturnType;
    bool functionHasReturnStatement;
    int loopDepth;

    void report(TypeChkError kind, const Node* where, const string& message);

    void analyzeTopLevelDecl(const Decl* decl);
    void analyzeFunctionDecl(const FunctionDecl* fn);
    void analyzeTopVarDecl(const TopVarDecl* tv);

    void analyzeBlock(const BlockStmt* block);
    void analyzeStatement(const Stmt* stmt);
    void analyzeIfStatement(const IfStmt* s);
    void analyzeWhileStatement(const WhileStmt* s);
    void analyzeForStatement(const ForStmt* s);
    void analyzeReturnStatement(const ReturnStmt* s);
    void analyzeExprStatement(const ExprStmt* s);
    void analyzeVarDeclStatement(const VarDeclStmt* s);

    Type checkExpression(const Expr* expr);
    Type checkUnaryExpression(const UnaryExpr* e);
    Type checkBinaryExpression(const BinaryExpr* e);
    Type checkCallExpression(const CallExpr* e);
    Type checkIndexExpression(const IndexExpr* e);
    Type checkIdentifier(const Ident* id);

    bool isNumeric(const Type& t) const;
    bool isInteger(const Type& t) const;
    bool isBoolean(const Type& t) const;
};

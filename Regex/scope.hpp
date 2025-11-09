#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>
#include "ast.hpp"

using namespace std;

enum class ScopeError {
    UndeclaredVariableAccessed,
    UndefinedFunctionCalled,
    VariableRedefinition,
    FunctionPrototypeRedefinition,
};

struct ScopeDiagnostic {
    ScopeError kind;
    string name;
    string message;
    const Node* where;
};

enum class SymbolKind { Variable, Function };

struct FunctionSignature {
    vector<Type> paramTypes;
    optional<Type> returnType;
    bool equals(const FunctionSignature& other) const {
        if ((bool)returnType != (bool)other.returnType) return false;
        if (returnType && other.returnType && returnType->kind != other.returnType->kind) return false;
        if (paramTypes.size() != other.paramTypes.size()) return false;
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (paramTypes[i].kind != other.paramTypes[i].kind) return false;
        }
        return true;
    }
};

struct Symbol {
    SymbolKind kind;
    string name;
    optional<Type> variableType;
    optional<FunctionSignature> functionSig;
    bool isPrototype = false;
    bool isDefined = false;
};

struct ScopeFrame {
    unordered_map<string, Symbol> table;
    ScopeFrame* parent = nullptr;
};

class ScopeAnalyzer {
public:
    ScopeAnalyzer();
    ~ScopeAnalyzer() = default;
    void analyzeProgram(const Program& program);
    const vector<ScopeDiagnostic>& getDiagnostics() const { return diagnostics; }
    bool hasErrors() const { return !diagnostics.empty(); }
    const Symbol* getResolvedSymbolForIdent(const Ident* id) const;
    const Symbol* getResolvedSymbolForCall(const CallExpr* call) const;

private:
    void enterNewScope();
    void exitCurrentScope();
    void declareVariableInCurrentScope(const string& name, const Type& type, const Node* where);
    void declareFunctionPrototypeInCurrentScope(const string& name, const FunctionSignature& sig, const Node* where);
    void declareFunctionDefinitionInCurrentScope(const string& name, const FunctionSignature& sig, const Node* where);
    const Symbol* lookupAnySymbol(const string& name) const;
    const Symbol* lookupVariableSymbol(const string& name) const;
    const Symbol* lookupFunctionSymbol(const string& name) const;
    void report(ScopeError kind, const string& name, const Node* where, const string& message);
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
    void analyzeExpression(const Expr* expr);
    void analyzeUnaryExpression(const UnaryExpr* e);
    void analyzeBinaryExpression(const BinaryExpr* e);
    void analyzeCallExpression(const CallExpr* e);
    void analyzeIndexExpression(const IndexExpr* e);
    void analyzeIdentifierUse(const Ident* id, bool isCalleeContext);

    vector<unique_ptr<ScopeFrame>> ownedFrames;
    ScopeFrame* current = nullptr;
    vector<ScopeDiagnostic> diagnostics;
    unordered_map<const Ident*, const Symbol*> resolvedIdents;
    unordered_map<const CallExpr*, const Symbol*> resolvedCalls;
};

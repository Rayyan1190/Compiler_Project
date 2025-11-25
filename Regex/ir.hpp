#pragma once
#include <string>
#include <vector>
#include <ostream>
#include "typechk.hpp"

using namespace std;

enum class IRInstrKind {
    Assign,
    Unary,
    Binary,
    Label,
    Goto,
    IfGoto,
    Param,
    Call,
    Return,
    ReturnVoid,
    IndexLoad,
    IndexStore
};

struct IRInstr {
    IRInstrKind kind;
    string dst;
    string src1;
    string src2;
    string info;
};

struct IRFunction {
    string name;
    vector<string> params;
    vector<IRInstr> instructions;
};

struct IRGlobal {
    string name;
    Type type;
    bool hasInit;
    string initValue;
};

struct IRProgram {
    vector<IRGlobal> globals;
    vector<IRFunction> functions;
};

enum class IRGenError {
    UnsupportedExpression,
    UnsupportedStatement,
    InvalidAssignmentTarget
};

struct IRGenDiagnostic {
    IRGenError kind;
    string message;
    const Node* where;
};

class IRGenerator {
public:
    IRGenerator(const ScopeAnalyzer& s, const TypeChecker& t);
    IRProgram generate(const Program& program);
    const vector<IRGenDiagnostic>& getDiagnostics() const;
    bool hasErrors() const;

private:
    const ScopeAnalyzer& scope;
    const TypeChecker& types;
    IRProgram irProgram;
    vector<IRGenDiagnostic> diagnostics;
    IRFunction* currentFunction;
    int tempCounter;
    int labelCounter;

    void report(IRGenError kind, const Node* where, const string& message);
    string createTemp();
    string createLabel(const string& base);
    void emit(const IRInstr& instr);

    void generateTopLevelDecl(const Decl* decl);
    void generateFunction(const FunctionDecl* fn);
    void generateTopVar(const TopVarDecl* tv);

    void generateBlock(const BlockStmt* block);
    void generateStatement(const Stmt* stmt);
    void generateIf(const IfStmt* s);
    void generateWhile(const WhileStmt* s);
    void generateFor(const ForStmt* s);
    void generateReturn(const ReturnStmt* s);
    void generateExprStmt(const ExprStmt* s);
    void generateVarDeclStmt(const VarDeclStmt* s);

    string generateExpr(const Expr* expr);
    string generateUnary(const UnaryExpr* e);
    string generateBinary(const BinaryExpr* e);
    string generateCall(const CallExpr* e);
    string generateIndex(const IndexExpr* e);
    string generateIdentifier(const Ident* id);

    string opStringForBinary(BinaryOp op) const;
};

void printIRProgram(const IRProgram& ir, ostream& os);

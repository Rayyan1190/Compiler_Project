#include "scope.hpp"
#include <cassert>
using namespace std;

static const char* scopeErrorName(ScopeError e) {
    switch (e) {
        case ScopeError::UndeclaredVariableAccessed: return "UndeclaredVariableAccessed";
        case ScopeError::UndefinedFunctionCalled: return "UndefinedFunctionCalled";
        case ScopeError::VariableRedefinition: return "VariableRedefinition";
        case ScopeError::FunctionPrototypeRedefinition: return "FunctionPrototypeRedefinition";
        default: return "ScopeError";
    }
}

ScopeAnalyzer::ScopeAnalyzer() {
    enterNewScope();
}

void ScopeAnalyzer::enterNewScope() {
    auto frame = make_unique<ScopeFrame>();
    frame->parent = current;
    current = frame.get();
    ownedFrames.emplace_back(move(frame));
}

void ScopeAnalyzer::exitCurrentScope() {
    assert(current && "attempted to pop scope when none exists");
    current = current->parent;
}

void ScopeAnalyzer::report(ScopeError kind, const string& name, const Node* where, const string& message) {
    diagnostics.push_back(ScopeDiagnostic{kind, name, message, where});
}

const Symbol* ScopeAnalyzer::lookupAnySymbol(const string& name) const {
    for (auto* f = current; f; f = f->parent) {
        auto it = f->table.find(name);
        if (it != f->table.end()) return &it->second;
    }
    return nullptr;
}

const Symbol* ScopeAnalyzer::lookupVariableSymbol(const string& name) const {
    const Symbol* s = lookupAnySymbol(name);
    return (s && s->kind == SymbolKind::Variable) ? s : nullptr;
}

const Symbol* ScopeAnalyzer::lookupFunctionSymbol(const string& name) const {
    const Symbol* s = lookupAnySymbol(name);
    return (s && s->kind == SymbolKind::Function) ? s : nullptr;
}

void ScopeAnalyzer::declareVariableInCurrentScope(const string& name, const Type& type, const Node* where) {
    assert(current && "no active scope");
    auto it = current->table.find(name);
    if (it != current->table.end()) {
        report(ScopeError::VariableRedefinition, name, where, "conflicting variable name in the same scope");
        return;
    }
    Symbol sym;
    sym.kind = SymbolKind::Variable;
    sym.name = name;
    sym.variableType = type;
    current->table.emplace(name, move(sym));
}

void ScopeAnalyzer::declareFunctionPrototypeInCurrentScope(const string& name, const FunctionSignature& sig, const Node* where) {
    assert(current && "no active scope");
    auto it = current->table.find(name);
    if (it == current->table.end()) {
        Symbol sym;
        sym.kind = SymbolKind::Function;
        sym.name = name;
        sym.functionSig = sig;
        sym.isPrototype = true;
        sym.isDefined = false;
        current->table.emplace(name, move(sym));
        return;
    }
    Symbol& existing = it->second;
    if (existing.kind != SymbolKind::Function) {
        report(ScopeError::VariableRedefinition, name, where, "name already used for a variable in this scope");
        return;
    }
    if (existing.isDefined) {
        report(ScopeError::FunctionPrototypeRedefinition, name, where, "prototype appears after a definition in the same scope");
        return;
    }
    if (!existing.functionSig->equals(sig)) {
        report(ScopeError::FunctionPrototypeRedefinition, name, where, "conflicting function prototypes in the same scope");
        return;
    }
}

void ScopeAnalyzer::declareFunctionDefinitionInCurrentScope(const string& name, const FunctionSignature& sig, const Node* where) {
    assert(current && "no active scope");
    auto it = current->table.find(name);
    if (it == current->table.end()) {
        Symbol sym;
        sym.kind = SymbolKind::Function;
        sym.name = name;
        sym.functionSig = sig;
        sym.isPrototype = false;
        sym.isDefined = true;
        current->table.emplace(name, move(sym));
        return;
    }
    Symbol& existing = it->second;
    if (existing.kind != SymbolKind::Function) {
        report(ScopeError::VariableRedefinition, name, where, "name already used for a variable in this scope");
        return;
    }
    if (existing.isDefined) {
        report(ScopeError::FunctionPrototypeRedefinition, name, where, "function redefinition in the same scope");
        return;
    }
    if (!existing.functionSig->equals(sig)) {
        report(ScopeError::FunctionPrototypeRedefinition, name, where, "definition conflicts with previous prototype");
        return;
    }
    existing.isPrototype = false;
    existing.isDefined = true;
}

void ScopeAnalyzer::analyzeProgram(const Program& program) {
    for (const auto& d : program.decls) analyzeTopLevelDecl(d.get());
}

void ScopeAnalyzer::analyzeTopLevelDecl(const Decl* decl) {
    if (auto* fn = dynamic_cast<const FunctionDecl*>(decl)) analyzeFunctionDecl(fn);
    else if (auto* tv = dynamic_cast<const TopVarDecl*>(decl)) analyzeTopVarDecl(tv);
}

void ScopeAnalyzer::analyzeTopVarDecl(const TopVarDecl* tv) {
    const auto& s = *tv->decl;
    declareVariableInCurrentScope(s.name, s.type, tv);
    if (s.init) analyzeExpression(s.init->get());
}

void ScopeAnalyzer::analyzeFunctionDecl(const FunctionDecl* fn) {
    FunctionSignature sig;
    sig.returnType = fn->retType;
    for (const auto& p : fn->params) sig.paramTypes.push_back(p.type);
    declareFunctionDefinitionInCurrentScope(fn->name, sig, fn);
    enterNewScope();
    for (const auto& p : fn->params) declareVariableInCurrentScope(p.name, p.type, fn);
    analyzeBlock(fn->body.get());
    exitCurrentScope();
}

void ScopeAnalyzer::analyzeBlock(const BlockStmt* block) {
    enterNewScope();
    for (const auto& s : block->stmts) analyzeStatement(s.get());
    exitCurrentScope();
}

void ScopeAnalyzer::analyzeStatement(const Stmt* stmt) {
    if (auto* b = dynamic_cast<const BlockStmt*>(stmt)) { analyzeBlock(b); return; }
    if (auto* s = dynamic_cast<const IfStmt*>(stmt)) { analyzeIfStatement(s); return; }
    if (auto* s = dynamic_cast<const WhileStmt*>(stmt)) { analyzeWhileStatement(s); return; }
    if (auto* s = dynamic_cast<const ForStmt*>(stmt)) { analyzeForStatement(s); return; }
    if (auto* s = dynamic_cast<const ReturnStmt*>(stmt)) { analyzeReturnStatement(s); return; }
    if (auto* s = dynamic_cast<const ExprStmt*>(stmt)) { analyzeExprStatement(s); return; }
    if (auto* s = dynamic_cast<const VarDeclStmt*>(stmt)) { analyzeVarDeclStatement(s); return; }
}

void ScopeAnalyzer::analyzeIfStatement(const IfStmt* s) {
    analyzeExpression(s->cond.get());
    analyzeStatement(s->thenS.get());
    if (s->elseS) analyzeStatement(s->elseS->get());
}

void ScopeAnalyzer::analyzeWhileStatement(const WhileStmt* s) {
    analyzeExpression(s->cond.get());
    analyzeStatement(s->body.get());
}

void ScopeAnalyzer::analyzeForStatement(const ForStmt* s) {
    enterNewScope();
    if (s->init) analyzeStatement(s->init->get());
    if (s->cond) analyzeExpression(s->cond->get());
    if (s->incr) analyzeExpression(s->incr->get());
    analyzeStatement(s->body.get());
    exitCurrentScope();
}

void ScopeAnalyzer::analyzeReturnStatement(const ReturnStmt* s) {
    if (s->expr) analyzeExpression(s->expr->get());
}

void ScopeAnalyzer::analyzeExprStatement(const ExprStmt* s) {
    analyzeExpression(s->expr.get());
}

void ScopeAnalyzer::analyzeVarDeclStatement(const VarDeclStmt* s) {
    declareVariableInCurrentScope(s->name, s->type, s);
    if (s->init) analyzeExpression(s->init->get());
}

void ScopeAnalyzer::analyzeExpression(const Expr* expr) {
    if (!expr) return;
    if (dynamic_cast<const IntLit*>(expr)) return;
    if (dynamic_cast<const FloatLit*>(expr)) return;
    if (dynamic_cast<const StringLit*>(expr)) return;
    if (dynamic_cast<const CharLit*>(expr)) return;
    if (dynamic_cast<const BoolLit*>(expr)) return;
    if (auto* id = dynamic_cast<const Ident*>(expr)) { analyzeIdentifierUse(id, false); return; }
    if (auto* u = dynamic_cast<const UnaryExpr*>(expr)) { analyzeUnaryExpression(u); return; }
    if (auto* b = dynamic_cast<const BinaryExpr*>(expr)) { analyzeBinaryExpression(b); return; }
    if (auto* c = dynamic_cast<const CallExpr*>(expr)) { analyzeCallExpression(c); return; }
    if (auto* x = dynamic_cast<const IndexExpr*>(expr)) { analyzeIndexExpression(x); return; }
}

void ScopeAnalyzer::analyzeUnaryExpression(const UnaryExpr* e) {
    analyzeExpression(e->rhs.get());
}

void ScopeAnalyzer::analyzeBinaryExpression(const BinaryExpr* e) {
    analyzeExpression(e->lhs.get());
    analyzeExpression(e->rhs.get());
}

void ScopeAnalyzer::analyzeCallExpression(const CallExpr* e) {
    if (auto* id = dynamic_cast<const Ident*>(e->callee.get())) {
        const Symbol* fn = lookupFunctionSymbol(id->name);
        if (!fn) {
            if (lookupVariableSymbol(id->name)) report(ScopeError::UndefinedFunctionCalled, id->name, e, "identifier is a variable, not a function");
            else report(ScopeError::UndefinedFunctionCalled, id->name, e, "call to undefined function");
        } else resolvedCalls[e] = fn;
    } else analyzeExpression(e->callee.get());
    for (const auto& arg : e->args) analyzeExpression(arg.get());
}

void ScopeAnalyzer::analyzeIndexExpression(const IndexExpr* e) {
    analyzeExpression(e->base.get());
    analyzeExpression(e->index.get());
}

void ScopeAnalyzer::analyzeIdentifierUse(const Ident* id, bool) {
    const Symbol* var = lookupVariableSymbol(id->name);
    if (!var) report(ScopeError::UndeclaredVariableAccessed, id->name, id, "use of undeclared variable");
    else resolvedIdents[id] = var;
}

const Symbol* ScopeAnalyzer::getResolvedSymbolForIdent(const Ident* id) const {
    auto it = resolvedIdents.find(id);
    return it == resolvedIdents.end() ? nullptr : it->second;
}

const Symbol* ScopeAnalyzer::getResolvedSymbolForCall(const CallExpr* call) const {
    auto it = resolvedCalls.find(call);
    return it == resolvedCalls.end() ? nullptr : it->second;
}

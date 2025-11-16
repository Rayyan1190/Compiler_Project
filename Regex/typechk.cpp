#include "typechk.hpp"
using namespace std;

TypeChecker::TypeChecker(const ScopeAnalyzer& scopeInfo)
    : scope(scopeInfo),
      currentFunctionReturnType(Type::Unknown()),
      functionHasReturnType(false),
      functionHasReturnStatement(false),
      loopDepth(0) {}

bool TypeChecker::hasErrors() const {
    return !diagnostics.empty();
}

const vector<TypeChkDiagnostic>& TypeChecker::getDiagnostics() const {
    return diagnostics;
}

void TypeChecker::report(TypeChkError kind, const Node* where, const string& message) {
    diagnostics.push_back(TypeChkDiagnostic{kind, message, where});
}

bool TypeChecker::isNumeric(const Type& t) const {
    return t.kind == TypeKind::Int || t.kind == TypeKind::Float;
}

bool TypeChecker::isInteger(const Type& t) const {
    return t.kind == TypeKind::Int;
}

bool TypeChecker::isBoolean(const Type& t) const {
    return t.kind == TypeKind::Bool;
}

void TypeChecker::analyzeProgram(const Program& program) {
    for (const auto& d : program.decls) {
        analyzeTopLevelDecl(d.get());
    }
}

void TypeChecker::analyzeTopLevelDecl(const Decl* decl) {
    if (auto* fn = dynamic_cast<const FunctionDecl*>(decl)) {
        analyzeFunctionDecl(fn);
    } else if (auto* tv = dynamic_cast<const TopVarDecl*>(decl)) {
        analyzeTopVarDecl(tv);
    }
}

void TypeChecker::analyzeTopVarDecl(const TopVarDecl* tv) {
    const VarDeclStmt* s = tv->decl.get();
    analyzeVarDeclStatement(s);
}

void TypeChecker::analyzeFunctionDecl(const FunctionDecl* fn) {
    functionHasReturnStatement = false;
    if (fn->retType) {
        currentFunctionReturnType = *fn->retType;
        functionHasReturnType = true;
    } else {
        currentFunctionReturnType = Type::Unknown();
        functionHasReturnType = false;
    }
    analyzeBlock(fn->body.get());
    if (functionHasReturnType && !functionHasReturnStatement) {
        report(TypeChkError::ReturnStmtNotFound, fn, "function '" + fn->name + "' is missing a return statement");
    }
}

void TypeChecker::analyzeBlock(const BlockStmt* block) {
    for (const auto& s : block->stmts) {
        analyzeStatement(s.get());
    }
}

void TypeChecker::analyzeStatement(const Stmt* stmt) {
    if (auto* b = dynamic_cast<const BlockStmt*>(stmt)) {
        analyzeBlock(b);
        return;
    }
    if (auto* s = dynamic_cast<const IfStmt*>(stmt)) {
        analyzeIfStatement(s);
        return;
    }
    if (auto* s = dynamic_cast<const WhileStmt*>(stmt)) {
        analyzeWhileStatement(s);
        return;
    }
    if (auto* s = dynamic_cast<const ForStmt*>(stmt)) {
        analyzeForStatement(s);
        return;
    }
    if (auto* s = dynamic_cast<const ReturnStmt*>(stmt)) {
        analyzeReturnStatement(s);
        return;
    }
    if (auto* s = dynamic_cast<const ExprStmt*>(stmt)) {
        analyzeExprStatement(s);
        return;
    }
    if (auto* s = dynamic_cast<const VarDeclStmt*>(stmt)) {
        analyzeVarDeclStatement(s);
        return;
    }
}

void TypeChecker::analyzeIfStatement(const IfStmt* s) {
    Type condType = checkExpression(s->cond.get());
    if (!isBoolean(condType) && condType.kind != TypeKind::Unknown) {
        report(TypeChkError::NonBooleanCondStmt, s->cond.get(), "if condition must be boolean");
    }
    analyzeStatement(s->thenS.get());
    if (s->elseS) analyzeStatement(s->elseS->get());
}

void TypeChecker::analyzeWhileStatement(const WhileStmt* s) {
    Type condType = checkExpression(s->cond.get());
    if (!isBoolean(condType) && condType.kind != TypeKind::Unknown) {
        report(TypeChkError::NonBooleanCondStmt, s->cond.get(), "while condition must be boolean");
    }
    loopDepth++;
    analyzeStatement(s->body.get());
    loopDepth--;
}

void TypeChecker::analyzeForStatement(const ForStmt* s) {
    if (s->init) analyzeStatement(s->init->get());
    if (s->cond) {
        Type condType = checkExpression(s->cond->get());
        if (!isBoolean(condType) && condType.kind != TypeKind::Unknown) {
            report(TypeChkError::NonBooleanCondStmt, s->cond->get(), "for condition must be boolean");
        }
    }
    if (s->incr) checkExpression(s->incr->get());
    loopDepth++;
    analyzeStatement(s->body.get());
    loopDepth--;
}

void TypeChecker::analyzeReturnStatement(const ReturnStmt* s) {
    functionHasReturnStatement = true;
    if (!functionHasReturnType) {
        if (s->expr) {
            checkExpression(s->expr->get());
            report(TypeChkError::ErroneousReturnType, s, "void function should not return a value");
        }
        return;
    }
    if (!s->expr) {
        report(TypeChkError::ErroneousReturnType, s, "non-void function must return a value");
        return;
    }
    Type exprType = checkExpression(s->expr->get());
    if (exprType.kind != TypeKind::Unknown &&
        exprType.kind != currentFunctionReturnType.kind) {
        report(TypeChkError::ErroneousReturnType, s, "return expression type does not match function return type");
    }
}

void TypeChecker::analyzeExprStatement(const ExprStmt* s) {
    checkExpression(s->expr.get());
}

void TypeChecker::analyzeVarDeclStatement(const VarDeclStmt* s) {
    if (s->init) {
        Type initType = checkExpression(s->init->get());
        if (initType.kind != TypeKind::Unknown &&
            initType.kind != s->type.kind) {
            report(TypeChkError::ErroneousVarDecl, s, "initializer type does not match declared type '" + s->type.str() + "'");
        }
    }
}

Type TypeChecker::checkExpression(const Expr* expr) {
    if (!expr) {
        report(TypeChkError::EmptyExpression, nullptr, "empty expression");
        return Type::Unknown();
    }
    if (dynamic_cast<const IntLit*>(expr)) return Type::Int();
    if (dynamic_cast<const FloatLit*>(expr)) return Type::Float();
    if (dynamic_cast<const StringLit*>(expr)) return Type::String();
    if (dynamic_cast<const CharLit*>(expr)) return Type::Char();
    if (dynamic_cast<const BoolLit*>(expr)) return Type::Bool();
    if (auto* id = dynamic_cast<const Ident*>(expr)) return checkIdentifier(id);
    if (auto* u = dynamic_cast<const UnaryExpr*>(expr)) return checkUnaryExpression(u);
    if (auto* b = dynamic_cast<const BinaryExpr*>(expr)) return checkBinaryExpression(b);
    if (auto* c = dynamic_cast<const CallExpr*>(expr)) return checkCallExpression(c);
    if (auto* i = dynamic_cast<const IndexExpr*>(expr)) return checkIndexExpression(i);
    return Type::Unknown();
}

Type TypeChecker::checkIdentifier(const Ident* id) {
    const Symbol* sym = scope.getResolvedSymbolForIdent(id);
    if (!sym) return Type::Unknown();
    if (sym->variableType) return *sym->variableType;
    if (sym->functionSig && sym->functionSig->returnType) return *sym->functionSig->returnType;
    return Type::Unknown();
}

Type TypeChecker::checkUnaryExpression(const UnaryExpr* e) {
    Type rhsType = checkExpression(e->rhs.get());
    if (rhsType.kind == TypeKind::Unknown) return rhsType;
    switch (e->op) {
        case UnaryOp::Not:
            if (!isBoolean(rhsType)) {
                report(TypeChkError::ExpectedBooleanExpression, e, "logical not operator expects boolean");
            }
            return Type::Bool();
        case UnaryOp::BitNot:
            if (!isInteger(rhsType)) {
                report(TypeChkError::AttemptedBitOpOnNonNumeric, e, "bitwise not operator expects integer");
            }
            return Type::Int();
        case UnaryOp::Neg:
        case UnaryOp::Pos:
            if (!isNumeric(rhsType)) {
                report(TypeChkError::AttemptedAddOpOnNonNumeric, e, "unary plus/minus expect numeric operand");
            }
            return rhsType;
    }
    return Type::Unknown();
}

Type TypeChecker::checkBinaryExpression(const BinaryExpr* e) {
    Type leftType = checkExpression(e->lhs.get());
    Type rightType = checkExpression(e->rhs.get());
    if (leftType.kind == TypeKind::Unknown || rightType.kind == TypeKind::Unknown) {
        return Type::Unknown();
    }
    switch (e->op) {
        case BinaryOp::Assign:
            if (leftType.kind != rightType.kind) {
                report(TypeChkError::ExpressionTypeMismatch, e, "assignment requires both sides to have the same type");
            }
            return leftType;
        case BinaryOp::Or:
        case BinaryOp::And:
            if (!isBoolean(leftType) || !isBoolean(rightType)) {
                report(TypeChkError::AttemptedBoolOpOnNonBools, e, "logical operators require boolean operands");
            }
            return Type::Bool();
        case BinaryOp::BitOr:
        case BinaryOp::BitXor:
        case BinaryOp::BitAnd:
            if (!isInteger(leftType) || !isInteger(rightType)) {
                report(TypeChkError::AttemptedBitOpOnNonNumeric, e, "bitwise operators require integer operands");
            }
            return Type::Int();
        case BinaryOp::Eq:
        case BinaryOp::Neq:
            if (leftType.kind != rightType.kind) {
                report(TypeChkError::ExpressionTypeMismatch, e, "equality operators require operands of the same type");
            }
            return Type::Bool();
        case BinaryOp::Lt:
        case BinaryOp::Le:
        case BinaryOp::Gt:
        case BinaryOp::Ge:
            if (!isNumeric(leftType) || !isNumeric(rightType)) {
                report(TypeChkError::ExpressionTypeMismatch, e, "relational operators require numeric operands");
            }
            return Type::Bool();
        case BinaryOp::Shl:
        case BinaryOp::Shr:
            if (!isInteger(leftType) || !isInteger(rightType)) {
                report(TypeChkError::AttemptedShiftOnNonInt, e, "shift operators require integer operands");
            }
            return Type::Int();
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            if (!isNumeric(leftType) || !isNumeric(rightType)) {
                report(TypeChkError::AttemptedAddOpOnNonNumeric, e, "arithmetic operators require numeric operands");
                return Type::Unknown();
            }
            if (leftType.kind == TypeKind::Float || rightType.kind == TypeKind::Float) {
                return Type::Float();
            }
            return Type::Int();
    }
    return Type::Unknown();
}

Type TypeChecker::checkCallExpression(const CallExpr* e) {
    const Symbol* fnSym = scope.getResolvedSymbolForCall(e);
    if (!fnSym || !fnSym->functionSig) {
        for (const auto& arg : e->args) checkExpression(arg.get());
        return Type::Unknown();
    }
    const FunctionSignature& sig = *fnSym->functionSig;
    if (e->args.size() != sig.paramTypes.size()) {
        report(TypeChkError::FnCallParamCount, e, "function call has incorrect number of arguments");
    }
    size_t n = e->args.size();
    if (sig.paramTypes.size() < n) n = sig.paramTypes.size();
    for (size_t i = 0; i < n; ++i) {
        Type argType = checkExpression(e->args[i].get());
        Type paramType = sig.paramTypes[i];
        if (argType.kind != TypeKind::Unknown &&
            argType.kind != paramType.kind) {
            report(TypeChkError::FnCallParamType, e->args[i].get(), "argument type does not match parameter type");
        }
    }
    if (sig.returnType) return *sig.returnType;
    return Type::Unknown();
}

Type TypeChecker::checkIndexExpression(const IndexExpr* e) {
    Type baseType = checkExpression(e->base.get());
    Type indexType = checkExpression(e->index.get());
    if (!isInteger(indexType) && indexType.kind != TypeKind::Unknown) {
        report(TypeChkError::ExpressionTypeMismatch, e->index.get(), "index expression must be integer");
    }
    return baseType;
}

#include "ir.hpp"

using namespace std;

IRGenerator::IRGenerator(const ScopeAnalyzer& s, const TypeChecker& t)
    : scope(s),
      types(t),
      currentFunction(nullptr),
      tempCounter(0),
      labelCounter(0) {}

IRProgram IRGenerator::generate(const Program& program) {
    irProgram.globals.clear();
    irProgram.functions.clear();
    diagnostics.clear();
    currentFunction = nullptr;
    tempCounter = 0;
    labelCounter = 0;
    for (const auto& d : program.decls) {
        generateTopLevelDecl(d.get());
    }
    return irProgram;
}

const vector<IRGenDiagnostic>& IRGenerator::getDiagnostics() const {
    return diagnostics;
}

bool IRGenerator::hasErrors() const {
    return !diagnostics.empty();
}

void IRGenerator::report(IRGenError kind, const Node* where, const string& message) {
    diagnostics.push_back(IRGenDiagnostic{kind, message, where});
}

string IRGenerator::createTemp() {
    string name = "%t" + to_string(tempCounter++);
    return name;
}

string IRGenerator::createLabel(const string& base) {
    string name = base + "_" + to_string(labelCounter++);
    return name;
}

void IRGenerator::emit(const IRInstr& instr) {
    if (currentFunction) {
        currentFunction->instructions.push_back(instr);
    }
}

void IRGenerator::generateTopLevelDecl(const Decl* decl) {
    if (auto* fn = dynamic_cast<const FunctionDecl*>(decl)) {
        generateFunction(fn);
    } else if (auto* tv = dynamic_cast<const TopVarDecl*>(decl)) {
        generateTopVar(tv);
    } else {
        report(IRGenError::UnsupportedStatement, decl, "unsupported top level declaration");
    }
}

void IRGenerator::generateTopVar(const TopVarDecl* tv) {
    const VarDeclStmt* s = tv->decl.get();
    IRGlobal g;
    g.name = s->name;
    g.type = s->type;
    g.hasInit = false;
    g.initValue = "";
    if (s->init) {
        const Expr* e = s->init->get();
        if (auto* il = dynamic_cast<const IntLit*>(e)) {
            g.hasInit = true;
            g.initValue = il->raw;
        } else if (auto* fl = dynamic_cast<const FloatLit*>(e)) {
            g.hasInit = true;
            g.initValue = fl->raw;
        } else if (auto* bl = dynamic_cast<const BoolLit*>(e)) {
            g.hasInit = true;
            g.initValue = bl->v ? "true" : "false";
        } else if (auto* sl = dynamic_cast<const StringLit*>(e)) {
            g.hasInit = true;
            g.initValue = "\"" + sl->v + "\"";
        } else if (auto* cl = dynamic_cast<const CharLit*>(e)) {
            g.hasInit = true;
            g.initValue = "'" + cl->v + "'";
        } else {
            report(IRGenError::UnsupportedExpression, e, "non-literal global initializer is not supported");
        }
    }
    irProgram.globals.push_back(g);
}

void IRGenerator::generateFunction(const FunctionDecl* fn) {
    irProgram.functions.emplace_back();
    IRFunction& f = irProgram.functions.back();
    f.name = fn->name;
    f.params.clear();
    for (const auto& p : fn->params) {
        f.params.push_back(p.name);
    }
    IRFunction* saved = currentFunction;
    currentFunction = &f;
    tempCounter = 0;
    generateBlock(fn->body.get());
    currentFunction = saved;
}

void IRGenerator::generateBlock(const BlockStmt* block) {
    for (const auto& s : block->stmts) {
        generateStatement(s.get());
    }
}

void IRGenerator::generateStatement(const Stmt* stmt) {
    if (auto* b = dynamic_cast<const BlockStmt*>(stmt)) {
        generateBlock(b);
        return;
    }
    if (auto* s = dynamic_cast<const IfStmt*>(stmt)) {
        generateIf(s);
        return;
    }
    if (auto* s = dynamic_cast<const WhileStmt*>(stmt)) {
        generateWhile(s);
        return;
    }
    if (auto* s = dynamic_cast<const ForStmt*>(stmt)) {
        generateFor(s);
        return;
    }
    if (auto* s = dynamic_cast<const ReturnStmt*>(stmt)) {
        generateReturn(s);
        return;
    }
    if (auto* s = dynamic_cast<const ExprStmt*>(stmt)) {
        generateExprStmt(s);
        return;
    }
    if (auto* s = dynamic_cast<const VarDeclStmt*>(stmt)) {
        generateVarDeclStmt(s);
        return;
    }
    report(IRGenError::UnsupportedStatement, stmt, "unsupported statement");
}

void IRGenerator::generateIf(const IfStmt* s) {
    string condTemp = generateExpr(s->cond.get());
    string thenLabel = createLabel("if_then");
    string elseLabel = s->elseS ? createLabel("if_else") : createLabel("if_end");
    string endLabel = s->elseS ? createLabel("if_end") : elseLabel;

    IRInstr ifg;
    ifg.kind = IRInstrKind::IfGoto;
    ifg.src1 = condTemp;
    ifg.info = thenLabel;
    emit(ifg);

    IRInstr g;
    g.kind = IRInstrKind::Goto;
    g.info = elseLabel;
    emit(g);

    IRInstr lt;
    lt.kind = IRInstrKind::Label;
    lt.info = thenLabel;
    emit(lt);
    generateStatement(s->thenS.get());

    if (s->elseS) {
        IRInstr g2;
        g2.kind = IRInstrKind::Goto;
        g2.info = endLabel;
        emit(g2);
        IRInstr le;
        le.kind = IRInstrKind::Label;
        le.info = elseLabel;
        emit(le);
        generateStatement(s->elseS->get());
        IRInstr lend;
        lend.kind = IRInstrKind::Label;
        lend.info = endLabel;
        emit(lend);
    } else {
        IRInstr lend;
        lend.kind = IRInstrKind::Label;
        lend.info = elseLabel;
        emit(lend);
    }
}

void IRGenerator::generateWhile(const WhileStmt* s) {
    string condLabel = createLabel("while_cond");
    string bodyLabel = createLabel("while_body");
    string endLabel = createLabel("while_end");

    IRInstr lc;
    lc.kind = IRInstrKind::Label;
    lc.info = condLabel;
    emit(lc);

    string condTemp = generateExpr(s->cond.get());
    IRInstr ifg;
    ifg.kind = IRInstrKind::IfGoto;
    ifg.src1 = condTemp;
    ifg.info = bodyLabel;
    emit(ifg);

    IRInstr gEnd;
    gEnd.kind = IRInstrKind::Goto;
    gEnd.info = endLabel;
    emit(gEnd);

    IRInstr lb;
    lb.kind = IRInstrKind::Label;
    lb.info = bodyLabel;
    emit(lb);
    generateStatement(s->body.get());

    IRInstr gBack;
    gBack.kind = IRInstrKind::Goto;
    gBack.info = condLabel;
    emit(gBack);

    IRInstr le;
    le.kind = IRInstrKind::Label;
    le.info = endLabel;
    emit(le);
}

void IRGenerator::generateFor(const ForStmt* s) {
    if (s->init) {
        generateStatement(s->init->get());
    }

    string condLabel = createLabel("for_cond");
    string bodyLabel = createLabel("for_body");
    string endLabel = createLabel("for_end");

    IRInstr lc;
    lc.kind = IRInstrKind::Label;
    lc.info = condLabel;
    emit(lc);

    if (s->cond) {
        string condTemp = generateExpr(s->cond->get());
        IRInstr ifg;
        ifg.kind = IRInstrKind::IfGoto;
        ifg.src1 = condTemp;
        ifg.info = bodyLabel;
        emit(ifg);
        IRInstr gEnd;
        gEnd.kind = IRInstrKind::Goto;
        gEnd.info = endLabel;
        emit(gEnd);
    } else {
        IRInstr gBody;
        gBody.kind = IRInstrKind::Goto;
        gBody.info = bodyLabel;
        emit(gBody);
    }

    IRInstr lb;
    lb.kind = IRInstrKind::Label;
    lb.info = bodyLabel;
    emit(lb);
    generateStatement(s->body.get());

    if (s->incr) {
        generateExpr(s->incr->get());
    }

    IRInstr gBack;
    gBack.kind = IRInstrKind::Goto;
    gBack.info = condLabel;
    emit(gBack);

    IRInstr le;
    le.kind = IRInstrKind::Label;
    le.info = endLabel;
    emit(le);
}

void IRGenerator::generateReturn(const ReturnStmt* s) {
    if (s->expr) {
        string temp = generateExpr(s->expr->get());
        IRInstr r;
        r.kind = IRInstrKind::Return;
        r.src1 = temp;
        emit(r);
    } else {
        IRInstr r;
        r.kind = IRInstrKind::ReturnVoid;
        emit(r);
    }
}

void IRGenerator::generateExprStmt(const ExprStmt* s) {
    generateExpr(s->expr.get());
}

void IRGenerator::generateVarDeclStmt(const VarDeclStmt* s) {
    if (s->init) {
        string temp = generateExpr(s->init->get());
        IRInstr a;
        a.kind = IRInstrKind::Assign;
        a.dst = s->name;
        a.src1 = temp;
        emit(a);
    }
}

string IRGenerator::generateExpr(const Expr* expr) {
    if (!expr) {
        report(IRGenError::UnsupportedExpression, nullptr, "empty expression");
        return "";
    }
    if (auto* il = dynamic_cast<const IntLit*>(expr)) {
        string t = createTemp();
        IRInstr a;
        a.kind = IRInstrKind::Assign;
        a.dst = t;
        a.src1 = il->raw;
        emit(a);
        return t;
    }
    if (auto* fl = dynamic_cast<const FloatLit*>(expr)) {
        string t = createTemp();
        IRInstr a;
        a.kind = IRInstrKind::Assign;
        a.dst = t;
        a.src1 = fl->raw;
        emit(a);
        return t;
    }
    if (auto* sl = dynamic_cast<const StringLit*>(expr)) {
        string t = createTemp();
        IRInstr a;
        a.kind = IRInstrKind::Assign;
        a.dst = t;
        a.src1 = "\"" + sl->v + "\"";
        emit(a);
        return t;
    }
    if (auto* cl = dynamic_cast<const CharLit*>(expr)) {
        string t = createTemp();
        IRInstr a;
        a.kind = IRInstrKind::Assign;
        a.dst = t;
        a.src1 = "'" + cl->v + "'";
        emit(a);
        return t;
    }
    if (auto* bl = dynamic_cast<const BoolLit*>(expr)) {
        string t = createTemp();
        IRInstr a;
        a.kind = IRInstrKind::Assign;
        a.dst = t;
        a.src1 = bl->v ? "true" : "false";
        emit(a);
        return t;
    }
    if (auto* id = dynamic_cast<const Ident*>(expr)) {
        return generateIdentifier(id);
    }
    if (auto* u = dynamic_cast<const UnaryExpr*>(expr)) {
        return generateUnary(u);
    }
    if (auto* b = dynamic_cast<const BinaryExpr*>(expr)) {
        return generateBinary(b);
    }
    if (auto* c = dynamic_cast<const CallExpr*>(expr)) {
        return generateCall(c);
    }
    if (auto* x = dynamic_cast<const IndexExpr*>(expr)) {
        return generateIndex(x);
    }
    report(IRGenError::UnsupportedExpression, expr, "unsupported expression");
    return "";
}

string IRGenerator::generateIdentifier(const Ident* id) {
    return id->name;
}

string IRGenerator::generateUnary(const UnaryExpr* e) {
    string rhs = generateExpr(e->rhs.get());
    string dst = createTemp();
    string op;
    if (e->op == UnaryOp::Not) op = "!";
    else if (e->op == UnaryOp::BitNot) op = "~";
    else if (e->op == UnaryOp::Neg) op = "-";
    else if (e->op == UnaryOp::Pos) op = "+";
    IRInstr u;
    u.kind = IRInstrKind::Unary;
    u.dst = dst;
    u.src1 = rhs;
    u.info = op;
    emit(u);
    return dst;
}

string IRGenerator::opStringForBinary(BinaryOp op) const {
    switch (op) {
        case BinaryOp::Or: return "||";
        case BinaryOp::And: return "&&";
        case BinaryOp::BitOr: return "|";
        case BinaryOp::BitXor: return "^";
        case BinaryOp::BitAnd: return "&";
        case BinaryOp::Eq: return "==";
        case BinaryOp::Neq: return "!=";
        case BinaryOp::Lt: return "<";
        case BinaryOp::Le: return "<=";
        case BinaryOp::Gt: return ">";
        case BinaryOp::Ge: return ">=";
        case BinaryOp::Shl: return "<<";
        case BinaryOp::Shr: return ">>";
        case BinaryOp::Add: return "+";
        case BinaryOp::Sub: return "-";
        case BinaryOp::Mul: return "*";
        case BinaryOp::Div: return "/";
        case BinaryOp::Mod: return "%";
        case BinaryOp::Assign: return "=";
    }
    return "?";
}

string IRGenerator::generateBinary(const BinaryExpr* e) {
    if (e->op == BinaryOp::Assign) {
        if (auto* id = dynamic_cast<const Ident*>(e->lhs.get())) {
            string rhs = generateExpr(e->rhs.get());
            IRInstr a;
            a.kind = IRInstrKind::Assign;
            a.dst = id->name;
            a.src1 = rhs;
            emit(a);
            return id->name;
        } else if (auto* idx = dynamic_cast<const IndexExpr*>(e->lhs.get())) {
            string base = generateExpr(idx->base.get());
            string index = generateExpr(idx->index.get());
            string rhs = generateExpr(e->rhs.get());
            IRInstr st;
            st.kind = IRInstrKind::IndexStore;
            st.dst = base;
            st.src1 = index;
            st.src2 = rhs;
            emit(st);
            return rhs;
        } else {
            report(IRGenError::InvalidAssignmentTarget, e->lhs.get(), "invalid assignment target");
            string rhs = generateExpr(e->rhs.get());
            return rhs;
        }
    }
    string left = generateExpr(e->lhs.get());
    string right = generateExpr(e->rhs.get());
    string dst = createTemp();
    string op = opStringForBinary(e->op);
    IRInstr b;
    b.kind = IRInstrKind::Binary;
    b.dst = dst;
    b.src1 = left;
    b.src2 = right;
    b.info = op;
    emit(b);
    return dst;
}

string IRGenerator::generateCall(const CallExpr* e) {
    for (const auto& arg : e->args) {
        string t = generateExpr(arg.get());
        IRInstr p;
        p.kind = IRInstrKind::Param;
        p.src1 = t;
        emit(p);
    }

    string funcName;
    if (auto* id = dynamic_cast<const Ident*>(e->callee.get())) {
        funcName = id->name;
    } else {
        funcName = "<call>";
    }

    const Symbol* fnSym = scope.getResolvedSymbolForCall(e);
    bool hasReturn = false;
    if (fnSym && fnSym->functionSig && fnSym->functionSig->returnType) {
        hasReturn = true;
    }

    IRInstr c;
    c.kind = IRInstrKind::Call;
    c.info = funcName;
    c.src1 = to_string(e->args.size());

    if (hasReturn) {
        string dst = createTemp();
        c.dst = dst;
        emit(c);
        return dst;
    } else {
        c.dst = "";
        emit(c);
        return "";
    }
}

string IRGenerator::generateIndex(const IndexExpr* e) {
    string base = generateExpr(e->base.get());
    string index = generateExpr(e->index.get());
    string dst = createTemp();
    IRInstr i;
    i.kind = IRInstrKind::IndexLoad;
    i.dst = dst;
    i.src1 = base;
    i.src2 = index;
    emit(i);
    return dst;
}

void printIRProgram(const IRProgram& ir, ostream& os) {
    for (const auto& g : ir.globals) {
        os << "global " << g.type.str() << " " << g.name;
        if (g.hasInit) {
            os << " = " << g.initValue;
        }
        os << "\n";
    }
    if (!ir.globals.empty()) os << "\n";
    for (const auto& fn : ir.functions) {
        os << "function " << fn.name << "(";
        for (size_t i = 0; i < fn.params.size(); ++i) {
            os << fn.params[i];
            if (i + 1 < fn.params.size()) os << ", ";
        }
        os << ")\n";
        for (const auto& ins : fn.instructions) {
            os << "  ";
            switch (ins.kind) {
                case IRInstrKind::Label:
                    os << ins.info << ":";
                    break;
                case IRInstrKind::Goto:
                    os << "goto " << ins.info;
                    break;
                case IRInstrKind::IfGoto:
                    os << "if " << ins.src1 << " goto " << ins.info;
                    break;
                case IRInstrKind::Assign:
                    os << ins.dst << " = " << ins.src1;
                    break;
                case IRInstrKind::Unary:
                    os << ins.dst << " = " << ins.info << ins.src1;
                    break;
                case IRInstrKind::Binary:
                    os << ins.dst << " = " << ins.src1 << " " << ins.info << " " << ins.src2;
                    break;
                case IRInstrKind::Param:
                    os << "param " << ins.src1;
                    break;
                case IRInstrKind::Call:
                    if (!ins.dst.empty()) {
                        os << ins.dst << " = call " << ins.info << ", " << ins.src1;
                    } else {
                        os << "call " << ins.info << ", " << ins.src1;
                    }
                    break;
                case IRInstrKind::Return:
                    os << "return " << ins.src1;
                    break;
                case IRInstrKind::ReturnVoid:
                    os << "return";
                    break;
                case IRInstrKind::IndexLoad:
                    os << ins.dst << " = " << ins.src1 << "[" << ins.src2 << "]";
                    break;
                case IRInstrKind::IndexStore:
                    os << ins.dst << "[" << ins.src1 << "] = " << ins.src2;
                    break;
            }
            os << "\n";
        }
        os << "end\n\n";
    }
}

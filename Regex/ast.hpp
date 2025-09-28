#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <optional>
#include <utility>

using namespace std;

enum class TypeKind { Int, Float, Bool, String, Char, Unknown };

struct Type {
    TypeKind kind = TypeKind::Unknown;
    static Type Int()    { return {TypeKind::Int}; }
    static Type Float()  { return {TypeKind::Float}; }
    static Type Bool()   { return {TypeKind::Bool}; }
    static Type String() { return {TypeKind::String}; }
    static Type Char()   { return {TypeKind::Char}; }
    static Type Unknown(){ return {TypeKind::Unknown}; }
    string str() const {
        switch (kind) {
            case TypeKind::Int:    return "int";
            case TypeKind::Float:  return "float";
            case TypeKind::Bool:   return "bool";
            case TypeKind::String: return "string";
            case TypeKind::Char:   return "char";
            default:               return "unknown";
        }
    }
};

struct Node {
    virtual ~Node() = default;
    virtual void print(ostream& os, int indent = 0) const = 0;
};

inline void indent(ostream& os, int n){ for(int i=0;i<n;i++) os << ' '; }

struct Expr; struct Stmt; struct Decl;
using ExprPtr = shared_ptr<Expr>;
using StmtPtr = shared_ptr<Stmt>;
using DeclPtr = shared_ptr<Decl>;

struct Expr : Node {};
struct Stmt : Node {};
struct Decl : Node {};

enum class UnaryOp { Not, BitNot, Neg, Pos };
enum class BinaryOp {
    Assign,
    Or, And,
    BitOr, BitXor, BitAnd,
    Eq, Neq,
    Lt, Le, Gt, Ge,
    Shl, Shr,
    Add, Sub,
    Mul, Div, Mod
};

struct IntLit : Expr {
    string raw; long long v{};
    explicit IntLit(string r, long long val): raw(move(r)), v(val) {}
    void print(ostream& os, int i) const override { indent(os,i); os<<"IntLit("<<raw<<")\n"; }
};
struct FloatLit : Expr {
    string raw; double v{};
    explicit FloatLit(string r, double val): raw(move(r)), v(val) {}
    void print(ostream& os, int i) const override { indent(os,i); os<<"FloatLit("<<raw<<")\n"; }
};
struct StringLit : Expr {
    string v;
    explicit StringLit(string s): v(move(s)) {}
    void print(ostream& os, int i) const override { indent(os,i); os<<"StringLit(\""<<v<<"\")\n"; }
};
struct CharLit : Expr {
    string v;
    explicit CharLit(string s): v(move(s)) {}
    void print(ostream& os, int i) const override { indent(os,i); os<<"CharLit('"<<v<<"')\n"; }
};
struct BoolLit : Expr {
    bool v{};
    explicit BoolLit(bool b): v(b) {}
    void print(ostream& os, int i) const override { indent(os,i); os<<"BoolLit("<<(v?"true":"false")<<")\n"; }
};
struct Ident : Expr {
    string name;
    explicit Ident(string s): name(move(s)) {}
    void print(ostream& os, int i) const override { indent(os,i); os<<"Ident("<<name<<")\n"; }
};

struct UnaryExpr : Expr {
    UnaryOp op; ExprPtr rhs;
    UnaryExpr(UnaryOp o, ExprPtr r): op(o), rhs(move(r)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"Unary("<<(op==UnaryOp::Not?"!": op==UnaryOp::BitNot?"~": op==UnaryOp::Neg?"-":"+")<<")\n";
        rhs->print(os, i+2);
    }
};
struct BinaryExpr : Expr {
    BinaryOp op; ExprPtr lhs, rhs;
    BinaryExpr(BinaryOp o, ExprPtr a, ExprPtr b): op(o), lhs(move(a)), rhs(move(b)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"Binary(";
        switch(op){
            case BinaryOp::Assign: os<<"="; break;
            case BinaryOp::Or: os<<"||"; break; case BinaryOp::And: os<<"&&"; break;
            case BinaryOp::BitOr: os<<"|"; break; case BinaryOp::BitXor: os<<"^"; break; case BinaryOp::BitAnd: os<<"&"; break;
            case BinaryOp::Eq: os<<"=="; break; case BinaryOp::Neq: os<<"!="; break;
            case BinaryOp::Lt: os<<"<"; break; case BinaryOp::Le: os<<"<="; break; case BinaryOp::Gt: os<<">"; break; case BinaryOp::Ge: os<<">="; break;
            case BinaryOp::Shl: os<<"<<"; break; case BinaryOp::Shr: os<<">>"; break;
            case BinaryOp::Add: os<<"+"; break; case BinaryOp::Sub: os<<"-"; break;
            case BinaryOp::Mul: os<<"*"; break; case BinaryOp::Div: os<<"/"; break; case BinaryOp::Mod: os<<"%"; break;
        }
        os<<")\n";
        lhs->print(os, i+2);
        rhs->print(os, i+2);
    }
};
struct CallExpr : Expr {
    ExprPtr callee; vector<ExprPtr> args;
    CallExpr(ExprPtr c, vector<ExprPtr> a): callee(move(c)), args(move(a)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"Call\n";
        indent(os,i+2); os<<"Callee:\n"; callee->print(os, i+4);
        indent(os,i+2); os<<"Args:\n";
        for (auto& a: args) a->print(os, i+4);
    }
};
struct IndexExpr : Expr {
    ExprPtr base, index;
    IndexExpr(ExprPtr b, ExprPtr idx): base(move(b)), index(move(idx)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"Index\n";
        base->print(os, i+2);
        index->print(os, i+2);
    }
};

struct BlockStmt : Stmt {
    vector<StmtPtr> stmts;
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"Block\n";
        for (auto& s: stmts) s->print(os, i+2);
    }
};
struct ExprStmt : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e): expr(move(e)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"ExprStmt\n"; expr->print(os, i+2);
    }
};
struct ReturnStmt : Stmt {
    optional<ExprPtr> expr;
    explicit ReturnStmt(optional<ExprPtr> e): expr(move(e)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"Return\n";
        if (expr) (*expr)->print(os, i+2);
    }
};
struct IfStmt : Stmt {
    ExprPtr cond; StmtPtr thenS; optional<StmtPtr> elseS;
    IfStmt(ExprPtr c, StmtPtr t, optional<StmtPtr> e): cond(move(c)), thenS(move(t)), elseS(move(e)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"If\n";
        indent(os,i+2); os<<"Cond:\n"; cond->print(os, i+4);
        indent(os,i+2); os<<"Then:\n"; thenS->print(os, i+4);
        if (elseS){ indent(os,i+2); os<<"Else:\n"; (*elseS)->print(os, i+4); }
    }
};
struct WhileStmt : Stmt {
    ExprPtr cond; StmtPtr body;
    WhileStmt(ExprPtr c, StmtPtr b): cond(move(c)), body(move(b)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"While\n";
        indent(os,i+2); os<<"Cond:\n"; cond->print(os, i+4);
        indent(os,i+2); os<<"Body:\n"; body->print(os, i+4);
    }
};
struct ForStmt : Stmt {
    optional<StmtPtr> init; optional<ExprPtr> cond; optional<ExprPtr> incr; StmtPtr body;
    ForStmt(optional<StmtPtr> i, optional<ExprPtr> c, optional<ExprPtr> n, StmtPtr b)
        : init(move(i)), cond(move(c)), incr(move(n)), body(move(b)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"For\n";
        indent(os,i+2); os<<"Init:\n"; if (init) (*init)->print(os, i+4);
        indent(os,i+2); os<<"Cond:\n"; if (cond) (*cond)->print(os, i+4);
        indent(os,i+2); os<<"Incr:\n"; if (incr) (*incr)->print(os, i+4);
        indent(os,i+2); os<<"Body:\n"; body->print(os, i+4);
    }
};
struct VarDeclStmt : Stmt {
    Type type; string name; optional<ExprPtr> init;
    VarDeclStmt(Type t, string n, optional<ExprPtr> i): type(t), name(move(n)), init(move(i)) {}
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"VarDecl("<<type.str()<<" "<<name<<")\n";
        if (init){ indent(os,i+2); os<<"Init:\n"; (*init)->print(os, i+4); }
    }
};

struct Param { Type type; string name; };

struct FunctionDecl : Decl {
    string name; vector<Param> params; optional<Type> retType; shared_ptr<BlockStmt> body;
    void print(ostream& os, int i) const override {
        indent(os,i); os<<"Function "<<name<<"\n";
        indent(os,i+2); os<<"Params:\n";
        for (auto& p: params){ indent(os,i+4); os<<p.type.str()<<" "<<p.name<<"\n"; }
        if (retType){ indent(os,i+2); os<<"ReturnType: "<<retType->str()<<"\n"; }
        indent(os,i+2); os<<"Body:\n"; body->print(os, i+4);
    }
};
struct TopVarDecl : Decl {
    shared_ptr<VarDeclStmt> decl;
    explicit TopVarDecl(shared_ptr<VarDeclStmt> d): decl(move(d)) {}
    void print(ostream& os, int i) const override { indent(os,i); os<<"TopVar\n"; decl->print(os, i+2); }
};

struct Program : Node {
    vector<DeclPtr> decls;
    void print(ostream& os, int i = 0) const override {
        indent(os,i); os<<"Program\n";
        for (auto& d: decls) d->print(os, i+2);
    }
};

#include "parser.hpp"
#include <cstdlib>
#include <limits>
#include <cctype>
#include <memory>
#include <utility>

using namespace std;

static bool isBoolIdent(const Token& tok){
    return tok.type == TokenType::T_IDENTIFIER && (tok.lexeme=="true" || tok.lexeme=="false");
}

Parser::Parser(vector<Token> toks, string src)
    : tokens(move(toks)), source(move(src)) {}

void Parser::pushScope(){ scopes.emplace_back(); }
void Parser::popScope(){ if (!scopes.empty()) scopes.pop_back(); }
void Parser::declareVar(const string& name, TypeKind k){
    if (scopes.empty()) pushScope();
    scopes.back()[name] = k;
}
optional<TypeKind> Parser::lookupVar(const string& name) const{
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it){
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return nullopt;
}
static ParseError expected_error_for(TypeKind k){
    switch (k){
        case TypeKind::Bool:   return ParseError::ExpectedBoolLit;
        case TypeKind::Int:    return ParseError::ExpectedIntLit;
        case TypeKind::Float:  return ParseError::ExpectedFloatLit;
        case TypeKind::String: return ParseError::ExpectedStringLit;
        default:               return ParseError::ExpectedExpr;
    }
}
void Parser::checkLiteralAgainst(TypeKind expected, const ExprPtr& rhs, const char* contextMsg){
    TypeKind got = TypeKind::Unknown;
    if (dynamic_cast<IntLit*>(rhs.get()))        got = TypeKind::Int;
    else if (dynamic_cast<FloatLit*>(rhs.get())) got = TypeKind::Float;
    else if (dynamic_cast<BoolLit*>(rhs.get()))  got = TypeKind::Bool;
    else if (dynamic_cast<StringLit*>(rhs.get()))got = TypeKind::String;
    else if (dynamic_cast<CharLit*>(rhs.get()))  got = TypeKind::Char;

    if (got != TypeKind::Unknown && got != expected){
        auto ek = expected_error_for(expected);
        string msg = string(contextMsg) + ": initializer/assignment literal does not match declared type";
        throw ParseException(ek, msg);
    }
}

bool Parser::atEnd() const { return i >= tokens.size(); }
const Token& Parser::peek() const {
    if (atEnd()) throw ParseException(ParseError::UnexpectedEOF, "Unexpected end of input");
    return tokens[i];
}
const Token& Parser::prev() const { return tokens[i-1]; }
const Token& Parser::advance() { const Token& t = peek(); ++i; return t; }
bool Parser::check(TokenType t) const { return !atEnd() && tokens[i].type == t; }
bool Parser::match(initializer_list<TokenType> list){
    if (atEnd()) return false;
    for (auto t: list){ if (tokens[i].type == t){ ++i; return true; } }
    return false;
}
const Token& Parser::expect(TokenType t, ParseError errKind, const char* msg){
    if (check(t)) return advance();
    if (atEnd())
        throw ParseException(ParseError::UnexpectedEOF, string("Expected ")+msg+" before EOF");
    throw ParseException(errKind, string("Expected ")+msg+", got "+toString(peek()), peek());
}

shared_ptr<Program> Parser::parse(){
    auto prog = make_shared<Program>();
    pushScope();
    while (!atEnd()){
        prog->decls.push_back(parseTopLevel());
    }
    popScope();
    return prog;
}

DeclPtr Parser::parseTopLevel(){
    if (match({TokenType::T_FUNCTION})){
        --i;
        return parseFunction();
    }
    if (check(TokenType::T_INT) || check(TokenType::T_FLOAT) || check(TokenType::T_BOOL)
        || check(TokenType::T_STRING) || check(TokenType::T_CHAR)) {
        auto vd = parseVarDeclStmt();
        expect(TokenType::T_SEMICOLON, ParseError::FailedToFindToken, "';'");
        return make_shared<TopVarDecl>(vd);
    }
    throw ParseException(ParseError::UnexpectedToken, "Unexpected token at top-level: " + toString(peek()), peek());
}

shared_ptr<FunctionDecl> Parser::parseFunction(){
    expect(TokenType::T_FUNCTION, ParseError::FailedToFindToken, "'fn'");
    const Token& nameTok = expect(TokenType::T_IDENTIFIER, ParseError::ExpectedIdentifier, "function name");
    expect(TokenType::T_PARENL, ParseError::FailedToFindToken, "'('");
    vector<Param> params;
    if (!check(TokenType::T_PARENR)){
        params = parseParams();
    }
    expect(TokenType::T_PARENR, ParseError::FailedToFindToken, "')'");
    pushScope();
    for (const auto& p : params) declareVar(p.name, p.type.kind);
    auto body = parseBlock();
    popScope();
    auto fn = make_shared<FunctionDecl>();
    fn->name = nameTok.lexeme;
    fn->params = move(params);
    fn->retType = nullopt;
    fn->body = body;
    return fn;
}

vector<Param> Parser::parseParams(){
    vector<Param> ps;
    ps.push_back(parseParam());
    while (match({TokenType::T_COMMA})){
        ps.push_back(parseParam());
    }
    return ps;
}
Param Parser::parseParam(){
    Type t = parseType();
    const Token& id = expect(TokenType::T_IDENTIFIER, ParseError::ExpectedIdentifier, "parameter name");
    return Param{t, id.lexeme};
}
Type Parser::parseType(){
    if      (match({TokenType::T_INT}))    return Type::Int();
    else if (match({TokenType::T_FLOAT}))  return Type::Float();
    else if (match({TokenType::T_BOOL}))   return Type::Bool();
    else if (match({TokenType::T_STRING})) return Type::String();
    else if (match({TokenType::T_CHAR}))   return Type::Char();
    throw ParseException(ParseError::ExpectedTypeToken, "Expected a type token (int|float|bool|string|char)", peek());
}

shared_ptr<BlockStmt> Parser::parseBlock(){
    expect(TokenType::T_BRACEL, ParseError::FailedToFindToken, "'{'");
    pushScope();
    auto blk = make_shared<BlockStmt>();
    while (!check(TokenType::T_BRACER)){
        blk->stmts.push_back(parseStmt());
    }
    expect(TokenType::T_BRACER, ParseError::FailedToFindToken, "'}'");
    popScope();
    return blk;
}
StmtPtr Parser::parseStmt(){
    if (check(TokenType::T_BRACEL)) return parseBlock();
    if (match({TokenType::T_IF}))    return parseIf();
    if (match({TokenType::T_WHILE})) return parseWhile();
    if (match({TokenType::T_FOR}))   return parseFor();
    if (match({TokenType::T_RETURN}))return parseReturn();
    if (check(TokenType::T_INT) || check(TokenType::T_FLOAT) || check(TokenType::T_BOOL)
        || check(TokenType::T_STRING) || check(TokenType::T_CHAR)) {
        auto vd = parseVarDeclStmt();
        expect(TokenType::T_SEMICOLON, ParseError::FailedToFindToken, "';'");
        return vd;
    }
    return parseExprStmt();
}
StmtPtr Parser::parseIf(){
    expect(TokenType::T_PARENL, ParseError::FailedToFindToken, "'(' after if");
    auto cond = parseExpr();
    expect(TokenType::T_PARENR, ParseError::FailedToFindToken, "')' after if condition");
    auto thenS = parseStmt();
    optional<StmtPtr> elseS;
    if (match({TokenType::T_ELSE})) elseS = parseStmt();
    return make_shared<IfStmt>(cond, thenS, elseS);
}
StmtPtr Parser::parseWhile(){
    expect(TokenType::T_PARENL, ParseError::FailedToFindToken, "'(' after while");
    auto cond = parseExpr();
    expect(TokenType::T_PARENR, ParseError::FailedToFindToken, "')' after while condition");
    auto body = parseStmt();
    return make_shared<WhileStmt>(cond, body);
}
StmtPtr Parser::parseFor(){
    expect(TokenType::T_PARENL, ParseError::FailedToFindToken, "'(' after for");
    optional<StmtPtr> init;
    if (!check(TokenType::T_SEMICOLON)){
        if (check(TokenType::T_INT) || check(TokenType::T_FLOAT) || check(TokenType::T_BOOL)
            || check(TokenType::T_STRING) || check(TokenType::T_CHAR)) {
            init = parseVarDeclStmt();
        } else {
            auto e = parseExpr();
            init = make_shared<ExprStmt>(e);
        }
    }
    expect(TokenType::T_SEMICOLON, ParseError::FailedToFindToken, "';' after for init");
    optional<ExprPtr> cond;
    if (!check(TokenType::T_SEMICOLON)){
        cond = parseExpr();
    }
    expect(TokenType::T_SEMICOLON, ParseError::FailedToFindToken, "';' after for condition");
    optional<ExprPtr> incr;
    if (!check(TokenType::T_PARENR)){
        incr = parseExpr();
    }
    expect(TokenType::T_PARENR, ParseError::FailedToFindToken, "')' after for increment");
    auto body = parseStmt();
    return make_shared<ForStmt>(init, cond, incr, body);
}
StmtPtr Parser::parseReturn(){
    if (!check(TokenType::T_SEMICOLON)){
        auto e = parseExpr();
        expect(TokenType::T_SEMICOLON, ParseError::FailedToFindToken, "';' after return expr");
        return make_shared<ReturnStmt>(e);
    } else {
        expect(TokenType::T_SEMICOLON, ParseError::FailedToFindToken, "';' after return");
        return make_shared<ReturnStmt>(nullopt);
    }
}
StmtPtr Parser::parseExprStmt(){
    auto e = parseExpr();
    expect(TokenType::T_SEMICOLON, ParseError::FailedToFindToken, "';' after expression");
    return make_shared<ExprStmt>(e);
}

shared_ptr<VarDeclStmt> Parser::parseVarDeclStmt(){
    Type t = parseType();
    const Token& nameTok = expect(TokenType::T_IDENTIFIER, ParseError::ExpectedIdentifier, "variable name");
    while (match({TokenType::T_BRACKETL})){
        if (!check(TokenType::T_BRACKETR)){
            parseExpr();
        }
        expect(TokenType::T_BRACKETR, ParseError::FailedToFindToken, "']' after array declarator");
    }
    optional<ExprPtr> init;
    if (match({TokenType::T_ASSIGNOP})){
        ExprPtr rhs = parseExpr();
        init = rhs;
        checkLiteralAgainst(t.kind, rhs, "Variable initialization");
    }
    declareVar(nameTok.lexeme, t.kind);
    return make_shared<VarDeclStmt>(t, nameTok.lexeme, init);
}

ExprPtr Parser::parseExpr(){ return parseAssignment(); }

ExprPtr Parser::parseAssignment(){
    auto left = parseOr();
    if (match({TokenType::T_ASSIGNOP})){
        auto rhs = parseAssignment();
        if (auto *id = dynamic_cast<Ident*>(left.get())){
            if (auto k = lookupVar(id->name)){
                checkLiteralAgainst(*k, rhs, "Assignment");
            }
        }
        return make_shared<BinaryExpr>(BinaryOp::Assign, left, rhs);
    }
    return left;
}
ExprPtr Parser::parseOr(){
    auto e = parseAnd();
    while (match({TokenType::T_OROR})){
        auto r = parseAnd();
        e = make_shared<BinaryExpr>(BinaryOp::Or, e, r);
    }
    return e;
}
ExprPtr Parser::parseAnd(){
    auto e = parseBitOr();
    while (match({TokenType::T_ANDAND})){
        auto r = parseBitOr();
        e = make_shared<BinaryExpr>(BinaryOp::And, e, r);
    }
    return e;
}
ExprPtr Parser::parseBitOr(){
    auto e = parseBitXor();
    while (match({TokenType::T_PIPE})){
        auto r = parseBitXor();
        e = make_shared<BinaryExpr>(BinaryOp::BitOr, e, r);
    }
    return e;
}
ExprPtr Parser::parseBitXor(){
    auto e = parseBitAnd();
    while (match({TokenType::T_CARET})){
        auto r = parseBitAnd();
        e = make_shared<BinaryExpr>(BinaryOp::BitXor, e, r);
    }
    return e;
}
ExprPtr Parser::parseBitAnd(){
    auto e = parseEquality();
    while (match({TokenType::T_AMP})){
        auto r = parseEquality();
        e = make_shared<BinaryExpr>(BinaryOp::BitAnd, e, r);
    }
    return e;
}
ExprPtr Parser::parseEquality(){
    auto e = parseRel();
    while (match({TokenType::T_EQUALSOP, TokenType::T_NOTEQ})){
        TokenType op = prev().type;
        auto r = parseRel();
        e = make_shared<BinaryExpr>(op==TokenType::T_EQUALSOP? BinaryOp::Eq : BinaryOp::Neq, e, r);
    }
    return e;
}
ExprPtr Parser::parseRel(){
    auto e = parseShift();
    while (match({TokenType::T_LT, TokenType::T_LE, TokenType::T_GT, TokenType::T_GE})){
        TokenType op = prev().type;
        auto r = parseShift();
        BinaryOp bop = BinaryOp::Lt;
        if (op==TokenType::T_LT) bop=BinaryOp::Lt;
        else if (op==TokenType::T_LE) bop=BinaryOp::Le;
        else if (op==TokenType::T_GT) bop=BinaryOp::Gt;
        else bop=BinaryOp::Ge;
        e = make_shared<BinaryExpr>(bop, e, r);
    }
    return e;
}
ExprPtr Parser::parseShift(){
    auto e = parseAdd();
    while (match({TokenType::T_SHL, TokenType::T_SHR})){
        TokenType op = prev().type;
        auto r = parseAdd();
        e = make_shared<BinaryExpr>(op==TokenType::T_SHL? BinaryOp::Shl : BinaryOp::Shr, e, r);
    }
    return e;
}
ExprPtr Parser::parseAdd(){
    auto e = parseMul();
    while (match({TokenType::T_PLUS, TokenType::T_MINUS})){
        TokenType op = prev().type;
        auto r = parseMul();
        e = make_shared<BinaryExpr>(op==TokenType::T_PLUS? BinaryOp::Add : BinaryOp::Sub, e, r);
    }
    return e;
}
ExprPtr Parser::parseMul(){
    auto e = parseUnary();
    while (match({TokenType::T_STAR, TokenType::T_SLASH, TokenType::T_PERCENT})){
        TokenType op = prev().type;
        auto r = parseUnary();
        BinaryOp bop = (op==TokenType::T_STAR? BinaryOp::Mul :
                        op==TokenType::T_SLASH? BinaryOp::Div : BinaryOp::Mod);
        e = make_shared<BinaryExpr>(bop, e, r);
    }
    return e;
}
ExprPtr Parser::parseUnary(){
    if (match({TokenType::T_NOT}))  return make_shared<UnaryExpr>(UnaryOp::Not,    parseUnary());
    if (match({TokenType::T_TILDE}))return make_shared<UnaryExpr>(UnaryOp::BitNot, parseUnary());
    if (match({TokenType::T_MINUS}))return make_shared<UnaryExpr>(UnaryOp::Neg,    parseUnary());
    if (match({TokenType::T_PLUS})) return make_shared<UnaryExpr>(UnaryOp::Pos,    parseUnary());
    return parsePostfix();
}
ExprPtr Parser::parsePostfix(){
    auto e = parsePrimary();
    for(;;){
        if (match({TokenType::T_PARENL})){
            vector<ExprPtr> args;
            if (!check(TokenType::T_PARENR)){
                args.push_back(parseExpr());
                while (match({TokenType::T_COMMA})){
                    args.push_back(parseExpr());
                }
            }
            expect(TokenType::T_PARENR, ParseError::FailedToFindToken, "')' after call args");
            e = make_shared<CallExpr>(e, move(args));
        } else if (match({TokenType::T_BRACKETL})){
            auto idx = parseExpr();
            expect(TokenType::T_BRACKETR, ParseError::FailedToFindToken, "']' after index");
            e = make_shared<IndexExpr>(e, idx);
        } else {
            break;
        }
    }
    return e;
}
ExprPtr Parser::parsePrimary(){
    if (match({TokenType::T_INTLIT})){
        const auto& t = prev();
        long long v = strtoll(t.value.c_str(), nullptr, 10);
        return make_shared<IntLit>(t.value, v);
    }
    if (match({TokenType::T_FLOATLIT})){
        const auto& t = prev();
        double v = strtod(t.value.c_str(), nullptr);
        return make_shared<FloatLit>(t.value, v);
    }
    if (match({TokenType::T_STRINGLIT})){
        const auto& t = prev();
        return make_shared<StringLit>(t.value);
    }
    if (match({TokenType::T_CHARLIT})){
        const auto& t = prev();
        return make_shared<CharLit>(t.value);
    }
    if (!atEnd() && isBoolIdent(peek())){
        const auto& t = advance();
        return make_shared<BoolLit>(t.lexeme == "true");
    }
    if (match({TokenType::T_IDENTIFIER})){
        return make_shared<Ident>(prev().lexeme);
    }
    if (match({TokenType::T_PARENL})){
        auto e = parseExpr();
        expect(TokenType::T_PARENR, ParseError::FailedToFindToken, "')' to close grouping");
        return e;
    }
    if (atEnd())
        throw ParseException(ParseError::UnexpectedEOF, "Expected expression, found EOF");
    throw ParseException(ParseError::ExpectedExpr, "Expected expression, got " + toString(peek()), peek());
}

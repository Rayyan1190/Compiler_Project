#include "lexer.hpp"
#include <stdexcept>
#include <utility>

using namespace std;

static pair<int,int> lineColOf(const string& text, size_t pos) {
    int line = 1, col = 1;
    for (size_t i = 0; i < pos && i < text.size(); ++i) {
        if (text[i] == '\n') { ++line; col = 1; }
        else { ++col; }
    }
    return {line, col};
}

static string decodeEscape(char n) {
    switch (n) {
        case 'n': return string(1, '\n');
        case 't': return string(1, '\t');
        case 'r': return string(1, '\r');
        case 'b': return string(1, '\b');
        case 'f': return string(1, '\f');
        case 'v': return string(1, '\v');
        case '\\':return string(1, '\\');
        case '\'':return string(1, '\'');
        case '"': return string(1, '"');
        default: throw runtime_error("Invalid escape sequence");
    }
}

Lexer::Lexer(string src): s(move(src)), n(s.size()), i(0) { initTables(); }

bool Lexer::isAlpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool Lexer::isDigit(char c) {
    return (c >= '0' && c <= '9');
}
bool Lexer::isAlnum(char c) {
    return isAlpha(c) || isDigit(c);
}

bool Lexer::eof() const { return i >= n; }
char Lexer::peek(size_t off) const { return (i + off < n) ? s[i + off] : '\0'; }
char Lexer::advance() { return s[i++]; }

bool Lexer::match(const char* lit) {
    size_t L = 0; while (lit[L] != '\0') ++L;
    if (i + L > n) return false;
    for (size_t k = 0; k < L; ++k) if (s[i + k] != lit[k]) return false;
    i += L; return true;
}

void Lexer::skipSpaceAndComments() {
    bool moved = true;
    while (moved && !eof()) {
        moved = false;
        while (!eof() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
            ++i; moved = true;
        }
        if (eof()) break;
        if (peek() == '/' && peek(1) == '/') {
            i += 2; moved = true;
            while (!eof() && peek() != '\n') ++i;
            continue;
        }
        if (peek() == '/' && peek(1) == '*') {
            size_t start = i;
            i += 2; moved = true;
            bool closed = false;
            while (!eof()) {
                if (peek() == '*' && peek(1) == '/') { i += 2; closed = true; break; }
                ++i;
            }
            if (!closed) {
                auto [ln, cl] = lineColOf(s, start);
                throw runtime_error("Unterminated block comment at line " + to_string(ln) + ", col " + to_string(cl));
            }
            continue;
        }
    }
}

Token Lexer::tok(TokenType t, const string& lex, const string& val, size_t start) {
    return Token{t, lex, val, start};
}
Token Lexer::tok(TokenType t, const string& lex, const string& val) {
    return Token{t, lex, val, i};
}

Token Lexer::scanString() {
    size_t start = i;
    advance();
    string val;
    while (!eof()) {
        char c = advance();
        if (c == '"') {
            string lex = s.substr(start, i - start);
            return tok(TokenType::T_STRINGLIT, lex, val, start);
        }
        if (c == '\\') {
            if (eof()) throw runtime_error("Unterminated string constant");
            char n = advance();
            val += decodeEscape(n);
        } else {
            val.push_back(c);
        }
    }
    throw runtime_error("Unterminated string constant");
}

Token Lexer::scanChar() {
    size_t start = i;
    advance();
    if (eof()) throw runtime_error("Missing closing ' in character literal");
    string val;
    char c = advance();
    if (c == '\\') {
        if (eof()) throw runtime_error("Missing closing ' in character literal");
        char n = advance();
        val += decodeEscape(n);
    } else {
        val.push_back(c);
    }
    if (eof()) throw runtime_error("Missing closing ' in character literal");
    char close = advance();
    if (close != '\'') {
        throw runtime_error("Multi-character character constant");
    }
    string lex = s.substr(start, i - start);
    return tok(TokenType::T_CHARLIT, lex, val, start);
}

Token Lexer::scanIdent() {
    size_t start = i;
    advance();
    while (!eof() && (isAlnum(peek()) || peek() == '_')) advance();
    string w = s.substr(start, i - start);
    if (kw.count(w)) return tok(kw[w], w, "", start);
    if (ty.count(w)) return tok(ty[w], w, "", start);
    return tok(TokenType::T_IDENTIFIER, w, w, start);
}

Token Lexer::scanNumber() {
    size_t start = i;
    bool isFloat = false;
    if (peek() == '.') {
        isFloat = true;
        advance();
        while (!eof() && isDigit(peek())) advance();
    } else {
        while (!eof() && isDigit(peek())) advance();
        if (!eof() && peek() == '.') {
            isFloat = true;
            advance();
            while (!eof() && isDigit(peek())) advance();
        }
    }
    if (!eof() && (peek() == 'e' || peek() == 'E')) {
        size_t save = i;
        advance();
        if (!eof() && (peek() == '+' || peek() == '-')) advance();
        if (eof() || !isDigit(peek())) {
            i = save;
        } else {
            isFloat = true;
            while (!eof() && isDigit(peek())) advance();
        }
    }
    string lex = s.substr(start, i - start);
    if (isFloat) return tok(TokenType::T_FLOATLIT, lex, lex, start);
    return tok(TokenType::T_INTLIT, lex, lex, start);
}

void Lexer::initTables() {
    kw = {{"fn",TokenType::T_FUNCTION},{"return",TokenType::T_RETURN},
          {"if",TokenType::T_IF},{"else",TokenType::T_ELSE},
          {"for",TokenType::T_FOR},{"while",TokenType::T_WHILE}};
    ty = {{"int",TokenType::T_INT},{"float",TokenType::T_FLOAT},
          {"bool",TokenType::T_BOOL},{"string",TokenType::T_STRING},
          {"char",TokenType::T_CHAR}};
}

vector<Token> Lexer::tokenize() {
    vector<Token> out;
    struct Delim { char ch; size_t at; };
    vector<Delim> dstack;
    auto push_delim = [&](TokenType t, size_t at) {
        char c = 0;
        if (t == TokenType::T_PARENL)   c = '(';
        if (t == TokenType::T_BRACEL)   c = '{';
        if (t == TokenType::T_BRACKETL) c = '[';
        if (c) dstack.push_back({c, at});
    };
    auto pop_delim = [&](TokenType t, size_t at) {
        char need = 0;
        if (t == TokenType::T_PARENR)   need = '(';
        if (t == TokenType::T_BRACER)   need = '{';
        if (t == TokenType::T_BRACKETR) need = '[';
        if (!need) return;
        if (dstack.empty() || dstack.back().ch != need) {
            auto [ln, cl] = lineColOf(s, at);
            throw runtime_error("Mismatched closing delimiter at line " + to_string(ln) + ", col " + to_string(cl));
        }
        dstack.pop_back();
    };

    while (!eof()) {
        skipSpaceAndComments();
        if (eof()) break;
        size_t startPos = i;
        char c = peek();
        if (isDigit(c) || (c == '.' && isDigit(peek(1)))) {
            size_t startBefore = i;
            Token num = scanNumber();
            if (!eof() && (isAlpha(peek()) || peek() == '_')) {
                while (!eof() && (isAlnum(peek()) || peek() == '_')) advance();
                string bad = s.substr(startBefore, i - startBefore);
                auto [ln, cl] = lineColOf(s, startBefore);
                throw runtime_error("Invalid numeric literal at line " + to_string(ln) + ", col " + to_string(cl) + ": '" + bad + "'");
            }
            out.push_back(num);
            continue;
        }
        if (isAlpha(c) || c == '_') {
            out.push_back(scanIdent());
            continue;
        }
        if (c == '"') {
            out.push_back(scanString());
            continue;
        }
        if (c == '\'') {
            out.push_back(scanChar());
            continue;
        }
        if (match("&&")) { out.push_back(tok(TokenType::T_ANDAND, "&&", "", startPos)); continue; }
        if (match("||")) { out.push_back(tok(TokenType::T_OROR, "||", "", startPos)); continue; }
        if (match("==")) { out.push_back(tok(TokenType::T_EQUALSOP, "==", "", startPos)); continue; }
        if (match("!=")) { out.push_back(tok(TokenType::T_NOTEQ, "!=", "", startPos)); continue; }
        if (match("<=")) { out.push_back(tok(TokenType::T_LE, "<=", "", startPos)); continue; }
        if (match(">=")) { out.push_back(tok(TokenType::T_GE, ">=", "", startPos)); continue; }
        if (match("<<")) { out.push_back(tok(TokenType::T_SHL, "<<", "", startPos)); continue; }
        if (match(">>")) { out.push_back(tok(TokenType::T_SHR, ">>", "", startPos)); continue; }
        switch (c) {
            case '=': advance(); out.push_back(tok(TokenType::T_ASSIGNOP, "=", "", startPos)); break;
            case '<': advance(); out.push_back(tok(TokenType::T_LT, "<", "", startPos)); break;
            case '>': advance(); out.push_back(tok(TokenType::T_GT, ">", "", startPos)); break;
            case '!': advance(); out.push_back(tok(TokenType::T_NOT, "!", "", startPos)); break;
            case '+': advance(); out.push_back(tok(TokenType::T_PLUS, "+", "", startPos)); break;
            case '-': advance(); out.push_back(tok(TokenType::T_MINUS, "-", "", startPos)); break;
            case '*': advance(); out.push_back(tok(TokenType::T_STAR, "*", "", startPos)); break;
            case '/': advance(); out.push_back(tok(TokenType::T_SLASH, "/", "", startPos)); break;
            case '%': advance(); out.push_back(tok(TokenType::T_PERCENT, "%", "", startPos)); break;
            case '&': advance(); out.push_back(tok(TokenType::T_AMP, "&", "", startPos)); break;
            case '|': advance(); out.push_back(tok(TokenType::T_PIPE, "|", "", startPos)); break;
            case '^': advance(); out.push_back(tok(TokenType::T_CARET, "^", "", startPos)); break;
            case '~': advance(); out.push_back(tok(TokenType::T_TILDE, "~", "", startPos)); break;
            case '(': advance(); out.push_back(tok(TokenType::T_PARENL, "(", "", startPos)); push_delim(TokenType::T_PARENL, startPos); break;
            case ')': advance(); out.push_back(tok(TokenType::T_PARENR, ")", "", startPos)); pop_delim(TokenType::T_PARENR, startPos); break;
            case '{': advance(); out.push_back(tok(TokenType::T_BRACEL, "{", "", startPos)); push_delim(TokenType::T_BRACEL, startPos); break;
            case '}': advance(); out.push_back(tok(TokenType::T_BRACER, "}", "", startPos)); pop_delim(TokenType::T_BRACER, startPos); break;
            case '[': advance(); out.push_back(tok(TokenType::T_BRACKETL, "[", "", startPos)); push_delim(TokenType::T_BRACKETL, startPos); break;
            case ']': advance(); out.push_back(tok(TokenType::T_BRACKETR, "]", "", startPos)); pop_delim(TokenType::T_BRACKETR, startPos); break;
            case ',': advance(); out.push_back(tok(TokenType::T_COMMA, ",", "", startPos)); break;
            case ';': advance(); out.push_back(tok(TokenType::T_SEMICOLON, ";", "", startPos)); break;
            default: {
                auto [ln, cl] = lineColOf(s, startPos);
                string sym(1, c);
                throw runtime_error("Unrecognized symbol " + sym + " at line " + to_string(ln) + ", col " + to_string(cl));
            }
        }
    }
    if (!dstack.empty()) {
        auto last = dstack.back();
        auto [ln, cl] = lineColOf(s, last.at);
        string which = (last.ch=='(' ? "opening '('" :
                        last.ch=='{' ? "opening '{'" :
                        last.ch=='[' ? "opening '['" : "opening delimiter");
        throw runtime_error("Unclosed " + which + " starting at line " + to_string(ln) + ", col " + to_string(cl));
    }
    return out;
}

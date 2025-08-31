#include "lexer.hpp"
#include <unordered_map>
#include <regex>
#include <stdexcept>
#include <algorithm>
#include <string_view>

using namespace std;

struct Rule { TokenType type; regex pattern; };

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

static string unescapeString(const string& raw) {
    string s;
    for (size_t i = 1; i + 1 < raw.size(); ++i) {
        char c = raw[i];
        if (c == '\\' && i + 1 < raw.size()) {
            char n = raw[++i];
            s += decodeEscape(n);
        } else {
            s.push_back(c);
        }
    }
    return s;
}

static string unescapeChar(const string& raw) {
    if (raw.size() < 3 || raw.front()!='\'' || raw.back()!='\'')
        throw runtime_error("Missing closing ' in character literal");
    string inner = raw.substr(1, raw.size()-2);
    if (inner.empty())
        throw runtime_error("Missing closing ' in character literal");
    if (inner[0] == '\\') {
        if (inner.size()!=2)
            throw runtime_error("Multi-character character constant");
        return decodeEscape(inner[1]);
    } else {
        if (inner.size()!=1)
            throw runtime_error("Multi-character character constant");
        return inner;
    }
}

using svmatch = match_results<string_view::const_iterator>;
inline static bool regex_search_sv(string_view sv, const regex& re, svmatch& m) {
    return regex_search(sv.begin(), sv.end(), m, re);
}

static pair<int,int> lineColOf(const string& text, size_t pos) {
    int line = 1, col = 1;
    for (size_t i = 0; i < pos && i < text.size(); ++i) {
        if (text[i] == '\n') { ++line; col = 1; }
        else { ++col; }
    }
    return {line, col};
}

static regex whitespace{R"(^\s+)"};
static regex lineComment{R"(^//[^\n]*)"};
static regex blockComment{R"(^/\*[^*]*\*+([^/*][^*]*\*+)*/)"};
static regex strLit{R"(^"(\\.|[^"\\])*")"};
static regex charValid{R"(^'(\\.|[^'\\])')"};
static regex floatLit{R"(^(?:\d+\.\d*|\d*\.\d+)(?:[eE][+-]?\d+)?)"};
static regex intLit{R"(^\d+)"};
static regex identOrKeyword{R"(^[A-Za-z_]\w*)"};

static unordered_map<string, TokenType> kw;
static unordered_map<string, TokenType> ty;
static vector<Rule> rules;

Lexer::Lexer(string src): input(move(src)), pos(0) { buildRules(); }

static string_view curSV(const string& s, size_t pos) {
    return string_view(s).substr(pos);
}

void Lexer::buildRules() {
    kw = {{"fn",TokenType::T_FUNCTION},{"return",TokenType::T_RETURN},
          {"if",TokenType::T_IF},{"else",TokenType::T_ELSE},
          {"for",TokenType::T_FOR},{"while",TokenType::T_WHILE}};
    ty = {{"int",TokenType::T_INT},{"float",TokenType::T_FLOAT},
          {"bool",TokenType::T_BOOL},{"string",TokenType::T_STRING},
          {"char",TokenType::T_CHAR}};
    rules.clear();
    auto R = [&](TokenType t, const string& pat){ rules.push_back({t, regex("^" + pat)}); };
    R(TokenType::T_ANDAND,  R"(&&)");
    R(TokenType::T_OROR,    R"(\|\|)");
    R(TokenType::T_EQUALSOP,R"(==)");
    R(TokenType::T_NOTEQ,   R"(!=)");
    R(TokenType::T_LE,      R"(<=)");
    R(TokenType::T_GE,      R"(>=)");
    R(TokenType::T_SHL,     R"(<<)");
    R(TokenType::T_SHR,     R"(>>)");
    R(TokenType::T_ASSIGNOP, R"(=)");
    R(TokenType::T_LT,       R"(<)");
    R(TokenType::T_GT,       R"(>)");
    R(TokenType::T_NOT,      R"(!)");
    R(TokenType::T_PLUS,     R"(\+)");
    R(TokenType::T_MINUS,    R"(-)");
    R(TokenType::T_STAR,     R"(\*)");
    R(TokenType::T_SLASH,    R"(/)");
    R(TokenType::T_PERCENT,  R"(%)");
    R(TokenType::T_AMP,      R"(&)");
    R(TokenType::T_PIPE,     R"(\|)");
    R(TokenType::T_CARET,    R"(\^)");
    R(TokenType::T_TILDE,    R"(~)");
    R(TokenType::T_PARENL,   R"(\()");
    R(TokenType::T_PARENR,   R"(\))");
    R(TokenType::T_BRACEL,   R"(\{)");
    R(TokenType::T_BRACER,   R"(\})");
    R(TokenType::T_BRACKETL, R"(\[)");
    R(TokenType::T_BRACKETR, R"(\])");
    R(TokenType::T_COMMA,    R"(,)");
    R(TokenType::T_SEMICOLON,R"(;)");
}

void Lexer::skipSpaceAndComments() {
    bool moved = true;
    while (moved) {
        moved = false;
        svmatch m; auto v = curSV(input, pos);
        if (regex_search_sv(v, whitespace, m)) { pos += m.length(); moved = true; continue; }
        v = curSV(input, pos);
        if (regex_search_sv(v, lineComment, m)) { pos += m.length(); moved = true; continue; }
        v = curSV(input, pos);
        if (v.size() >= 2 && v[0] == '/' && v[1] == '*') {
            svmatch mb;
            if (regex_search_sv(v, blockComment, mb)) { pos += mb.length(); moved = true; continue; }
            auto [ln, cl] = lineColOf(input, pos);
            throw runtime_error("Unterminated block comment at line " + to_string(ln) + ", col " + to_string(cl));
        }
    }
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
            auto [ln, cl] = lineColOf(input, at);
            throw runtime_error("Mismatched closing delimiter at line " + to_string(ln) + ", col " + to_string(cl));
        }
        dstack.pop_back();
    };

    while (pos < input.size()) {
        skipSpaceAndComments();
        if (pos >= input.size()) break;
        {
            static const regex badNumeric(R"(^\d+[A-Za-z_]\w*)");
            svmatch m; auto v = curSV(input, pos);
            if (regex_search_sv(v, badNumeric, m)) {
                auto [ln, cl] = lineColOf(input, pos);
                throw runtime_error(string("Invalid numeric literal at line ") + to_string(ln) + ", col " + to_string(cl) + ": '" + m.str() + "'");
            }
        }
        bool matched = false;
        {
            svmatch m; auto v = curSV(input, pos);
            if (regex_search_sv(v, charValid, m)) {
                string raw = m.str();
                string val = unescapeChar(raw);
                out.push_back(Token{TokenType::T_CHARLIT, raw, val, pos});
                pos += m.length();
                matched = true;
            } else if (!matched && v.size() && v[0]=='\'') {
                auto rest = string_view(input).substr(pos + 1);
                auto it = find(rest.begin(), rest.end(), '\'');
                if (it == rest.end()) {
                    throw runtime_error("Missing closing ' in character literal");
                } else {
                    if (rest.size() >= 2 && rest[0] == '\\' && (pos + 3 <= input.size())) {
                        char esc = rest[1];
                        try { (void)decodeEscape(esc); }
                        catch (...) { throw runtime_error("Invalid escape sequence"); }
                    }
                    throw runtime_error("Multi-character character constant");
                }
            }
            if (matched) continue;
        }
        for (const auto& r : rules) {
            svmatch m; auto v = curSV(input, pos);
            if (regex_search_sv(v, r.pattern, m)) {
                string lex = m.str();
                Token tok{r.type, lex, "", pos};
                switch (r.type) {
                    case TokenType::T_PARENL:
                    case TokenType::T_BRACEL:
                    case TokenType::T_BRACKETL:
                        push_delim(r.type, pos);
                        break;
                    case TokenType::T_PARENR:
                    case TokenType::T_BRACER:
                    case TokenType::T_BRACKETR:
                        pop_delim(r.type, pos);
                        break;
                    default: break;
                }
                out.push_back(tok);
                pos += m.length();
                matched = true;
                break;
            }
        }
        if (matched) continue;
        svmatch m; auto v = curSV(input, pos);
        if (regex_search_sv(v, identOrKeyword, m)) {
            string w = m.str();
            Token tok;
            tok.startPos = pos;
            if (kw.count(w))      tok = Token{kw[w], w, "", pos};
            else if (ty.count(w)) tok = Token{ty[w], w, "", pos};
            else                  tok = Token{TokenType::T_IDENTIFIER, w, w, pos};
            out.push_back(tok);
            pos += m.length();
        } else if ((v = curSV(input, pos), regex_search_sv(v, floatLit, m))) {
            out.push_back(Token{TokenType::T_FLOATLIT, m.str(), m.str(), pos});
            pos += m.length();
        } else if ((v = curSV(input, pos), regex_search_sv(v, intLit, m))) {
            out.push_back(Token{TokenType::T_INTLIT, m.str(), m.str(), pos});
            pos += m.length();
        } else if ((v = curSV(input, pos), regex_search_sv(v, strLit, m))) {
            out.push_back(Token{TokenType::T_STRINGLIT, m.str(), unescapeString(m.str()), pos});
            pos += m.length();
        } else {
            if (input[pos] == '"') {
                throw runtime_error("Unterminated string constant");
            }
            auto [ln, cl] = lineColOf(input, pos);
            string sym(1, input[pos]);
            throw runtime_error("Unrecognized symbol " + sym + " at line " + to_string(ln) + ", col " + to_string(cl));
        }
    }
    if (!dstack.empty()) {
        auto last = dstack.back();
        auto [ln, cl] = lineColOf(input, last.at);
        string which = (last.ch=='(' ? "opening '('" :
                        last.ch=='{' ? "opening '{'" :
                        last.ch=='[' ? "opening '['" : "opening delimiter");
        throw runtime_error("Unclosed " + which + " starting at line " + to_string(ln) + ", col " + to_string(cl));
    }
    return out;
}

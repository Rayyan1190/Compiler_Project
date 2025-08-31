#include "token.hpp"
#include <string>
using namespace std;

static string escapeForPrint(const string& s) {
    string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            case '\"': out += "\\\""; break;
            case '\'': out += "\\\'"; break;
            case '\\': out += "\\\\"; break;
            default:   out.push_back(c); break;
        }
    }
    return out;
}

string toString(const Token& t) {
    auto nameOnly = [&](const char* n){ return string(n); };
    auto withVal  = [&](const char* n, const string& v){
        switch (t.type) {
            case TokenType::T_IDENTIFIER: return string(n) + "(\"" + v + "\")";
            case TokenType::T_STRINGLIT:  return string(n) + "(\"" + escapeForPrint(v) + "\")";
            case TokenType::T_CHARLIT:    return string(n) + "('" + escapeForPrint(v) + "')";
            case TokenType::T_INTLIT:
            case TokenType::T_FLOATLIT:   return string(n) + "(" + v + ")";
            default: return string(n);
        }
    };
    switch (t.type) {
        case TokenType::T_FUNCTION:  return nameOnly("T_FUNCTION");
        case TokenType::T_RETURN:    return nameOnly("T_RETURN");
        case TokenType::T_IF:        return nameOnly("T_IF");
        case TokenType::T_ELSE:      return nameOnly("T_ELSE");
        case TokenType::T_FOR:       return nameOnly("T_FOR");
        case TokenType::T_WHILE:     return nameOnly("T_WHILE");
        case TokenType::T_INT:       return nameOnly("T_INT");
        case TokenType::T_FLOAT:     return nameOnly("T_FLOAT");
        case TokenType::T_BOOL:      return nameOnly("T_BOOL");
        case TokenType::T_STRING:    return nameOnly("T_STRING");
        case TokenType::T_CHAR:      return nameOnly("T_CHAR");
        case TokenType::T_IDENTIFIER:return withVal("T_IDENTIFIER", t.value);
        case TokenType::T_INTLIT:    return withVal("T_INTLIT", t.value);
        case TokenType::T_FLOATLIT:  return withVal("T_FLOATLIT", t.value);
        case TokenType::T_STRINGLIT: return withVal("T_STRINGLIT", t.value);
        case TokenType::T_CHARLIT:   return withVal("T_CHARLIT", t.value);
        case TokenType::T_PARENL:    return nameOnly("T_PARENL");
        case TokenType::T_PARENR:    return nameOnly("T_PARENR");
        case TokenType::T_BRACEL:    return nameOnly("T_BRACEL");
        case TokenType::T_BRACER:    return nameOnly("T_BRACER");
        case TokenType::T_BRACKETL:  return nameOnly("T_BRACKETL");
        case TokenType::T_BRACKETR:  return nameOnly("T_BRACKETR");
        case TokenType::T_COMMA:     return nameOnly("T_COMMA");
        case TokenType::T_SEMICOLON: return nameOnly("T_SEMICOLON");
        case TokenType::T_ASSIGNOP:  return nameOnly("T_ASSIGNOP");
        case TokenType::T_EQUALSOP:  return nameOnly("T_EQUALSOP");
        case TokenType::T_NOTEQ:     return nameOnly("T_NOTEQ");
        case TokenType::T_LE:        return nameOnly("T_LE");
        case TokenType::T_GE:        return nameOnly("T_GE");
        case TokenType::T_LT:        return nameOnly("T_LT");
        case TokenType::T_GT:        return nameOnly("T_GT");
        case TokenType::T_ANDAND:    return nameOnly("T_ANDAND");
        case TokenType::T_OROR:      return nameOnly("T_OROR");
        case TokenType::T_NOT:       return nameOnly("T_NOT");
        case TokenType::T_PLUS:      return nameOnly("T_PLUS");
        case TokenType::T_MINUS:     return nameOnly("T_MINUS");
        case TokenType::T_STAR:      return nameOnly("T_STAR");
        case TokenType::T_SLASH:     return nameOnly("T_SLASH");
        case TokenType::T_PERCENT:   return nameOnly("T_PERCENT");
        case TokenType::T_AMP:       return nameOnly("T_AMP");
        case TokenType::T_PIPE:      return nameOnly("T_PIPE");
        case TokenType::T_CARET:     return nameOnly("T_CARET");
        case TokenType::T_TILDE:     return nameOnly("T_TILDE");
        case TokenType::T_SHL:       return nameOnly("T_SHL");
        case TokenType::T_SHR:       return nameOnly("T_SHR");
        default: return "UNKNOWN";
    }
}

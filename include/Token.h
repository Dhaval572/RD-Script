#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class t_TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // One or two character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // Literals
    IDENTIFIER, STRING, NUMBER,

    // Keywords
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    DISPLAY, RETURN, SUPER, THIS, TRUE, WHILE,

    // Rubber Duck specific keywords
    AUTO, LINKEDLIST, DYNAMIC_ARRAY,

    EOF_TOKEN
};

struct t_Token {
    t_TokenType type;
    std::string lexeme;
    // For literals, we could use a variant or void* to store the value
    // For simplicity in this example, we'll just store as string
    std::string literal;
    int line;

    t_Token(t_TokenType type, const std::string& lexeme, const std::string& literal, int line)
        : type(type), lexeme(lexeme), literal(literal), line(line) {}
};

#endif // TOKEN_H
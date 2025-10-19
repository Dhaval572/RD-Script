#pragma once
#include <string>

enum class t_TokenType
{
    // Single-character tokens
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    DOT,
    MINUS,
    PLUS,
    SEMICOLON,
    SLASH,
    STAR,

    // One or two character tokens
    BANG,
    BANG_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    PLUS_PLUS,      // ++
    MINUS_MINUS,    // --
    
    // Compound assignment operators
    PLUS_EQUAL,     // +=
    MINUS_EQUAL,    // -=
    STAR_EQUAL,     // *=
    SLASH_EQUAL,    // /=

    // Literals
    IDENTIFIER,
    STRING,
    FORMAT_STRING,
    NUMBER,

    // Keywords
    AND,
    BREAK,
    CLASS,
    CONTINUE,
    ELSE,
    FALSE,
    FUN,
    FOR,
    IF,
    NIL,
    OR,
    DISPLAY,
    RETURN,
    SUPER,
    THIS,
    TRUE,
    WHILE,

    // Rubber Duck specific keywords - keeping only essential ones
    AUTO,
    BENCHMARK,  // New benchmark keyword
    // Removed advanced functionality keywords: LINKEDLIST, DYNAMIC_ARRAY

    EOF_TOKEN
};

struct t_Token
{
    t_TokenType type;
    std::string lexeme;
    // For literals, we could use a variant or void* to store the value
    // For simplicity in this example, we'll just store as string
    std::string literal;
    int line;

    t_Token
    (
        t_TokenType type, 
        const std::string &lexeme, 
        const std::string &literal, 
        int line
    ) 
    : type(type), 
      lexeme(lexeme), 
      literal(literal), 
      line(line) {}
};
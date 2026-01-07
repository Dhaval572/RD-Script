#pragma once
#include <string>

enum class e_TokenType
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
    MODULUS,        

    // One or two character tokens
    BANG,
    BANG_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    PLUS_PLUS,      
    MINUS_MINUS,  
    // Compound assignment operators
    PLUS_EQUAL,     
    MINUS_EQUAL,    
    STAR_EQUAL,     
    SLASH_EQUAL,    
    MODULUS_EQUAL,  

    // Literals
    IDENTIFIER,
    STRING,
    FORMAT_STRING,
    NUMBER,

    // Keywords
    AND,
    BREAK,
    CLASS,
    CONST,
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

    // Rubber Duck specific keywords 
    AUTO,
    BENCHMARK,  
    GETIN,      
    TYPEOF, // TODO
    EOF_TOKEN
};

struct t_Token
{
    e_TokenType type;
    std::string lexeme;
    // For literals, we could use a variant or void* to store the value
    // For simplicity in this example, we'll just store as string
    std::string literal;
    int line;

    explicit t_Token
    (
        e_TokenType type, 
        const std::string &lexeme, 
        const std::string &literal, 
        int line
    ) 
    : type(type), 
      lexeme(lexeme), 
      literal(literal), 
      line(line) {}
};
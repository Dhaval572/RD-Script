#pragma once

#include <string>
#include <vector>
#include "Token.h"

class t_Lexer
{
private:
    std::string source;
    std::vector<t_Token> tokens;
    int start;
    int current;
    int line;

    bool IsAtEnd();
    void ScanToken();
    char Advance();
    void AddToken(t_TokenType type);
    void AddToken(t_TokenType type, const std::string &literal);
    bool Match(char expected);
    char Peek();
    char PeekNext();

    // Helpers for literals
    void String();
    void FormatString(); // Add this declaration
    void Number();
    void Identifier();

    // Keyword checking
    t_TokenType IdentifierType();

public:
    t_Lexer(const std::string &source);
    std::vector<t_Token> ScanTokens();
};
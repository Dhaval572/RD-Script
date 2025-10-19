#pragma once

#include <string>
#include <vector>
#include "Token.h"
#include "ErrorHandling.h"

class t_Lexer
{
private:
    std::string source;
    std::vector<t_Token> tokens;
    int start;
    int current;
    int line;

    bool IsAtEnd();
    t_ParsingResult ScanToken();
    char Advance();
    void AddToken(t_TokenType type);
    void AddToken(t_TokenType type, const std::string &literal);
    bool Match(char expected);
    char Peek();
    char PeekNext();

    // Helpers for literals
    t_Expected<std::string, t_ErrorInfo> String();
    t_Expected<std::string, t_ErrorInfo> FormatString(); 
    void Number();
    void Identifier();

    // Keyword checking
    t_TokenType IdentifierType();

public:
    t_Lexer(const std::string &source);
    t_ParsingResult ScanTokens();
};
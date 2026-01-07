#pragma once

#include <string>
#include <vector>
#include <rubberduck/Token.h>
#include <rubberduck/ErrorHandling.h>

class Lexer
{
private:
    std::string m_Source;
    std::vector<t_Token> m_Tokens;
    int m_Start;
    int m_Current;
    int m_Line;

    bool IsAtEnd();
    ParsingResult ScanToken();
    char Advance();
    void AddToken(e_TokenType type);
    void AddToken(e_TokenType type, const std::string &literal);
    bool Match(char expected);
    char Peek();
    char PeekNext();

    // Helpers for literals
    Expected<std::string, t_ErrorInfo> String();
    Expected<std::string, t_ErrorInfo> FormatString(); 
    void Number();
    void Identifier();

    // Keyword checking
    e_TokenType IdentifierType();

public:
    explicit Lexer(const std::string &source);
    ParsingResult ScanTokens();
};
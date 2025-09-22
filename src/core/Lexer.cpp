#include "Lexer.h"
#include <unordered_map>
#include <cctype>
#include <stdexcept>

static const std::unordered_map<std::string, t_TokenType> keywords =
{
    {"and", t_TokenType::AND},
    {"class", t_TokenType::CLASS},
    {"else", t_TokenType::ELSE},
    {"false", t_TokenType::FALSE},
    {"for", t_TokenType::FOR},
    {"fun", t_TokenType::FUN},
    {"if", t_TokenType::IF},
    {"nil", t_TokenType::NIL},
    {"or", t_TokenType::OR},
    {"Display", t_TokenType::DISPLAY},
    {"return", t_TokenType::RETURN},
    {"super", t_TokenType::SUPER},
    {"this", t_TokenType::THIS},
    {"true", t_TokenType::TRUE},
    {"while", t_TokenType::WHILE},
    {"Auto", t_TokenType::AUTO}
    // Removed advanced functionality keywords: linkedlist, dynamic_array
};

t_Lexer::t_Lexer(const std::string &source)
    : source(source), start(0), current(0), line(1) {}

std::vector<t_Token> t_Lexer::ScanTokens()
{
    while (!IsAtEnd())
    {
        start = current;
        ScanToken();
    }

    tokens.emplace_back(t_TokenType::EOF_TOKEN, "", "", line);
    return tokens;
}

bool t_Lexer::IsAtEnd()
{
    return current >= static_cast<int>(source.length());
}

void t_Lexer::ScanToken()
{
    char c = Advance();
    switch (c)
    {
    case '(':
        AddToken(t_TokenType::LEFT_PAREN);
        break;
    case ')':
        AddToken(t_TokenType::RIGHT_PAREN);
        break;
    case '{':
        AddToken(t_TokenType::LEFT_BRACE);
        break;
    case '}':
        AddToken(t_TokenType::RIGHT_BRACE);
        break;
    case ',':
        AddToken(t_TokenType::COMMA);
        break;
    case '.':
        AddToken(t_TokenType::DOT);
        break;
    case '-':
        AddToken(t_TokenType::MINUS);
        break;
    case '+':
        AddToken(t_TokenType::PLUS);
        break;
    case ';':
        AddToken(t_TokenType::SEMICOLON);
        break;
    case '*':
        AddToken(t_TokenType::STAR);
        break;
    case '!':
        AddToken(Match('=') ? t_TokenType::BANG_EQUAL : t_TokenType::BANG);
        break;
    case '=':
        AddToken(Match('=') ? t_TokenType::EQUAL_EQUAL : t_TokenType::EQUAL);
        break;
    case '<':
        AddToken(Match('=') ? t_TokenType::LESS_EQUAL : t_TokenType::LESS);
        break;
    case '>':
        AddToken(Match('=') ? t_TokenType::GREATER_EQUAL : t_TokenType::GREATER);
        break;
    case '/':
        if (Match('/'))
        {
            // A comment goes until the end of the line.
            while (Peek() != '\n' && !IsAtEnd())
                Advance();
        }
        else
        {
            AddToken(t_TokenType::SLASH);
        }
        break;
    case ' ':
    case '\r':
    case '\t':
        // Ignore whitespace.
        break;
    case '\n':
        line++;
        break;
    case '"':
        String();
        break;
    default:
        if (std::isdigit(c))
        {
            Number();
        }
        else if (std::isalpha(c) || c == '_')
        {
            // Check if this is a format string starting with 'k'
            if (c == 'k' && Peek() == '"') 
            {
                // Consume the opening quote
                Advance();
                FormatString();
            } 
            else 
            {
                Identifier();
            }
        }
        else
        {
            // Error: unexpected character
            throw std::runtime_error("Unexpected character at line " + std::to_string(line));
        }
        break;
    }
}

char t_Lexer::Advance()
{
    current++;
    return source[current - 1];
}

void t_Lexer::AddToken(t_TokenType type)
{
    AddToken(type, "");
}

void t_Lexer::AddToken(t_TokenType type, const std::string &literal)
{
    std::string text = source.substr(start, current - start);
    tokens.emplace_back(type, text, literal, line);
}

bool t_Lexer::Match(char expected)
{
    if (IsAtEnd())
        return false;
    if (source[current] != expected)
        return false;

    current++;
    return true;
}

char t_Lexer::Peek()
{
    if (IsAtEnd())
        return '\0';
    return source[current];
}

char t_Lexer::PeekNext()
{
    if (current + 1 >= static_cast<int>(source.length()))
        return '\0';
    return source[current + 1];
}

void t_Lexer::String()
{
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n')
            line++;
        Advance();
    }

    if (IsAtEnd())
    {
        // Error: Unterminated string.
        throw std::runtime_error("Unterminated string at line " + std::to_string(line));
    }

    // The closing ".
    Advance();

    // Trim the surrounding quotes.
    std::string value = source.substr(start + 1, current - start - 2);
    AddToken(t_TokenType::STRING, value);
}

void t_Lexer::FormatString()
{
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n')
            line++;
        Advance();
    }

    if (IsAtEnd())
    {
        // Error: Unterminated string.
        throw std::runtime_error("Unterminated format string at line " + std::to_string(line));
    }

    // The closing ".
    Advance();

    // Trim the surrounding quotes.
    std::string value = "k" + source.substr(start + 2, current - start - 3); // Include the 'k' prefix
    AddToken(t_TokenType::STRING, value);
}

void t_Lexer::Number()
{
    while (std::isdigit(Peek()))
        Advance();

    // Look for a fractional part.
    if (Peek() == '.' && std::isdigit(PeekNext()))
    {
        // Consume the "."
        Advance();

        while (std::isdigit(Peek()))
            Advance();
    }

    AddToken(t_TokenType::NUMBER, source.substr(start, current - start));
}

void t_Lexer::Identifier()
{
    while (std::isalnum(Peek()) || Peek() == '_')
        Advance();

    std::string text = source.substr(start, current - start);
    t_TokenType type = t_TokenType::IDENTIFIER;

    auto it = keywords.find(text);
    if (it != keywords.end())
    {
        type = it->second;
    }

    AddToken(type);
}
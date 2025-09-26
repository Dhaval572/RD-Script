#include "Lexer.h"
#include <unordered_map>
#include <cctype>
#include <stdexcept>

static const std::unordered_map<std::string, t_TokenType> keywords =
{
    {"and", t_TokenType::AND},
    {"break", t_TokenType::BREAK},
    {"class", t_TokenType::CLASS},
    {"continue", t_TokenType::CONTINUE},
    {"else", t_TokenType::ELSE},
    {"false", t_TokenType::FALSE},
    {"for", t_TokenType::FOR},
    {"fun", t_TokenType::FUN},
    {"if", t_TokenType::IF},
    {"nil", t_TokenType::NIL},
    {"or", t_TokenType::OR},
    {"display", t_TokenType::DISPLAY},
    {"return", t_TokenType::RETURN},
    {"super", t_TokenType::SUPER},
    {"this", t_TokenType::THIS},
    {"true", t_TokenType::TRUE},
    {"while", t_TokenType::WHILE},
    {"auto", t_TokenType::AUTO}
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
        if (Match('-')) 
        {
            AddToken(t_TokenType::MINUS_MINUS);
        } 
        else 
        {
            AddToken(t_TokenType::MINUS);
        }
        break;
    case '+':
        if (Match('+')) 
        {
            AddToken(t_TokenType::PLUS_PLUS);
        } 
        else 
        {
            AddToken(t_TokenType::PLUS);
        }
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
    case '&':
        if (Match('&')) 
        {
            AddToken(t_TokenType::AND);
        } 
        else 
        {
            throw std::runtime_error("Unexpected character at line " + std::to_string(line));
        }
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
            while (Peek() != '\n' && !IsAtEnd()) Advance();
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
        {
            std::string value = String();
            AddToken(t_TokenType::STRING, value);
        }
        break;
    case '$':
        if (Peek() == '"') 
        {
            Advance(); // Consume the '"'
            std::string value = FormatString();
            AddToken(t_TokenType::FORMAT_STRING, value);
        }
        else
        {
            throw std::runtime_error("Unexpected character at line " + std::to_string(line));
        }
        break;
    default:
        if (std::isdigit(c))
        {
            Number();
        }
        else if (std::isalpha(c) || c == '_')
        {
            Identifier();
        }
        else
        {
            throw std::runtime_error("Unexpected character at line " + std::to_string(line));
        }
        break;
    }
}

bool t_Lexer::Match(char expected)
{
    if (IsAtEnd()) return false;
    if (source[current] != expected) return false;

    current++;
    return true;
}

char t_Lexer::Peek()
{
    if (IsAtEnd()) return '\0';
    return source[current];
}

char t_Lexer::PeekNext()
{
    if (current + 1 >= static_cast<int>(source.length())) return '\0';
    return source[current + 1];
}

std::string t_Lexer::String()
{
    std::string value;
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n') line++;
        value += Advance();
    }

    if (IsAtEnd())
    {
        throw std::runtime_error
        (
            "Unterminated string at line " + std::to_string(line)
        );
    }

    // The closing ".
    Advance();

    // Trim the surrounding quotes.
    return value;
}

std::string t_Lexer::FormatString()
{
    std::string value = "$"; // Preserve the $ prefix for format string identification
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n') line++;
        value += Advance();
    }

    if (IsAtEnd())
    {
        throw std::runtime_error
        (
            "Unterminated format string at line " + std::to_string(line)
        );
    }

    // The closing ".
    Advance();

    // Return the value including the $ prefix
    return value;
}

void t_Lexer::Number()
{
    while (std::isdigit(Peek())) Advance();

    // Look for a fractional part.
    if (Peek() == '.' && std::isdigit(PeekNext()))
    {
        // Consume the "."
        Advance();

        while (std::isdigit(Peek())) Advance();
    }

    AddToken(t_TokenType::NUMBER, source.substr(start, current - start));
}

void t_Lexer::Identifier()
{
    while (std::isalnum(Peek()) || Peek() == '_') Advance();

    std::string text = source.substr(start, current - start);
    
    t_TokenType type = IdentifierType();
    AddToken(type, text);
}

t_TokenType t_Lexer::IdentifierType()
{
    std::string text = source.substr(start, current - start);
    
    auto it = keywords.find(text);
    return (it != keywords.end()) ? it->second : t_TokenType::IDENTIFIER;
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
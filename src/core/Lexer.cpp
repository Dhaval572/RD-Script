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
            while (Peek() != '\n' && !IsAtEnd())
            {
                Advance();
            }
        }
        else
        {
            AddToken(t_TokenType::SLASH);
        }
        break;
    case ' ':
    case '\t':
    case '\n':
        // Skip whitespace characters
        if (c == '\n') 
        {
            line++;
        }
        break;
    case '\r':
        // Skip carriage returns (removed permanently)
        break;
    case '"':
        String();
        break;
    case '$':
        if (Peek() == '"')
        {
            Advance(); // consume the '"'
            FormatString();
        }
        else
        {
            // Just a regular identifier that starts with $
            Identifier();
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

char t_Lexer::Advance()
{
    return source[current++];
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
    if (IsAtEnd()) return false;
    return source[current] == expected ? (++current, true) : false;
}

char t_Lexer::Peek()
{
    if (IsAtEnd()) return '\0';
    return source[current];
}

char t_Lexer::PeekNext()
{
    return 
    (
        current + 1 >= static_cast<int>(source.length())
    ) ? '\0' : source[current + 1];

}

void t_Lexer::String()
{
    std::string value = "";
    
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n') 
        {
            line++;
        }
        
        if (Peek() == '\\')
        {
            // Handle escape sequences
            Advance(); // consume the backslash
            char escaped = Advance(); // consume the escaped character
            
            switch (escaped)
            {
            case 'n':
                value += '\n';
                break;
            case 't':
                value += '\t';
                break;
            case 'r':
                value += '\r';
                break;
            case '"':
                value += '"';
                break;
            case '\\':
                value += '\\';
                break;
            default:
                // For unrecognized escape sequences, just add the character as-is
                value += escaped;
                break;
            }
        }
        else
        {
            value += Advance();
        }
    }

    if (IsAtEnd())
    {
        throw std::runtime_error("Unterminated string at line " + std::to_string(line));
    }

    // The closing ".
    Advance();

    AddToken(t_TokenType::STRING, value);
}

void t_Lexer::FormatString()
{
    std::string value = "$"; // Include the '$' prefix
    
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n') 
        {
            line++;
        }
        
        if (Peek() == '\\')
        {
            // Handle escape sequences
            Advance(); // consume the backslash
            char escaped = Advance(); // consume the escaped character
            
            switch (escaped)
            {
            case 'n':
                value += '\n';
                break;
            case 't':
                value += '\t';
                break;
            case 'r':
                value += '\r';
                break;
            case '"':
                value += '"';
                break;
            case '\\':
                value += '\\';
                break;
            default:
                // For unrecognized escape sequences, just add the character as-is
                value += escaped;
                break;
            }
        }
        else
        {
            value += Advance();
        }
    }

    if (IsAtEnd())
    {
        // Error: Unterminated string.
        throw std::runtime_error("Unterminated format string at line " + std::to_string(line));
    }
    Advance();

    AddToken(t_TokenType::FORMAT_STRING, value);
}

void t_Lexer::Number()
{
    while (std::isdigit(Peek()))
    {
        Advance();
    }

    // Look for a fractional part.
    if (Peek() == '.' && std::isdigit(PeekNext()))
    {
        do 
        {
            Advance();
        }while(std::isdigit(Peek()));
    }

    AddToken(t_TokenType::NUMBER, source.substr(start, current - start));
}

void t_Lexer::Identifier()
{
    while (std::isalnum(Peek()) || Peek() == '_')
    {
        Advance();
    }

    std::string text = source.substr(start, current - start);
    t_TokenType type = t_TokenType::IDENTIFIER;

    // Checks that is keyword or not
    auto it = keywords.find(text);
    if (it != keywords.end())
    {
        type = it->second;
    }

    AddToken(type);
}
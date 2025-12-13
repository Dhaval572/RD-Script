#include "Lexer.h"
#include <unordered_map>
#include <cctype>
#include "ErrorHandling.h"

static const std::unordered_map<std::string, e_TOKEN_TYPE> keywords =
{
    {"and", e_TOKEN_TYPE::AND},
    {"break", e_TOKEN_TYPE::BREAK},
    {"class", e_TOKEN_TYPE::CLASS},
    {"continue", e_TOKEN_TYPE::CONTINUE},
    {"else", e_TOKEN_TYPE::ELSE},
    {"false", e_TOKEN_TYPE::FALSE},
    {"for", e_TOKEN_TYPE::FOR},
    {"fun", e_TOKEN_TYPE::FUN},
    {"if", e_TOKEN_TYPE::IF},
    {"nil", e_TOKEN_TYPE::NIL},
    {"or", e_TOKEN_TYPE::OR},
    {"display", e_TOKEN_TYPE::DISPLAY},
    {"return", e_TOKEN_TYPE::RETURN},
    {"super", e_TOKEN_TYPE::SUPER},
    {"this", e_TOKEN_TYPE::THIS},
    {"true", e_TOKEN_TYPE::TRUE},
    {"while", e_TOKEN_TYPE::WHILE},
    {"auto", e_TOKEN_TYPE::AUTO},
    {"benchmark", e_TOKEN_TYPE::BENCHMARK},
    {"getin", e_TOKEN_TYPE::GETIN} 
};

t_Lexer::t_Lexer(const std::string &source)
    : source(source), start(0), current(0), line(1) {}

t_ParsingResult t_Lexer::ScanTokens()
{
    while (!IsAtEnd())
    {
        start = current;
        t_ParsingResult result = ScanToken();
        if (!result.HasValue())
        {
            return result; 
        }
    }

    tokens.emplace_back(e_TOKEN_TYPE::EOF_TOKEN, "", "", line);
    return t_ParsingResult(tokens);
}

bool t_Lexer::IsAtEnd()
{
    return current >= static_cast<int>(source.length());
}

t_ParsingResult t_Lexer::ScanToken()
{
    char c = Advance();
    switch (c)
    {
    case '(':
        AddToken(e_TOKEN_TYPE::LEFT_PAREN);
        break;
    case ')':
        AddToken(e_TOKEN_TYPE::RIGHT_PAREN);
        break;
    case '{':
        AddToken(e_TOKEN_TYPE::LEFT_BRACE);
        break;
    case '}':
        AddToken(e_TOKEN_TYPE::RIGHT_BRACE);
        break;
    case ',':
        AddToken(e_TOKEN_TYPE::COMMA);
        break;
    case '.':
        AddToken(e_TOKEN_TYPE::DOT);
        break;
    case '-':
        if (Match('-')) 
        {
            AddToken(e_TOKEN_TYPE::MINUS_MINUS);
        } 
        else if (Match('='))
        {
            AddToken(e_TOKEN_TYPE::MINUS_EQUAL);
        }
        else 
        {
            AddToken(e_TOKEN_TYPE::MINUS);
        }
        break;
    case '+':
        if (Match('+')) 
        {
            AddToken(e_TOKEN_TYPE::PLUS_PLUS);
        } 
        else if (Match('='))
        {
            AddToken(e_TOKEN_TYPE::PLUS_EQUAL);
        }
        else 
        {
            AddToken(e_TOKEN_TYPE::PLUS);
        }
        break;
    case ';':
        AddToken(e_TOKEN_TYPE::SEMICOLON);
        break;
    case '*':
        AddToken(Match('=') ? e_TOKEN_TYPE::STAR_EQUAL : e_TOKEN_TYPE::STAR);
        break;
    case '!':
        AddToken(Match('=') ? e_TOKEN_TYPE::BANG_EQUAL : e_TOKEN_TYPE::BANG);
        break;
    case '=':
        AddToken(Match('=') ? e_TOKEN_TYPE::EQUAL_EQUAL : e_TOKEN_TYPE::EQUAL);
        break;
    case '&':
        if (Match('&')) 
        {
            AddToken(e_TOKEN_TYPE::AND);
        } 
        else 
        {
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    e_ERROR_TYPE::LEXING_ERROR, 
                    "Unexpected character", 
                    line, 
                    current
                )
            );
        }
        break;
    case '|':
        if (Match('|'))
        {
            AddToken(e_TOKEN_TYPE::OR);
        }
        else
        {
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    e_ERROR_TYPE::LEXING_ERROR,
                    "Unexpected character",
                    line,
                    current
                )
            );
        }
        break;
    case '<':
        AddToken(Match('=') ? e_TOKEN_TYPE::LESS_EQUAL : e_TOKEN_TYPE::LESS);
        break;
    case '>':
        AddToken(Match('=') ? e_TOKEN_TYPE::GREATER_EQUAL : e_TOKEN_TYPE::GREATER);
        break;
    case '/':
        if (Match('/')) 
        {
            // A comment goes until the end of the line.
            while (Peek() != '\n' && !IsAtEnd()) Advance();
        } 
        else
        {
            AddToken(Match('=') ? e_TOKEN_TYPE::SLASH_EQUAL : e_TOKEN_TYPE::SLASH);
        }
        break;

    case ' ':
    case '\r':
    case '\t':
        break;
    case '\n':
        line++;
        break;
    case '"':
        {
            t_Expected<std::string, t_ErrorInfo> result = String();
            if (!result.HasValue())
            {
                return t_ParsingResult(result.Error());
            }
            AddToken(e_TOKEN_TYPE::STRING, result.Value());
        }
        break;
    case '$':
        if (Peek() == '"') 
        {
            Advance(); 
            t_Expected<std::string, t_ErrorInfo> result = FormatString();
            if (!result.HasValue())
            {
                return t_ParsingResult(result.Error());
            }
            AddToken(e_TOKEN_TYPE::FORMAT_STRING, result.Value());
        }
        else
        {
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    e_ERROR_TYPE::LEXING_ERROR, 
                    "Unexpected character", 
                    line, 
                    current
                )
            );
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
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    e_ERROR_TYPE::LEXING_ERROR, 
                    "Unexpected character", 
                    line, 
                    current
                )
            );
        }
        break;
    }
    
    return t_ParsingResult(tokens);
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
    return 
    (
        current + 1 < static_cast<int>(source.length())
    ) ? source[current + 1] : '\0';
}

t_Expected<std::string, t_ErrorInfo> t_Lexer::String()
{
    std::string value;
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n') line++;
        
        // Handle escape sequences
        if (Peek() == '\\') 
        {
            Advance(); // Consume the backslash
            char escaped = Advance(); // Get the character after backslash
            
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
                case '\\':
                    value += '\\';
                    break;
                case '"':
                    value += '"';
                    break;
                default:
                    // For unrecognized escape sequences, keep both characters
                    value += '\\';
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
        return t_Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ERROR_TYPE::LEXING_ERROR, 
                "Unterminated string", 
                line, 
                current
            )
        );
    }

    // The closing ".
    Advance();

    // Return the processed string value (without surrounding quotes)
    return t_Expected<std::string, t_ErrorInfo>(value);
}

t_Expected<std::string, t_ErrorInfo> t_Lexer::FormatString()
{
    std::string value;
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n') line++;
        
        // Handle escape sequences
        if (Peek() == '\\') 
        {
            Advance(); // Consume the backslash
            char escaped = Advance(); // Get the character after backslash
            
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
                case '\\':
                    value += '\\';
                    break;
                case '"':
                    value += '"';
                    break;
                default:
                    // For unrecognized escape sequences, keep both characters
                    value += '\\';
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
        return t_Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ERROR_TYPE::LEXING_ERROR, 
                "Unterminated format string", 
                line,
                current
            )
        );
    }

    Advance();

    // Return the processed string value (without surrounding quotes)
    return t_Expected<std::string, t_ErrorInfo>(value);
}

void t_Lexer::Number()
{
    while (std::isdigit(Peek())) Advance();

    // Look for a fractional part.
    if (Peek() == '.' && std::isdigit(PeekNext()))
    {
        Advance();
        while (std::isdigit(Peek())) Advance();
    }

    AddToken(e_TOKEN_TYPE::NUMBER, source.substr(start, current - start));
}

void t_Lexer::Identifier()
{
    while (std::isalnum(Peek()) || Peek() == '_') Advance();

    std::string text = source.substr(start, current - start);
    e_TOKEN_TYPE type = IdentifierType();
    AddToken(type, text);
}

e_TOKEN_TYPE t_Lexer::IdentifierType()
{
    std::string text = source.substr(start, current - start);
    
    auto it = keywords.find(text);
    return (it != keywords.end()) ? it->second : e_TOKEN_TYPE::IDENTIFIER;
}

char t_Lexer::Advance()
{
    return source[current++];
}

void t_Lexer::AddToken(e_TOKEN_TYPE type)
{
    AddToken(type, "");
}

void t_Lexer::AddToken(e_TOKEN_TYPE type, const std::string &literal)
{
    std::string text = source.substr(start, current - start);
    tokens.emplace_back(type, text, literal, line);
}
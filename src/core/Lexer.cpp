#include "Lexer.h"
#include <unordered_map>
#include <cctype>
#include "ErrorHandling.h"

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
    {"auto", t_TokenType::AUTO},
    {"benchmark", t_TokenType::BENCHMARK}  // Add benchmark keyword
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
            return result; // Propagate error
        }
    }

    tokens.emplace_back(t_TokenType::EOF_TOKEN, "", "", line);
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
        else if (Match('='))
        {
            AddToken(t_TokenType::MINUS_EQUAL);
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
        else if (Match('='))
        {
            AddToken(t_TokenType::PLUS_EQUAL);
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
        AddToken(Match('=') ? t_TokenType::STAR_EQUAL : t_TokenType::STAR);
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
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    t_ErrorType::LEXING_ERROR, 
                    "Unexpected character", 
                    line, 
                    current
                )
            );
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
            AddToken(Match('=') ? t_TokenType::SLASH_EQUAL : t_TokenType::SLASH);
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
            AddToken(t_TokenType::STRING, result.Value());
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
            AddToken(t_TokenType::FORMAT_STRING, result.Value());
        }
        else
        {
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    t_ErrorType::LEXING_ERROR, 
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
                    t_ErrorType::LEXING_ERROR, 
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
                t_ErrorType::LEXING_ERROR, 
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
                t_ErrorType::LEXING_ERROR, 
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
#include <rubberduck/Lexer.h>
#include <unordered_map>
#include <cctype>
#include <rubberduck/ErrorHandling.h>

static const std::unordered_map<std::string, e_TokenType> keywords =
{
    {"and", e_TokenType::AND},
    {"break", e_TokenType::BREAK},
    {"class", e_TokenType::CLASS},
    {"continue", e_TokenType::CONTINUE},
    {"else", e_TokenType::ELSE},
    {"false", e_TokenType::FALSE},
    {"for", e_TokenType::FOR},
    {"fun", e_TokenType::FUN},
    {"if", e_TokenType::IF},
    {"nil", e_TokenType::NIL},
    {"or", e_TokenType::OR},
    {"display", e_TokenType::DISPLAY},
    {"return", e_TokenType::RETURN},
    {"super", e_TokenType::SUPER},
    {"this", e_TokenType::THIS},
    {"true", e_TokenType::TRUE},
    {"while", e_TokenType::WHILE},
    {"auto", e_TokenType::AUTO},
    {"benchmark", e_TokenType::BENCHMARK},
    {"getin", e_TokenType::GETIN} 
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

    tokens.emplace_back(e_TokenType::EOF_TOKEN, "", "", line);
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
        AddToken(e_TokenType::LEFT_PAREN);
        break;
    case ')':
        AddToken(e_TokenType::RIGHT_PAREN);
        break;
    case '{':
        AddToken(e_TokenType::LEFT_BRACE);
        break;
    case '}':
        AddToken(e_TokenType::RIGHT_BRACE);
        break;
    case ',':
        AddToken(e_TokenType::COMMA);
        break;
    case '.':
        AddToken(e_TokenType::DOT);
        break;
    case '-':
        if (Match('-')) 
        {
            AddToken(e_TokenType::MINUS_MINUS);
        } 
        else if (Match('='))
        {
            AddToken(e_TokenType::MINUS_EQUAL);
        }
        else 
        {
            AddToken(e_TokenType::MINUS);
        }
        break;
    case '+':
        if (Match('+')) 
        {
            AddToken(e_TokenType::PLUS_PLUS);
        } 
        else if (Match('='))
        {
            AddToken(e_TokenType::PLUS_EQUAL);
        }
        else 
        {
            AddToken(e_TokenType::PLUS);
        }
        break;
    case ';':
        AddToken(e_TokenType::SEMICOLON);
        break;
    case '*':
        AddToken(Match('=') ? e_TokenType::STAR_EQUAL : e_TokenType::STAR);
        break;
    case '%':
        AddToken(Match('=') ? e_TokenType::MODULUS_EQUAL : e_TokenType::MODULUS);
        break;
    case '!':
        AddToken(Match('=') ? e_TokenType::BANG_EQUAL : e_TokenType::BANG);
        break;
    case '=':
        AddToken(Match('=') ? e_TokenType::EQUAL_EQUAL : e_TokenType::EQUAL);
        break;
    case '&':
        if (Match('&')) 
        {
            AddToken(e_TokenType::AND);
        } 
        else 
        {
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    e_ErrorType::LEXING_ERROR, 
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
            AddToken(e_TokenType::OR);
        }
        else
        {
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    e_ErrorType::LEXING_ERROR,
                    "Unexpected character",
                    line,
                    current
                )
            );
        }
        break;
    case '<':
        AddToken(Match('=') ? e_TokenType::LESS_EQUAL : e_TokenType::LESS);
        break;
    case '>':
        AddToken(Match('=') ? e_TokenType::GREATER_EQUAL : e_TokenType::GREATER);
        break;
    case '/':
        if (Match('/')) 
        {
            // A comment goes until the end of the line.
            while (Peek() != '\n' && !IsAtEnd()) Advance();
        } 
        else
        {
            AddToken(Match('=') ? e_TokenType::SLASH_EQUAL : e_TokenType::SLASH);
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
            AddToken(e_TokenType::STRING, result.Value());
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
            AddToken(e_TokenType::FORMAT_STRING, result.Value());
        }
        else
        {
            return t_ParsingResult
            (
                t_ErrorInfo
                (
                    e_ErrorType::LEXING_ERROR, 
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
                    e_ErrorType::LEXING_ERROR, 
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
                e_ErrorType::LEXING_ERROR, 
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
                e_ErrorType::LEXING_ERROR, 
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

    AddToken(e_TokenType::NUMBER, source.substr(start, current - start));
}

void t_Lexer::Identifier()
{
    while (std::isalnum(Peek()) || Peek() == '_') Advance();

    std::string text = source.substr(start, current - start);
    e_TokenType type = IdentifierType();
    AddToken(type, text);
}

e_TokenType t_Lexer::IdentifierType()
{
    std::string text = source.substr(start, current - start);
    
    auto it = keywords.find(text);
    return (it != keywords.end()) ? it->second : e_TokenType::IDENTIFIER;
}

char t_Lexer::Advance()
{
    return source[current++];
}

void t_Lexer::AddToken(e_TokenType type)
{
    AddToken(type, "");
}

void t_Lexer::AddToken(e_TokenType type, const std::string &literal)
{
    std::string text = source.substr(start, current - start);
    tokens.emplace_back(type, text, literal, line);
}
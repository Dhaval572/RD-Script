#include <rubberduck/Lexer.h>
#include <rubberduck/ErrorHandling.h>
#include <unordered_map>
#include <cctype>

static const std::unordered_map<std::string, e_TokenType> keywords =
{
    {"and", e_TokenType::AND},
    {"break", e_TokenType::BREAK},
    {"class", e_TokenType::CLASS},
    {"const", e_TokenType::CONST},
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
    {"getin", e_TokenType::GETIN},
    {"typeof", e_TokenType::TYPEOF},
    {"sizeof", e_TokenType::SIZEOF} 
};

Lexer::Lexer(const std::string &source)
    : m_Source(source), m_Start(0), m_Current(0), m_Line(1) {}

ParsingResult Lexer::ScanTokens()
{
    while (!IsAtEnd())
    {
        m_Start = m_Current;
        ParsingResult result = ScanToken();
        if (!result.HasValue())
        {
            return result; 
        }
    }

    m_Tokens.emplace_back(e_TokenType::EOF_TOKEN, "", "", m_Line);
    return ParsingResult(m_Tokens);
}

bool Lexer::IsAtEnd()
{
    return m_Current >= static_cast<int>(m_Source.length());
}

ParsingResult Lexer::ScanToken()
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
        AddToken
        (
            Match('=') ? 
            e_TokenType::STAR_EQUAL : 
            e_TokenType::STAR
        );
        break;
    case '%':
        AddToken
        (
            Match('=') ? 
            e_TokenType::MODULUS_EQUAL : 
            e_TokenType::MODULUS
        );
        break;
    case '!':
        AddToken
        (
            Match('=') ? 
            e_TokenType::BANG_EQUAL : 
            e_TokenType::BANG
        );
        break;
    case '=':
        AddToken
        (
            Match('=') ? 
            e_TokenType::EQUAL_EQUAL : 
            e_TokenType::EQUAL
        );
        break;
    case '&':
        if (Match('&')) 
        {
            AddToken(e_TokenType::AND);
        } 
        else 
        {
            return ParsingResult
            (
                t_ErrorInfo
                (
                    e_ErrorType::LEXING_ERROR, 
                    "Unexpected character", 
                    m_Line, 
                    m_Current
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
            return ParsingResult
            (
                t_ErrorInfo
                (
                    e_ErrorType::LEXING_ERROR,
                    "Unexpected character",
                    m_Line,
                    m_Current
                )
            );
        }
        break;
    case '<':
        AddToken
        (
            Match('=') ? 
            e_TokenType::LESS_EQUAL : 
            e_TokenType::LESS
        );
        break;
    case '>':
        AddToken
        (
            Match('=') ? 
            e_TokenType::GREATER_EQUAL : 
            e_TokenType::GREATER
        );
        break;
    case '/':
        if (Match('/')) 
        {
            // A comment goes until the end of the line.
            while (Peek() != '\n' && !IsAtEnd()) Advance();
        } 
        else
        {
            AddToken
            (
                Match('=') ? 
                e_TokenType::SLASH_EQUAL : 
                e_TokenType::SLASH
            );
        }
        break;

    case ' ':
    case '\r':
    case '\t':
        break;
    case '\n':
        m_Line++;
        break;
    case '"':
        {
            Expected<std::string, t_ErrorInfo> result = String();
            if (!result.HasValue())
            {
                return ParsingResult(result.Error());
            }
            AddToken(e_TokenType::STRING, result.Value());
        }
        break;
    case '$':
        if (Peek() == '"') 
        {
            Advance(); 
            Expected<std::string, t_ErrorInfo> result = FormatString();
            if (!result.HasValue())
            {
                return ParsingResult(result.Error());
            }
            AddToken(e_TokenType::FORMAT_STRING, result.Value());
        }
        else
        {
            return ParsingResult
            (
                t_ErrorInfo
                (
                    e_ErrorType::LEXING_ERROR, 
                    "Unexpected character", 
                    m_Line, 
                    m_Current
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
            return ParsingResult
            (
                t_ErrorInfo
                (
                    e_ErrorType::LEXING_ERROR, 
                    "Unexpected character", 
                    m_Line, 
                    m_Current
                )
            );
        }
        break;
    }
    
    return ParsingResult(m_Tokens);
}

bool Lexer::Match(char expected)
{
    if (IsAtEnd()) return false;
    if (m_Source[m_Current] != expected) return false;

    m_Current++;
    return true;
}

char Lexer::Peek()
{
    if (IsAtEnd()) return '\0';
    return m_Source[m_Current];
}

char Lexer::PeekNext()
{
    return 
    (
        m_Current + 1 < static_cast<int>(m_Source.length())
    ) ? m_Source[m_Current + 1] : '\0';
}

Expected<std::string, t_ErrorInfo> Lexer::String()
{
    std::string value;
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n') m_Line++;
        
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
        return Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::LEXING_ERROR, 
                "Unterminated string", 
                m_Line, 
                m_Current
            )
        );
    }
    Advance();

    // Return the processed string value (without surrounding quotes)
    return Expected<std::string, t_ErrorInfo>(value);
}

Expected<std::string, t_ErrorInfo> Lexer::FormatString()
{
    std::string value;
    while (Peek() != '"' && !IsAtEnd())
    {
        if (Peek() == '\n') m_Line++;
        
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
        return Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::LEXING_ERROR, 
                "Unterminated format string", 
                m_Line,
                m_Current
            )
        );
    }

    Advance();

    // Return the processed string value (without surrounding quotes)
    return Expected<std::string, t_ErrorInfo>(value);
}

void Lexer::Number()
{
    while (std::isdigit(Peek())) Advance();

    // Look for a fractional part.
    if (Peek() == '.' && std::isdigit(PeekNext()))
    {
        Advance();
        while (std::isdigit(Peek())) Advance();
    }

    AddToken
    (
        e_TokenType::NUMBER, 
        m_Source.substr(m_Start, m_Current - m_Start)
    );
}

void Lexer::Identifier()
{
    while (std::isalnum(Peek()) || Peek() == '_') Advance();

    std::string text = m_Source.substr(m_Start, m_Current - m_Start);
    e_TokenType type = IdentifierType();
    AddToken(type, text);
}

e_TokenType Lexer::IdentifierType()
{
    std::string text = m_Source.substr(m_Start, m_Current - m_Start);
    
    auto it = keywords.find(text);
    return (it != keywords.end()) ? it->second : e_TokenType::IDENTIFIER;
}

char Lexer::Advance()
{
    return m_Source[m_Current++];
}

void Lexer::AddToken(e_TokenType type)
{
    AddToken(type, "");
}

void Lexer::AddToken(e_TokenType type, const std::string &literal)
{
    std::string text = m_Source.substr(m_Start, m_Current - m_Start);
    m_Tokens.emplace_back(type, text, literal, m_Line);
}
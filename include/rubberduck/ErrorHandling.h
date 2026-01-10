#pragma once

#include <string>
#include <variant>
#include <vector>
#include <rubberduck/Token.h>

// Error types for RD Script
enum class e_ErrorType
{
    LEXING_ERROR,
    PARSING_ERROR,
    RUNTIME_ERROR,
    TYPE_ERROR
};

// Error information structure
struct t_ErrorInfo
{
    e_ErrorType type;
    std::string message;
    int line;
    int column;

    t_ErrorInfo() 
        : type(e_ErrorType::RUNTIME_ERROR), 
          message(""), 
          line(0), 
          column(0) {}
    
    t_ErrorInfo
    (
        e_ErrorType type, 
        const std::string& message, 
        int line = 0, 
        int column = 0
    )
        : type(type), 
          message(message), 
          line(line), 
          column(column) {}
};

// Expected type for error handling - similar to C++23's std::expected
template<typename T, typename E>
class Expected
{
private:
    std::variant<T, E> m_Value;

public:
    // Constructors
    Expected(const T& value) : m_Value(value) {}
    Expected(T&& value) : m_Value(std::move(value)) {}
    Expected(const E& error) : m_Value(error) {}
    Expected(E&& error) : m_Value(std::move(error)) {}

    explicit operator bool() const 
    { 
        return std::holds_alternative<T>(m_Value); 
    }
    
    // Access the value (undefined behavior if operator bool() returns false)
    const T& Value() const { return std::get<T>(m_Value); }
    T& Value() { return std::get<T>(m_Value); }
    
    // Access the error (undefined behavior if operator bool() returns true)
    const E& Error() const { return std::get<E>(m_Value); }
    E& Error() { return std::get<E>(m_Value); }
    
    // Value or default
    T ValueOr(const T& default_value) const
    {
        return static_cast<bool>(*this) ? Value() : default_value;
    }
};

// Convenience type aliases
using ParsingResult = Expected<std::vector<t_Token>, t_ErrorInfo>;
using InterpretationResult = Expected<int, t_ErrorInfo>; 

// Error reporting function
void ReportError(const t_ErrorInfo& error);
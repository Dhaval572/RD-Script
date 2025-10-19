#pragma once

#include <string>
#include <variant>
#include <vector>
#include "Token.h"

// Error types for RD Script
enum class t_ErrorType
{
    LEXING_ERROR,
    PARSING_ERROR,
    RUNTIME_ERROR,
    TYPE_ERROR
};

// Error information structure
struct t_ErrorInfo
{
    t_ErrorType type;
    std::string message;
    int line;
    int column;

    t_ErrorInfo() 
        : type(t_ErrorType::RUNTIME_ERROR), 
          message(""), 
          line(0), 
          column(0) {}
    
    t_ErrorInfo
    (
        t_ErrorType type, 
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
class t_Expected
{
private:
    std::variant<T, E> m_Value;

public:
    // Constructors
    t_Expected(const T& value) : m_Value(value) {}
    t_Expected(T&& value) : m_Value(std::move(value)) {}
    t_Expected(const E& error) : m_Value(error) {}
    t_Expected(E&& error) : m_Value(std::move(error)) {}

    // Check if the result is a value or an error
    bool HasValue() const { return std::holds_alternative<T>(m_Value); }
    
    // Access the value (undefined behavior if HasValue() is false)
    const T& Value() const { return std::get<T>(m_Value); }
    T& Value() { return std::get<T>(m_Value); }
    
    // Access the error (undefined behavior if HasValue() is true)
    const E& Error() const { return std::get<E>(m_Value); }
    E& Error() { return std::get<E>(m_Value); }
    
    // Value or default
    T ValueOr(const T& default_value) const
    {
        return HasValue() ? Value() : default_value;
    }
};

// Convenience type aliases
using t_ParsingResult = t_Expected<std::vector<t_Token>, t_ErrorInfo>;
using t_InterpretationResult = t_Expected<int, t_ErrorInfo>; // Use int instead of void

// Error reporting function
void ReportError(const t_ErrorInfo& error);
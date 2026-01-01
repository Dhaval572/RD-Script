#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include <rubberduck/AST.h>
#include <rubberduck/ErrorHandling.h>

// Type enumeration for RD Script values
enum class e_ValueType
{
    NIL,
    NUMBER,
    STRING,
    BOOLEAN
};

// Struct to hold typed values with direct numeric storage
struct t_TypedValue
{
    std::string value;
    e_ValueType type;
    double numeric_value;  
    bool has_numeric_value;

    t_TypedValue() 
        : value("nil"), 
          type(e_ValueType::NIL), 
          numeric_value(0.0), 
          has_numeric_value(false) {}
    
    t_TypedValue(const std::string& val, e_ValueType typ) 
        : value(val), 
          type(typ), 
          numeric_value(0.0), 
          has_numeric_value(false) 
    {
        // Pre-compute numeric value if possible
        if (typ == e_ValueType::NUMBER) 
        {
            try 
            {
                numeric_value = std::stod(val);
                has_numeric_value = true;
            } 
            catch (...) 
            {
                has_numeric_value = false;
            }
        }
    }
    
    // Constructor for direct numeric values
    t_TypedValue(double num_val) 
        : type(e_ValueType::NUMBER), 
          numeric_value(num_val), 
          has_numeric_value(true) 
    {
        // Format the string representation properly
        value = std::to_string(num_val);

        // Remove trailing zeros and decimal point if not needed
        value.erase(value.find_last_not_of('0') + 1, std::string::npos);
        value.erase(value.find_last_not_of('.') + 1, std::string::npos);
    }
    
    // Copy constructor
    t_TypedValue(const t_TypedValue& other) 
        : value(other.value), 
          type(other.type), 
          numeric_value(other.numeric_value), 
          has_numeric_value(other.has_numeric_value) {}
    
    // Assignment operator
    t_TypedValue& operator=(const t_TypedValue& other) 
    {
        if (this != &other)
        {
            value = other.value;
            type = other.type;
            numeric_value = other.numeric_value;
            has_numeric_value = other.has_numeric_value;
        }
        return *this;
    }
};

class Interpreter
{
private:
    std::unordered_map<std::string, t_TypedValue> m_Environment;
    std::vector<std::unordered_map<std::string, t_TypedValue>> m_ScopeStack; 
    int m_LoopDepth = 0; 
    std::string m_ControlSignal; // "break" | "continue" | ""
    std::unordered_map<std::string, t_FunStmt*> m_Functions;

    std::string m_ReturnValue; 
    bool m_IsReturning = false;

    bool m_BufferOutput = false;
    std::string m_OutputBuffer;

    Expected<std::string, t_ErrorInfo> Evaluate(t_Expr *expr);
    Expected<int, t_ErrorInfo> Execute(t_Stmt *stmt);
    bool IsTruthy(const std::string &value);
    std::string Stringify(const std::string &value);
    Expected<std::string, t_ErrorInfo> EvaluateFormatExpression
    (
        const std::string &expr_str
    );
    
    e_ValueType DetectType(const std::string& value);

    // Helper functions for number formatting and type checking
    static std::string FormatNumber(double value);
    bool IsInteger(const std::string& value);
    bool IsFloat(const std::string& value);
    
    // Optimized loop execution methods
    bool IsSimpleNumericLoop(t_ForStmt* for_stmt);
    Expected<int, t_ErrorInfo> ExecuteSimpleNumericLoop
    (
        t_ForStmt* for_stmt
    ); 
    
    // Ultra-fast native loop optimization for simple accumulation patterns
    bool IsSimpleAccumulationLoop(t_ForStmt* for_stmt);
    Expected<int, t_ErrorInfo> ExecuteAccumulationLoop
    (
        t_ForStmt* for_stmt
    );
    
    // Ultra-fast native optimization for nested loops with arithmetic expressions
    bool IsNestedArithmeticLoop(t_ForStmt* for_stmt);
    Expected<int, t_ErrorInfo> ExecuteNestedArithmeticLoop
    (
        t_ForStmt* for_stmt
    ); 

    // Optimized arithmetic operations
    Expected<bool, t_ErrorInfo> PerformComparison
    (
        const t_TypedValue& left, 
        const e_TokenType op, 
        const t_TypedValue& right
    );
    
    // Scope management
    void PushScope();
    void PopScope();

    void WriteOutput(const std::string& text);
    void FlushOutput();
    Expected<int, t_ErrorInfo> DeclareVariable
    (
        const std::string& name, 
        int line
    ); 

public:
    Interpreter();
    InterpretationResult Interpret
    (
        const std::vector<PoolPtr<t_Stmt>> &statements
    );
};
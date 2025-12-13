#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <chrono>
#include "AST.h"
#include "ErrorHandling.h"

// Type enumeration for RD Script values
enum class e_VALUE_TYPE
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
    e_VALUE_TYPE type;
    double numeric_value;  
    bool has_numeric_value;

    t_TypedValue() 
        : value("nil"), 
          type(e_VALUE_TYPE::NIL), 
          numeric_value(0.0), 
          has_numeric_value(false) {}
    
    t_TypedValue(const std::string& val, e_VALUE_TYPE typ) 
        : value(val), 
          type(typ), 
          numeric_value(0.0), 
          has_numeric_value(false) 
    {
        // Pre-compute numeric value if possible
        if (typ == e_VALUE_TYPE::NUMBER) 
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
        : type(e_VALUE_TYPE::NUMBER), 
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

class t_Interpreter
{
private:
    std::unordered_map<std::string, t_TypedValue> environment;
    std::vector<std::unordered_map<std::string, t_TypedValue>> scope_stack; 
    int loop_depth = 0; 
    std::string control_signal; // "break" | "continue" | ""
    std::unordered_map<std::string, t_FunStmt*> functions;
    std::string return_value; // To store return values from functions
    bool is_returning = false; // Flag to indicate if we're currently returning

    t_Expected<std::string, t_ErrorInfo> Evaluate(t_Expr *expr);
    t_Expected<int, t_ErrorInfo> Execute(t_Stmt *stmt);
    bool IsTruthy(const std::string &value);
    std::string Stringify(const std::string &value);
    t_Expected<std::string, t_ErrorInfo> EvaluateFormatExpression
    (
        const std::string &expr_str
    );
    
    e_VALUE_TYPE DetectType(const std::string& value);

    // Helper functions for number formatting and type checking
    static std::string FormatNumber(double value);
    bool IsInteger(const std::string& value);
    bool IsFloat(const std::string& value);
    
    // Optimized loop execution methods
    bool IsSimpleNumericLoop(t_ForStmt* for_stmt);
    t_Expected<int, t_ErrorInfo> ExecuteSimpleNumericLoop
    (
        t_ForStmt* for_stmt
    ); 
    
    // Ultra-fast native loop optimization for simple accumulation patterns
    bool IsSimpleAccumulationLoop(t_ForStmt* for_stmt);
    t_Expected<int, t_ErrorInfo> ExecuteAccumulationLoop
    (
        t_ForStmt* for_stmt
    ); 
    
    // General optimized int-only for-loop execution
    bool IsIntForLoop(t_ForStmt* for_stmt);
    t_Expected<int, t_ErrorInfo> ExecuteIntForLoop
    (
        t_ForStmt* for_stmt
    );

    // Optimized arithmetic operations
    t_Expected<bool, t_ErrorInfo> PerformComparison
    (
        const t_TypedValue& left, const e_TOKEN_TYPE op, const t_TypedValue& right
    );
    
    // Scope management
    void PushScope();
    void PopScope();
    t_Expected<int, t_ErrorInfo> DeclareVariable
    (
        const std::string& name, int line
    ); 

public:
    t_Interpreter();
    t_InterpretationResult Interpret(const std::vector<t_Stmt *> &statements);
};
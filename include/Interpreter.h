#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <chrono>
#include "AST.h"

// Type enumeration for RD Script values
enum class t_ValueType
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
    t_ValueType type;
    double numeric_value;  // Direct numeric storage for faster operations
    bool has_numeric_value;

    t_TypedValue() : value("nil"), type(t_ValueType::NIL), numeric_value(0.0), has_numeric_value(false) {}
    
    t_TypedValue(const std::string& val, t_ValueType typ) : value(val), type(typ), numeric_value(0.0), has_numeric_value(false) {
        // Pre-compute numeric value if possible
        if (typ == t_ValueType::NUMBER) {
            try {
                numeric_value = std::stod(val);
                has_numeric_value = true;
            } catch (...) {
                has_numeric_value = false;
            }
        }
    }
    
    // Constructor for direct numeric values
    t_TypedValue(double num_val) : type(t_ValueType::NUMBER), numeric_value(num_val), has_numeric_value(true) {
        // Format the string representation properly
        value = std::to_string(num_val);
        // Remove trailing zeros and decimal point if not needed
        value.erase(value.find_last_not_of('0') + 1, std::string::npos);
        value.erase(value.find_last_not_of('.') + 1, std::string::npos);
    }
    
    // Copy constructor
    t_TypedValue(const t_TypedValue& other) : 
        value(other.value), 
        type(other.type), 
        numeric_value(other.numeric_value), 
        has_numeric_value(other.has_numeric_value) {}
    
    // Assignment operator
    t_TypedValue& operator=(const t_TypedValue& other) {
        if (this != &other) {
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

    std::string Evaluate(t_Expr *expr);
    void Execute(t_Stmt *stmt);
    bool IsTruthy(const std::string &value);
    std::string Stringify(const std::string &value);
    std::string EvaluateFormatExpression(const std::string &expr_str);
    
    // Helper function to detect the type of a value
    t_ValueType DetectType(const std::string& value);
    
    // Helper functions for number formatting and type checking
    static std::string FormatNumber(double value);
    bool IsInteger(const std::string& value);
    bool IsFloat(const std::string& value);
    
    // Optimized loop execution methods
    bool IsSimpleNumericLoop(t_ForStmt* for_stmt);
    void ExecuteSimpleNumericLoop(t_ForStmt* for_stmt);
    
    // Optimized arithmetic operations
    std::string PerformAddition(const t_TypedValue& left, const t_TypedValue& right);
    std::string PerformSubtraction(const t_TypedValue& left, const t_TypedValue& right);
    std::string PerformMultiplication(const t_TypedValue& left, const t_TypedValue& right);
    std::string PerformDivision(const t_TypedValue& left, const t_TypedValue& right);
    bool PerformComparison(const t_TypedValue& left, const t_TokenType op, const t_TypedValue& right);

public:
    t_Interpreter();
    void Interpret(const std::vector<t_Stmt *> &statements);
};
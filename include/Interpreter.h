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

// Struct to hold typed values
struct t_TypedValue
{
    std::string value;
    t_ValueType type;

    t_TypedValue() : value("nil"), type(t_ValueType::NIL) {}
    t_TypedValue(const std::string& val, t_ValueType typ) : value(val), type(typ) {}
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

public:
    t_Interpreter();
    void Interpret(const std::vector<t_Stmt *> &statements);
};
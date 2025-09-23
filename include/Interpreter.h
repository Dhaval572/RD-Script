#pragma once

#include <vector>
#include <unordered_map>
#include "AST.h"

class t_Interpreter
{
private:
    std::unordered_map<std::string, std::string> environment;

    std::string Evaluate(t_Expr *expr);
    void Execute(t_Stmt *stmt);
    std::string Stringify(const std::string &value);
    std::string EvaluateFormatExpression(const std::string &expr_str); 

public:
    t_Interpreter();
    void Interpret(const std::vector<t_Stmt *> &statements);
};
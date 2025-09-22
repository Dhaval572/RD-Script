#include "Interpreter.h"
#include <iostream>

t_Interpreter::t_Interpreter() {}

void t_Interpreter::Interpret(const std::vector<t_Stmt *> &statements)
{
    try
    {
        for (t_Stmt *statement : statements)
        {
            Execute(statement);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Runtime error: " << e.what() << std::endl;
    }
}

void t_Interpreter::Execute(t_Stmt *stmt)
{
    // In a real implementation, we would use the visitor pattern
    // For simplicity, we'll use dynamic casting here

    if (t_VarStmt *var_stmt = dynamic_cast<t_VarStmt *>(stmt))
    {
        std::string value = "nil";
        if (var_stmt->initializer)
        {
            value = Evaluate(var_stmt->initializer.get());
        }
        environment[var_stmt->name] = value;
    }
    else if (t_PrintStmt *display_stmt = dynamic_cast<t_PrintStmt *>(stmt))
    {
        // This is now a Display statement
        std::string value = Evaluate(display_stmt->expression.get());
        std::cout << value << std::endl;
    }
    else if (t_ExpressionStmt *expr_stmt = dynamic_cast<t_ExpressionStmt *>(stmt))
    {
        Evaluate(expr_stmt->expression.get());
    }
    // Handle other statement types...
}

std::string t_Interpreter::Evaluate(t_Expr *expr)
{
    if (t_LiteralExpr *literal = dynamic_cast<t_LiteralExpr *>(expr))
    {
        return literal->value;
    }

    if (t_GroupingExpr *grouping = dynamic_cast<t_GroupingExpr *>(expr))
    {
        return Evaluate(grouping->expression.get());
    }

    if (t_UnaryExpr *unary = dynamic_cast<t_UnaryExpr *>(expr))
    {
        std::string right = Evaluate(unary->right.get());

        switch (unary->op.type)
        {
        case t_TokenType::MINUS:
            // In a real implementation, we would convert to number and negate
            return "-" + right;
        case t_TokenType::BANG:
            // In a real implementation, we would convert to boolean and negate
            return (right == "false" || right == "0") ? "true" : "false";
        }
    }

    if (t_BinaryExpr *binary = dynamic_cast<t_BinaryExpr *>(expr))
    {
        std::string left = Evaluate(binary->left.get());
        std::string right = Evaluate(binary->right.get());

        switch (binary->op.type)
        {
        case t_TokenType::PLUS:
            // In a real implementation, we would handle both numbers and strings
            return left + right;
        case t_TokenType::MINUS:
            // In a real implementation, we would convert to numbers and subtract
            return "result_of_subtraction";
        case t_TokenType::STAR:
            // In a real implementation, we would convert to numbers and multiply
            return "result_of_multiplication";
        case t_TokenType::SLASH:
            // In a real implementation, we would convert to numbers and divide
            return "result_of_division";
        case t_TokenType::BANG_EQUAL:
            return (left != right) ? "true" : "false";
        case t_TokenType::EQUAL_EQUAL:
            return (left == right) ? "true" : "false";
            // Handle comparison operators...
        }
    }

    if (t_VariableExpr *variable = dynamic_cast<t_VariableExpr *>(expr))
    {
        auto it = environment.find(variable->name);
        if (it != environment.end())
        {
            return it->second;
        }
        return "nil"; // Or throw an error for undefined variables
    }

    // In a real implementation, we would handle all cases
    return "";
}

std::string t_Interpreter::Stringify(const std::string &value)
{
    // In a real implementation, we would handle different types
    return value;
}
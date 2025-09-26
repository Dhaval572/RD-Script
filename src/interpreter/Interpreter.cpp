#include "Interpreter.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <stack>
#include "../include/Lexer.h"
#include "../include/Parser.h"

t_Interpreter::t_Interpreter() = default;

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
    if (t_BlockStmt *block_stmt = dynamic_cast<t_BlockStmt *>(stmt))
    {
        for (const auto &statement : block_stmt->statements)
        {
            Execute(statement.get());
        }
    }
    else if (t_IfStmt *if_stmt = dynamic_cast<t_IfStmt *>(stmt))
    {
        std::string condition_value = Evaluate(if_stmt->condition.get());
        if (IsTruthy(condition_value))
        {
            Execute(if_stmt->then_branch.get());
        }
        else if (if_stmt->else_branch)
        {
            Execute(if_stmt->else_branch.get());
        }
    }
    else if (t_VarStmt *var_stmt = dynamic_cast<t_VarStmt *>(stmt))
    {
        // Check if variable already exists
        if (environment.find(var_stmt->name) != environment.end())
        {
            throw std::runtime_error("Variable '" + var_stmt->name + "' already declared");
        }

        std::string value = "nil";
        if (var_stmt->initializer)
        {
            value = Evaluate(var_stmt->initializer.get());
        }
        environment[var_stmt->name] = value;
    }
    else if (t_DisplayStmt *display_stmt = dynamic_cast<t_DisplayStmt *>(stmt))
    {
        bool first = true;
        for (const auto &expr : display_stmt->expressions)
        {
            if (!first)
            {
                std::cout << " "; // Add space between values
            }
            first = false;

            std::string value = Evaluate(expr.get());
            std::cout << value;
        }
        std::cout << std::endl;
    }
    else if (t_ExpressionStmt *expr_stmt = dynamic_cast<t_ExpressionStmt *>(stmt))
    {
        Evaluate(expr_stmt->expression.get());
    }
}

bool t_Interpreter::IsTruthy(const std::string &value)
{
    // In our language:
    // - "false" and "nil" are falsey
    // - Everything else is truthy
    return value != "false" && value != "nil";
}

std::string t_Interpreter::Evaluate(t_Expr *expr)
{
    if (t_LiteralExpr *literal = dynamic_cast<t_LiteralExpr *>(expr))
    {
        // Check if this is a format string (starts with $)
        if (literal->value.length() > 0 && literal->value[0] == '$')
        {
            // Process format string
            std::string format_str = literal->value.substr(1);
            std::string result = format_str;

            // Improved string replacement approach with better error handling
            size_t pos = 0;
            while ((pos = result.find('{')) != std::string::npos)
            {
                size_t end_pos = result.find('}', pos);
                if (end_pos != std::string::npos)
                {
                    std::string expression = result.substr(pos + 1, end_pos - pos - 1);

                    // Trim whitespace from expression
                    expression.erase(0, expression.find_first_not_of(" \t"));
                    expression.erase(expression.find_last_not_of(" \t") + 1);

                    // Evaluate the expression instead of just looking it up as a variable
                    std::string expr_value = EvaluateFormatExpression(expression);

                    result.replace(pos, end_pos - pos + 1, expr_value);
                }
                else
                {
                    // No closing brace found, treat as literal
                    break;
                }
            }

            return result;
        }

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
            return "-" + right;

        case t_TokenType::BANG:
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
            try
            {
                // Try to convert to numbers for arithmetic
                double left_num = std::stod(left);
                double right_num = std::stod(right);
                return std::to_string(left_num + right_num);
            }
            catch (...)
            {
                // If not numbers, prevent string concatenation
                throw std::runtime_error
                (
                    "String concatenation with '+' is not allowed. Use comma-separated values in display statements instead."
                );
            }

        case t_TokenType::MINUS:
            try
            {
                double left_num = std::stod(left);
                double right_num = std::stod(right);
                return std::to_string(left_num - right_num);
            }
            catch (...)
            {
                return "Error: Cannot perform arithmetic operation";
            }

        case t_TokenType::STAR:
            try
            {
                double left_num = std::stod(left);
                double right_num = std::stod(right);
                return std::to_string(left_num * right_num);
            }
            catch (...)
            {
                return "Error: Cannot perform arithmetic operation";
            }
        case t_TokenType::SLASH:
            try
            {
                double left_num = std::stod(left);
                double right_num = std::stod(right);
                if (right_num == 0)
                {
                    return "Error: Division by zero";
                }
                return std::to_string(left_num / right_num);
            }
            catch (...)
            {
                return "Error: Cannot perform arithmetic operation";
            }
        case t_TokenType::BANG_EQUAL:
            return (left != right) ? "true" : "false";

        case t_TokenType::EQUAL_EQUAL:
            return (left == right) ? "true" : "false";

        case t_TokenType::GREATER:
            try
            {
                double left_num = std::stod(left);
                double right_num = std::stod(right);
                return (left_num > right_num) ? "true" : "false";
            }
            catch (...)
            {
                return "Error: Cannot compare non-numeric values";
            }
        case t_TokenType::GREATER_EQUAL:
            try
            {
                double left_num = std::stod(left);
                double right_num = std::stod(right);
                return (left_num >= right_num) ? "true" : "false";
            }
            catch (...)
            {
                return "Error: Cannot compare non-numeric values";
            }
        case t_TokenType::LESS:
            try
            {
                double left_num = std::stod(left);
                double right_num = std::stod(right);
                return (left_num < right_num) ? "true" : "false";
            }
            catch (...)
            {
                return "Error: Cannot compare non-numeric values";
            }
        case t_TokenType::LESS_EQUAL:
            try
            {
                double left_num = std::stod(left);
                double right_num = std::stod(right);
                return (left_num <= right_num) ? "true" : "false";
            }
            catch (...)
            {
                return "Error: Cannot compare non-numeric values";
            }
        }
    }

    if (t_VariableExpr *variable = dynamic_cast<t_VariableExpr *>(expr))
    {
        auto it = environment.find(variable->name);
        if (it != environment.end())
        {
            return it->second;
        }
        return "nil";
    }

    return "";
}

std::string t_Interpreter::Stringify(const std::string &value)
{
    // In a real implementation, we would handle different types
    return value;
}

std::string t_Interpreter::EvaluateFormatExpression(const std::string &expr_str)
{
    try
    {
        // Handle empty expressions
        if (expr_str.empty())
        {
            return "";
        }

        // Create a temporary lexer and parser to evaluate the expression
        t_Lexer lexer(expr_str);
        std::vector<t_Token> tokens = lexer.ScanTokens();

        // Remove the EOF token as it's not needed for expression parsing
        if (!tokens.empty() && tokens.back().type == t_TokenType::EOF_TOKEN)
        {
            tokens.pop_back();
        }

        // If no tokens or only EOF token, return empty string
        if (tokens.empty())
        {
            return "";
        }

        // Create a parser for the expression
        t_Parser parser(tokens);

        // Parse and evaluate the expression
        std::unique_ptr<t_Expr> expr(parser.Expression());
        if (expr)
        {
            return Evaluate(expr.get());
        }
        return "nil";
    }
    catch (...)
    {
        // If parsing fails, treat as literal text
        return expr_str;
    }
}
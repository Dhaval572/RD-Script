#include "Interpreter.h"
#include <iostream>
#include <sstream>
#include <regex>

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
    if (t_VarStmt *var_stmt = dynamic_cast<t_VarStmt *>(stmt))
    {
        std::string value = "nil";
        if (var_stmt->initializer)
        {
            value = Evaluate(var_stmt->initializer.get());
        }
        environment[var_stmt->name] = value;
    }
    else if (t_DisplayStmt *display_stmt = dynamic_cast<t_DisplayStmt *>(stmt))
    {
        std::string value = Evaluate(display_stmt->expression.get());
        if (value.length() > 0 && value[0] == 'k')
        {
            // Process format string
            std::string format_str = value.substr(1); 
            std::string result = format_str;
            std::regex var_pattern(R"(\{([^}]+)\})");
            std::smatch match;
            
            while (std::regex_search(result, match, var_pattern))
            {
                std::string var_name = match[1].str();
                std::string var_value = "nil";
                
                auto it = environment.find(var_name);
                if (it != environment.end())
                {
                    var_value = it->second;
                }
                
                result = std::regex_replace(result, var_pattern, var_value, std::regex_constants::format_first_only);
            }
            
            std::cout << result << std::endl;
        }
        else
        {
            std::cout << value << std::endl;
        }
    }
    else if (t_ExpressionStmt *expr_stmt = dynamic_cast<t_ExpressionStmt *>(stmt))
    {
        Evaluate(expr_stmt->expression.get());
    }
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
                return left + right;
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
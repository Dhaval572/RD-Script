#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Token.h"

// Abstract base classes for expressions and statements
class t_Expr
{
public:
    virtual ~t_Expr() = default;
};

class t_Stmt
{
public:
    virtual ~t_Stmt() = default;
};

// Expression types
struct t_BinaryExpr : public t_Expr
{
    std::unique_ptr<t_Expr> left;
    t_Token op;
    std::unique_ptr<t_Expr> right;

    t_BinaryExpr(std::unique_ptr<t_Expr> left, t_Token op, std::unique_ptr<t_Expr> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}
};

struct t_LiteralExpr : public t_Expr
{
    std::string value;

    t_LiteralExpr(const std::string &value) : value(value) {}
};

struct t_UnaryExpr : public t_Expr
{
    t_Token op;
    std::unique_ptr<t_Expr> right;

    t_UnaryExpr(t_Token op, std::unique_ptr<t_Expr> right)
        : op(op), right(std::move(right)) {}
};

struct t_GroupingExpr : public t_Expr
{
    std::unique_ptr<t_Expr> expression;

    t_GroupingExpr(std::unique_ptr<t_Expr> expression)
        : expression(std::move(expression)) {}
};

struct t_VariableExpr : public t_Expr
{
    std::string name;

    t_VariableExpr(const std::string &name) : name(name) {}
};

// Statement types
struct t_ExpressionStmt : public t_Stmt
{
    std::unique_ptr<t_Expr> expression;

    t_ExpressionStmt(std::unique_ptr<t_Expr> expression)
        : expression(std::move(expression)) {}
};

// Note: Using PrintStmt for Display statements for simplicity
// In a full implementation, we might want a separate DisplayStmt
struct t_PrintStmt : public t_Stmt
{
    std::unique_ptr<t_Expr> expression;

    t_PrintStmt(std::unique_ptr<t_Expr> expression)
        : expression(std::move(expression)) {}
};

struct t_VarStmt : public t_Stmt
{
    std::string name;
    std::unique_ptr<t_Expr> initializer;

    t_VarStmt(const std::string &name, std::unique_ptr<t_Expr> initializer)
        : name(name), initializer(std::move(initializer)) {}
};
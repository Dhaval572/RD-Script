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

    t_BinaryExpr
    (
        std::unique_ptr<t_Expr> left, 
        t_Token op, 
        std::unique_ptr<t_Expr> right
    )
        : left(std::move(left)), 
          op(op), 
          right(std::move(right)) {}
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

// Increment/Decrement expressions
struct t_PrefixExpr : public t_Expr
{
    t_Token op;
    std::unique_ptr<t_Expr> operand;

    t_PrefixExpr(t_Token op, std::unique_ptr<t_Expr> operand)
        : op(op), operand(std::move(operand)) {}
};

struct t_PostfixExpr : public t_Expr
{
    std::unique_ptr<t_Expr> operand;
    t_Token op;

    t_PostfixExpr(std::unique_ptr<t_Expr> operand, t_Token op)
        : operand(std::move(operand)), op(op) {}
};

// Statement types
struct t_ExpressionStmt : public t_Stmt
{
    std::unique_ptr<t_Expr> expression;
    t_ExpressionStmt(std::unique_ptr<t_Expr> expression)
        : expression(std::move(expression)) {}
};

// Empty statement
struct t_EmptyStmt : public t_Stmt
{
    t_Token semicolon;

    t_EmptyStmt(t_Token semicolon)
        : semicolon(semicolon) {}
};

// Enhanced Display statement with format string support
struct t_DisplayStmt : public t_Stmt
{
    std::vector<std::unique_ptr<t_Expr>> expressions;

    t_DisplayStmt(std::vector<std::unique_ptr<t_Expr>> expressions)
        : expressions(std::move(expressions)) {}
};

struct t_VarStmt : public t_Stmt
{
    std::string name;
    std::unique_ptr<t_Expr> initializer;

    t_VarStmt
    (
        const std::string &name,
        std::unique_ptr<t_Expr> initializer
    )
        : name(name), 
          initializer(std::move(initializer)) {}
};

// Block statement
struct t_BlockStmt : public t_Stmt
{
    std::vector<std::unique_ptr<t_Stmt>> statements;

    t_BlockStmt(std::vector<std::unique_ptr<t_Stmt>> statements)
        : statements(std::move(statements)) {}
};

// If statement
struct t_IfStmt : public t_Stmt
{
    std::unique_ptr<t_Expr> condition;
    std::unique_ptr<t_Stmt> then_branch;
    std::unique_ptr<t_Stmt> else_branch;

    t_IfStmt(std::unique_ptr<t_Expr> condition, 
             std::unique_ptr<t_Stmt> then_branch,
             std::unique_ptr<t_Stmt> else_branch)
        : condition(std::move(condition)), 
          then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}
};

// For loop statement
struct t_ForStmt : public t_Stmt
{
    std::unique_ptr<t_Stmt> initializer;
    std::unique_ptr<t_Expr> condition;
    std::unique_ptr<t_Expr> increment;
    std::unique_ptr<t_Stmt> body;

    t_ForStmt(std::unique_ptr<t_Stmt> initializer,
              std::unique_ptr<t_Expr> condition,
              std::unique_ptr<t_Expr> increment,
              std::unique_ptr<t_Stmt> body)
        : initializer(std::move(initializer)),
          condition(std::move(condition)),
          increment(std::move(increment)),
          body(std::move(body)) {}
};

// Break statement
struct t_BreakStmt : public t_Stmt
{
    t_Token keyword;

    t_BreakStmt(t_Token keyword)
        : keyword(keyword) {}
};

// Continue statement
struct t_ContinueStmt : public t_Stmt
{
    t_Token keyword;

    t_ContinueStmt(t_Token keyword)
        : keyword(keyword) {}
};

// Benchmark statement
struct t_BenchmarkStmt : public t_Stmt
{
    std::unique_ptr<t_Stmt> body;

    t_BenchmarkStmt(std::unique_ptr<t_Stmt> body)
        : body(std::move(body)) {}
};
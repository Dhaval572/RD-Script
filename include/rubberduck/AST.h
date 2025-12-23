#pragma once

#include <string>
#include <vector>
#include <memory>
#include <rubberduck/Token.h>


template<typename T>
struct t_PoolDeleter
{
    void operator()(T* ptr) const
    {
        if (ptr)
        {
            ptr->~T();
        }
    }
};

template<typename T>
using t_PoolPtr = std::unique_ptr<T, t_PoolDeleter<T>>;

struct t_Expr
{
public:
    virtual ~t_Expr() = default;
};

struct t_Stmt
{
public:
    virtual ~t_Stmt() = default;
};

// Expression types
struct t_BinaryExpr : public t_Expr
{
    t_PoolPtr<t_Expr> left;
    t_Token op;
    t_PoolPtr<t_Expr> right;

    t_BinaryExpr
    (
        t_PoolPtr<t_Expr> left, 
        t_Token op, 
        t_PoolPtr<t_Expr> right
    )
        : left(std::move(left)), 
          op(op), 
          right(std::move(right)) {}
};

struct t_LiteralExpr : public t_Expr
{
    std::string value;
    e_TokenType token_type;
    
    t_LiteralExpr
    (
        const std::string &value, 
        e_TokenType type = e_TokenType::STRING
    ) : value(value), token_type(type) {}
};

struct t_UnaryExpr : public t_Expr
{
    t_Token op;
    t_PoolPtr<t_Expr> right;

    t_UnaryExpr(t_Token op, t_PoolPtr<t_Expr> right)
        : op(op), right(std::move(right)) {}
};

struct t_GroupingExpr : public t_Expr
{
    t_PoolPtr<t_Expr> expression;

    t_GroupingExpr(t_PoolPtr<t_Expr> expression)
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
    t_PoolPtr<t_Expr> operand;

    t_PrefixExpr(t_Token op, t_PoolPtr<t_Expr> operand)
        : op(op), operand(std::move(operand)) {}
};

struct t_PostfixExpr : public t_Expr
{
    t_PoolPtr<t_Expr> operand;
    t_Token op;

    t_PostfixExpr(t_PoolPtr<t_Expr> operand, t_Token op)
        : operand(std::move(operand)), op(op) {}
};

struct t_CallExpr : public t_Expr
{
    std::string callee;
    std::vector<t_PoolPtr<t_Expr>> arguments;
    int line;

    t_CallExpr
    (
        const std::string &callee,
        std::vector<t_PoolPtr<t_Expr>> arguments,
        int line = 0
    )
        : callee(callee),
          arguments(std::move(arguments)),
          line(line) {}
};

// Statement types
struct t_ExpressionStmt : public t_Stmt
{
    t_PoolPtr<t_Expr> expression;
    t_ExpressionStmt(t_PoolPtr<t_Expr> expression)
        : expression(std::move(expression)) {}
};

struct t_EmptyStmt : public t_Stmt
{
    t_Token semicolon;

    t_EmptyStmt(t_Token semicolon)
        : semicolon(semicolon) {}
};

struct t_DisplayStmt : public t_Stmt
{
    std::vector<t_PoolPtr<t_Expr>> expressions;

    t_DisplayStmt(std::vector<t_PoolPtr<t_Expr>> expressions)
        : expressions(std::move(expressions)) {}
};

struct t_GetinStmt : public t_Stmt
{
    t_Token keyword;
    std::string variable_name;

    t_GetinStmt
    (
        t_Token keyword,
        const std::string &variable_name
    )
        : keyword(keyword),
          variable_name(variable_name) {}
};

struct t_FunStmt : public t_Stmt
{
    std::string name;
    std::vector<std::string> parameters;
    t_PoolPtr<t_Stmt> body;

    t_FunStmt
    (
        const std::string &name,
        std::vector<std::string> parameters,
        t_PoolPtr<t_Stmt> body
    )
        : name(name),
          parameters(std::move(parameters)),
          body(std::move(body)) {}
};

struct t_VarStmt : public t_Stmt
{
    std::string name;
    t_PoolPtr<t_Expr> initializer;

    t_VarStmt
    (
        const std::string &name,
        t_PoolPtr<t_Expr> initializer
    )
        : name(name), 
          initializer(std::move(initializer)) {}
};

struct t_BlockStmt : public t_Stmt
{
    std::vector<t_PoolPtr<t_Stmt>> statements;

    t_BlockStmt(std::vector<t_PoolPtr<t_Stmt>> statements)
        : statements(std::move(statements)) {}
};

struct t_IfStmt : public t_Stmt
{
    t_PoolPtr<t_Expr> condition;
    t_PoolPtr<t_Stmt> then_branch;
    t_PoolPtr<t_Stmt> else_branch;

    t_IfStmt
    (
        t_PoolPtr<t_Expr> condition, 
        t_PoolPtr<t_Stmt> then_branch,
        t_PoolPtr<t_Stmt> else_branch
    )
        : condition(std::move(condition)), 
          then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}
};

struct t_ForStmt : public t_Stmt
{
    t_PoolPtr<t_Stmt> initializer;
    t_PoolPtr<t_Expr> condition;
    t_PoolPtr<t_Expr> increment;
    t_PoolPtr<t_Stmt> body;

    t_ForStmt
    (   
        t_PoolPtr<t_Stmt> initializer,
        t_PoolPtr<t_Expr> condition,
        t_PoolPtr<t_Expr> increment,
        t_PoolPtr<t_Stmt> body
    )
        : initializer(std::move(initializer)),
          condition(std::move(condition)),
          increment(std::move(increment)),
          body(std::move(body)) {}
};

struct t_BreakStmt : public t_Stmt
{
    t_Token keyword;

    t_BreakStmt(t_Token keyword)
        : keyword(keyword) {}
};

struct t_ContinueStmt : public t_Stmt
{
    t_Token keyword;

    t_ContinueStmt(t_Token keyword)
        : keyword(keyword) {}
};

struct t_BenchmarkStmt : public t_Stmt
{
    t_PoolPtr<t_Stmt> body;

    t_BenchmarkStmt(t_PoolPtr<t_Stmt> body)
        : body(std::move(body)) {}
};

// Add ReturnStmt for handling return statements in functions
struct t_ReturnStmt : public t_Stmt
{
    t_PoolPtr<t_Expr> value;
    
    t_ReturnStmt(t_PoolPtr<t_Expr> value)
        : value(std::move(value)) {}
};
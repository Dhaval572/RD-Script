#pragma once

#include <variant>
#include <rubberduck/Token.h>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>

template<typename T>
struct t_PoolDeleter
{
    void operator()(T* ptr) const noexcept
    {
        if (ptr)
        {
            ptr->~T();
        }
    }
};

template<typename T>
using PoolPtr = std::unique_ptr<T, t_PoolDeleter<T>>;

struct t_Expr;
struct t_Stmt;

struct t_BinaryExpr;
struct t_LiteralExpr;
struct t_UnaryExpr;
struct t_GroupingExpr;
struct t_VariableExpr;
struct t_PrefixExpr;
struct t_PostfixExpr;
struct t_CallExpr;
struct t_TypeofExpr;

struct t_BlockStmt;
struct t_IfStmt;
struct t_ForStmt;
struct t_BreakStmt;
struct t_ContinueStmt;
struct t_VarStmt;
struct t_DisplayStmt;
struct t_GetinStmt;
struct t_FunStmt;
struct t_BenchmarkStmt;
struct t_EmptyStmt;
struct t_ExpressionStmt;
struct t_ReturnStmt;

struct t_Expr
{
public:
    virtual ~t_Expr() = default;
    
    virtual bool IsBinary() const { return false; }
    virtual bool IsLiteral() const { return false; }
    virtual bool IsUnary() const { return false; }
    virtual bool IsGrouping() const { return false; }
    virtual bool IsVariable() const { return false; }
    virtual bool IsPrefix() const { return false; }
    virtual bool IsPostfix() const { return false; }
    virtual bool IsCall() const { return false; }
    virtual bool IsTypeof() const { return false; }
    
    virtual t_BinaryExpr* AsBinary() { return nullptr; }
    virtual t_LiteralExpr* AsLiteral() { return nullptr; }
    virtual t_UnaryExpr* AsUnary() { return nullptr; }
    virtual t_GroupingExpr* AsGrouping() { return nullptr; }
    virtual t_VariableExpr* AsVariable() { return nullptr; }
    virtual t_PrefixExpr* AsPrefix() { return nullptr; }
    virtual t_PostfixExpr* AsPostfix() { return nullptr; }
    virtual t_CallExpr* AsCall() { return nullptr; }
    virtual t_TypeofExpr* AsTypeof() { return nullptr; }
};

struct t_Stmt
{
public:
    virtual ~t_Stmt() = default;
    
    virtual bool IsBlock() const { return false; }
    virtual bool IsIf() const { return false; }
    virtual bool IsFor() const { return false; }
    virtual bool IsBreak() const { return false; }
    virtual bool IsContinue() const { return false; }
    virtual bool IsVar() const { return false; }
    virtual bool IsDisplay() const { return false; }
    virtual bool IsGetin() const { return false; }
    virtual bool IsFunction() const { return false; }
    virtual bool IsBenchmark() const { return false; }
    virtual bool IsEmpty() const { return false; }
    virtual bool IsExpression() const { return false; }
    virtual bool IsReturn() const { return false; }
    
    virtual t_BlockStmt* AsBlock() { return nullptr; }
    virtual t_IfStmt* AsIf() { return nullptr; }
    virtual t_ForStmt* AsFor() { return nullptr; }
    virtual t_BreakStmt* AsBreak() { return nullptr; }
    virtual t_ContinueStmt* AsContinue() { return nullptr; }
    virtual t_VarStmt* AsVar() { return nullptr; }
    virtual t_DisplayStmt* AsDisplay() { return nullptr; }
    virtual t_GetinStmt* AsGetin() { return nullptr; }
    virtual t_FunStmt* AsFunction() { return nullptr; }
    virtual t_BenchmarkStmt* AsBenchmark() { return nullptr; }
    virtual t_EmptyStmt* AsEmpty() { return nullptr; }
    virtual t_ExpressionStmt* AsExpression() { return nullptr; }
    virtual t_ReturnStmt* AsReturn() { return nullptr; }
};

struct t_BinaryExpr : public t_Expr
{
    PoolPtr<t_Expr> left;
    t_Token op;
    PoolPtr<t_Expr> right;

    t_BinaryExpr
    (
        PoolPtr<t_Expr> left, 
        t_Token op, 
        PoolPtr<t_Expr> right
    )
        : left(std::move(left)), 
          op(op), 
          right(std::move(right)) {}
    
    bool IsBinary() const override { return true; }
    t_BinaryExpr* AsBinary() override { return this; }
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
    
    bool IsLiteral() const override { return true; }
    t_LiteralExpr* AsLiteral() override { return this; }
};

struct t_UnaryExpr : public t_Expr
{
    t_Token op;
    PoolPtr<t_Expr> right;

    t_UnaryExpr(t_Token op, PoolPtr<t_Expr> right)
        : op(op), right(std::move(right)) {}
    
    bool IsUnary() const override { return true; }
    t_UnaryExpr* AsUnary() override { return this; }
};

struct t_GroupingExpr : public t_Expr
{
    PoolPtr<t_Expr> expression;

    t_GroupingExpr(PoolPtr<t_Expr> expression)
        : expression(std::move(expression)) {}
    
    bool IsGrouping() const override { return true; }
    t_GroupingExpr* AsGrouping() override { return this; }
};

struct t_VariableExpr : public t_Expr
{
    std::string name;
    t_VariableExpr(const std::string &name) : name(name) {}
    
    bool IsVariable() const override { return true; }
    t_VariableExpr* AsVariable() override { return this; }
};

struct t_PrefixExpr : public t_Expr
{
    t_Token op;
    PoolPtr<t_Expr> operand;

    t_PrefixExpr(t_Token op, PoolPtr<t_Expr> operand)
        : op(op), operand(std::move(operand)) {}
    
    bool IsPrefix() const override { return true; }
    t_PrefixExpr* AsPrefix() override { return this; }
};

struct t_PostfixExpr : public t_Expr
{
    PoolPtr<t_Expr> operand;
    t_Token op;

    t_PostfixExpr(PoolPtr<t_Expr> operand, t_Token op)
        : operand(std::move(operand)), op(op) {}
    
    bool IsPostfix() const override { return true; }
    t_PostfixExpr* AsPostfix() override { return this; }
};

struct t_CallExpr : public t_Expr
{
    std::string callee;
    std::vector<PoolPtr<t_Expr>> arguments;
    int line;

    t_CallExpr
    (
        const std::string &callee,
        std::vector<PoolPtr<t_Expr>> arguments,
        int line = 0
    )
        : callee(callee),
          arguments(std::move(arguments)),
          line(line) {}
    
    bool IsCall() const override { return true; }
    t_CallExpr* AsCall() override { return this; }
};

struct t_TypeofExpr : public t_Expr
{
    PoolPtr<t_Expr> operand;

    t_TypeofExpr(PoolPtr<t_Expr> operand)
        : operand(std::move(operand)) {}

    bool IsTypeof() const override { return true; }
    t_TypeofExpr* AsTypeof() override { return this; }
};

struct t_ExpressionStmt : public t_Stmt
{
    PoolPtr<t_Expr> expression;
    t_ExpressionStmt(PoolPtr<t_Expr> expression)
        : expression(std::move(expression)) {}
    
    bool IsExpression() const override { return true; }
    t_ExpressionStmt* AsExpression() override { return this; }
};

struct t_EmptyStmt : public t_Stmt
{
    t_Token semicolon;

    t_EmptyStmt(t_Token semicolon)
        : semicolon(semicolon) {}
    
    bool IsEmpty() const override { return true; }
    t_EmptyStmt* AsEmpty() override { return this; }
};

struct t_DisplayStmt : public t_Stmt
{
    std::vector<PoolPtr<t_Expr>> expressions;

    t_DisplayStmt(std::vector<PoolPtr<t_Expr>> expressions)
        : expressions(std::move(expressions)) {}
    
    bool IsDisplay() const override { return true; }
    t_DisplayStmt* AsDisplay() override { return this; }
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
    
    bool IsGetin() const override { return true; }
    t_GetinStmt* AsGetin() override { return this; }
};

struct t_FunStmt : public t_Stmt
{
    std::string name;
    std::vector<std::string> parameters;
    PoolPtr<t_Stmt> body;

    t_FunStmt
    (
        const std::string &name,
        std::vector<std::string> parameters,
        PoolPtr<t_Stmt> body
    )
        : name(name),
          parameters(std::move(parameters)),
          body(std::move(body)) {}
    
    bool IsFunction() const override { return true; }
    t_FunStmt* AsFunction() override { return this; }
};

struct t_VarStmt : public t_Stmt
{
    std::string name;
    PoolPtr<t_Expr> initializer;

    t_VarStmt
    (
        const std::string &name,
        PoolPtr<t_Expr> initializer
    )
        : name(name), 
          initializer(std::move(initializer)) {}
    
    bool IsVar() const override { return true; }
    t_VarStmt* AsVar() override { return this; }
};

struct t_BlockStmt : public t_Stmt
{
    std::vector<PoolPtr<t_Stmt>> statements;

    t_BlockStmt(std::vector<PoolPtr<t_Stmt>> statements)
        : statements(std::move(statements)) {}
    
    bool IsBlock() const override { return true; }
    t_BlockStmt* AsBlock() override { return this; }
};

struct t_IfStmt : public t_Stmt
{
    PoolPtr<t_Expr> condition;
    PoolPtr<t_Stmt> then_branch;
    PoolPtr<t_Stmt> else_branch;

    t_IfStmt
    (
        PoolPtr<t_Expr> condition, 
        PoolPtr<t_Stmt> then_branch,
        PoolPtr<t_Stmt> else_branch
    )
        : condition(std::move(condition)), 
          then_branch(std::move(then_branch)),
          else_branch(std::move(else_branch)) {}
    
    bool IsIf() const override { return true; }
    t_IfStmt* AsIf() override { return this; }
};

struct t_ForStmt : public t_Stmt
{
    PoolPtr<t_Stmt> initializer;
    PoolPtr<t_Expr> condition;
    PoolPtr<t_Expr> increment;
    PoolPtr<t_Stmt> body;

    t_ForStmt
    (   
        PoolPtr<t_Stmt> initializer,
        PoolPtr<t_Expr> condition,
        PoolPtr<t_Expr> increment,
        PoolPtr<t_Stmt> body
    )
        : initializer(std::move(initializer)),
          condition(std::move(condition)),
          increment(std::move(increment)),
          body(std::move(body)) {}
    
    bool IsFor() const override { return true; }
    t_ForStmt* AsFor() override { return this; }
};

struct t_BreakStmt : public t_Stmt
{
    t_Token keyword;

    t_BreakStmt(t_Token keyword)
        : keyword(keyword) {}
     
    bool IsBreak() const override { return true; }
    t_BreakStmt* AsBreak() override { return this; }
};

struct t_ContinueStmt : public t_Stmt
{
    t_Token keyword;

    t_ContinueStmt(t_Token keyword)
        : keyword(keyword) {}
    
    bool IsContinue() const override { return true; }
    t_ContinueStmt* AsContinue() override { return this; }
};

struct t_BenchmarkStmt : public t_Stmt
{
    PoolPtr<t_Stmt> body;

    t_BenchmarkStmt(PoolPtr<t_Stmt> body)
        : body(std::move(body)) {}
    
    bool IsBenchmark() const override { return true; }
    t_BenchmarkStmt* AsBenchmark() override { return this; }
};

struct t_ReturnStmt : public t_Stmt
{
    PoolPtr<t_Expr> value;
    
    t_ReturnStmt(PoolPtr<t_Expr> value)
        : value(std::move(value)) {}
    
    bool IsReturn() const override { return true; }
    t_ReturnStmt* AsReturn() override { return this; }
};

using t_StmtVariant = std::variant
<
    t_BlockStmt,
    t_IfStmt,
    t_ForStmt,
    t_BreakStmt,
    t_ContinueStmt,
    t_VarStmt,
    t_DisplayStmt,
    t_GetinStmt,
    t_FunStmt,
    t_BenchmarkStmt,
    t_EmptyStmt,
    t_ExpressionStmt,
    t_ReturnStmt
>;

using t_ExprVariant = std::variant
<
    t_BinaryExpr,
    t_LiteralExpr,
    t_UnaryExpr,
    t_GroupingExpr,
    t_VariableExpr,
    t_PrefixExpr,
    t_PostfixExpr,
    t_CallExpr,
    t_TypeofExpr
>;

namespace ast_internal 
{
    template<typename T> T* AsStmt(t_Stmt* stmt);
    template<typename T> T* AsExpr(t_Expr* expr);
}

template<typename T>
T* As(t_Expr* expr);

template<typename T>
T* As(t_Stmt* stmt);

namespace ast_internal 
{
    template<typename T>
    T* AsStmt(t_Stmt* stmt) 
    {
        if (!stmt) return nullptr;
        
        if constexpr (std::is_same_v<T, t_BlockStmt>) 
        {
            return stmt->IsBlock() ? stmt->AsBlock() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_IfStmt>) 
        {
            return stmt->IsIf() ? stmt->AsIf() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_ForStmt>) 
        {
            return stmt->IsFor() ? stmt->AsFor() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_BreakStmt>) 
        {
            return stmt->IsBreak() ? stmt->AsBreak() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_ContinueStmt>) 
        {
            return stmt->IsContinue() ? stmt->AsContinue() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_VarStmt>) 
        {
            return stmt->IsVar() ? stmt->AsVar() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_DisplayStmt>) 
        {
            return stmt->IsDisplay() ? stmt->AsDisplay() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_GetinStmt>) 
        {
            return stmt->IsGetin() ? stmt->AsGetin() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_FunStmt>) 
        {
            return stmt->IsFunction() ? stmt->AsFunction() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_BenchmarkStmt>) 
        {
            return stmt->IsBenchmark() ? stmt->AsBenchmark() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_EmptyStmt>) 
        {
            return stmt->IsEmpty() ? stmt->AsEmpty() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_ExpressionStmt>) 
        {
            return stmt->IsExpression() ? stmt->AsExpression() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_ReturnStmt>) 
        {
            return stmt->IsReturn() ? stmt->AsReturn() : nullptr;
        } 
        else 
        {
            static_assert(sizeof(T) == 0, "Unsupported statement type");
            return nullptr;
        }
    }

    template<typename T>
    T* AsExpr(t_Expr* expr) 
    {
        if (!expr) return nullptr;
        
        if constexpr (std::is_same_v<T, t_BinaryExpr>) 
        {
            return expr->IsBinary() ? expr->AsBinary() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_LiteralExpr>) 
        {
            return expr->IsLiteral() ? expr->AsLiteral() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_UnaryExpr>) 
        {
            return expr->IsUnary() ? expr->AsUnary() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_GroupingExpr>) 
        {
            return expr->IsGrouping() ? expr->AsGrouping() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_VariableExpr>) 
        {
            return expr->IsVariable() ? expr->AsVariable() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_PrefixExpr>) 
        {
            return expr->IsPrefix() ? expr->AsPrefix() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_PostfixExpr>) 
        {
            return expr->IsPostfix() ? expr->AsPostfix() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_CallExpr>) 
        {
            return expr->IsCall() ? expr->AsCall() : nullptr;
        } 
        else if constexpr (std::is_same_v<T, t_TypeofExpr>) 
        {
            return expr->IsTypeof() ? expr->AsTypeof() : nullptr;
        } 
        else 
        {
            static_assert(sizeof(T) == 0, "Unsupported expression type");
            return nullptr;
        }
    }
} 

template<typename T>
T* As(t_Expr* expr) 
{
    static_assert
    (
        std::is_base_of_v<t_Expr, T>, 
        "As<T>(t_Expr*) requires T derived from t_Expr"
    );
    return ast_internal::AsExpr<T>(expr);
}

template<typename T>
T* As(t_Stmt* stmt) 
{
    static_assert
    (
        std::is_base_of_v<t_Stmt, T>, 
        "As<T>(t_Stmt*) requires T derived from t_Stmt"
    );
    return ast_internal::AsStmt<T>(stmt);
}
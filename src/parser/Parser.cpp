#include "../include/Parser.h"
#include "AST.h"
#include <stdexcept>
#include <iostream>

// Initialize static memory pools with optimized sizes
// Larger block size for statements since they can contain multiple sub-elements
t_MemoryPool t_Parser::stmt_pool(sizeof(t_VarStmt) > sizeof(t_DisplayStmt) ? 
                                 sizeof(t_VarStmt) : sizeof(t_DisplayStmt));
// Larger block size for expressions since they can be complex
t_MemoryPool t_Parser::expr_pool(sizeof(t_BinaryExpr) > sizeof(t_LiteralExpr) ? 
                                 sizeof(t_BinaryExpr) : sizeof(t_LiteralExpr));

t_Parser::t_Parser(const std::vector<t_Token> &tokens)
    : tokens(tokens), current(0) {}

std::vector<t_Stmt *> t_Parser::Parse()
{
    std::vector<t_Stmt *> statements;
    while (!IsAtEnd())
    {
        statements.push_back(Statement());
    }

    return statements;
}

// Reset the memory pools
void t_Parser::ResetPools()
{
    stmt_pool.Reset();
    expr_pool.Reset();
}

bool t_Parser::IsAtEnd()
{
    return Peek().type == t_TokenType::EOF_TOKEN;
}

t_Token t_Parser::Advance()
{
    if (!IsAtEnd()) current++;
    return Previous();
}

bool t_Parser::Check(t_TokenType type)
{
    if (IsAtEnd()) return false;
    return Peek().type == type;
}

t_Token t_Parser::Peek()
{
    if (current >= tokens.size())
    {
        // Return EOF token if we're past the end of the tokens vector
        return t_Token(t_TokenType::EOF_TOKEN, "", "", 0);
    }
    return tokens[current];
}

t_Token t_Parser::Previous()
{
    if (current <= 0)
    {
        // Return EOF token if we're at the beginning or before
        return t_Token(t_TokenType::EOF_TOKEN, "", "", 0);
    }
    return tokens[current - 1];
}

bool t_Parser::Match(std::initializer_list<t_TokenType> types)
{
    for (t_TokenType type : types)
    {
        if (Check(type))
        {
            Advance();
            return true;
        }
    }

    return false;
}

t_Token t_Parser::Consume(t_TokenType type, const std::string &message)
{
    if (Check(type))
    {
        return Advance();
    }

    throw std::runtime_error(message + " at line " + std::to_string(Peek().line));
}

std::string t_Parser::Error(t_Token token, const std::string &message)
{
    // In a real implementation, we would report the error
    std::cerr << "Error: " << message << " at line " << token.line << std::endl;
    return message;
}

t_Stmt *t_Parser::Statement()
{
    if (Match({t_TokenType::LEFT_BRACE}))
    {
        return BlockStatement();
    }

    if (Match({t_TokenType::IF}))
    {
        return IfStatement();
    }

    if (Match({t_TokenType::FOR}))
    {
        return ForStatement();
    }

    if (Match({t_TokenType::BREAK}))
    {
        return BreakStatement();
    }

    if (Match({t_TokenType::CONTINUE}))
    {
        return ContinueStatement();
    }

    if (Match({t_TokenType::AUTO}))
    {
        return VarDeclaration();
    }

    if (Match({t_TokenType::DISPLAY}))
    {
        return DisplayStatement();
    }

    if (Match({t_TokenType::BENCHMARK}))
    {
        return BenchmarkStatement();
    }

    if (Match({t_TokenType::SEMICOLON}))
    {
        return EmptyStatement();
    }

    return ExpressionStatement();
}

t_Stmt *t_Parser::BlockStatement()
{
    std::vector<std::unique_ptr<t_Stmt>> statements;

    while (!Check(t_TokenType::RIGHT_BRACE) && !IsAtEnd())
    {
        statements.push_back(std::unique_ptr<t_Stmt>(Statement()));
    }

    Consume(t_TokenType::RIGHT_BRACE, "Expect '}' after block.");
    t_BlockStmt* stmt = static_cast<t_BlockStmt*>(stmt_pool.Allocate());
    new (stmt) t_BlockStmt(std::move(statements));
    return stmt;
} 

t_Stmt *t_Parser::BreakStatement()
{
    t_Token keyword = Previous();
    Consume(t_TokenType::SEMICOLON, "Expect ';' after 'break'.");
    t_BreakStmt* stmt = static_cast<t_BreakStmt*>(stmt_pool.Allocate());
    new (stmt) t_BreakStmt(keyword);
    return stmt;
}

t_Stmt *t_Parser::ContinueStatement()
{
    t_Token keyword = Previous();
    Consume(t_TokenType::SEMICOLON, "Expect ';' after 'continue'.");
    t_ContinueStmt* stmt = static_cast<t_ContinueStmt*>(stmt_pool.Allocate());
    new (stmt) t_ContinueStmt(keyword);
    return stmt;
}

t_Stmt *t_Parser::IfStatement()
{
    Consume
    (
        t_TokenType::LEFT_PAREN, 
        "Expect '(' after 'if'."
    );
    t_Expr *condition = Expression();
    Consume(t_TokenType::RIGHT_PAREN, "Expect ')' after if condition.");

    // Parse the then branch
    t_Stmt *then_branch = Statement();

    // Check for else branch
    std::unique_ptr<t_Stmt> else_branch = nullptr;
    if (Match({t_TokenType::ELSE}))
    {
        else_branch = std::unique_ptr<t_Stmt>(Statement());
    }

    t_IfStmt* stmt = static_cast<t_IfStmt*>(stmt_pool.Allocate());
    new (stmt) t_IfStmt
    (
        std::unique_ptr<t_Expr>(condition), 
        std::unique_ptr<t_Stmt>(then_branch), 
        std::move(else_branch)
    );
    return stmt;
}

t_Stmt *t_Parser::ForStatement()
{
    Consume(t_TokenType::LEFT_PAREN, "Expect '(' after 'for'.");

    // Parse initializer (can be a variable declaration or an expression statement)
    std::unique_ptr<t_Stmt> initializer;
    if (Match({t_TokenType::SEMICOLON}))
    {
        // No initializer
        initializer = nullptr;
    }
    else if (Match({t_TokenType::AUTO}))
    {
        // Variable declaration
        initializer = std::unique_ptr<t_Stmt>(VarDeclaration());
        // Note: VarDeclaration already consumes the semicolon
        // So we don't need to consume it here
    }
    else
    {
        // Expression statement
        initializer = std::unique_ptr<t_Stmt>(ExpressionStatement());
    }

    // Parse condition
    std::unique_ptr<t_Expr> condition;
    if (!Check(t_TokenType::SEMICOLON))
    {
        condition = std::unique_ptr<t_Expr>(Expression());
    }
    Consume(t_TokenType::SEMICOLON, "Expect ';' after loop condition.");

    // Parse increment
    std::unique_ptr<t_Expr> increment;
    if (!Check(t_TokenType::RIGHT_PAREN))
    {
        increment = std::unique_ptr<t_Expr>(Expression());
    }
    Consume(t_TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

    // Parse body
    t_Stmt *body = Statement();

    t_ForStmt* stmt = static_cast<t_ForStmt*>(stmt_pool.Allocate());
    new (stmt) t_ForStmt
    (
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        std::unique_ptr<t_Stmt>(body)
    );
    return stmt;
}

t_Stmt *t_Parser::VarDeclaration()
{
    t_Token name = Consume(t_TokenType::IDENTIFIER, "Expect variable name.");

    std::unique_ptr<t_Expr> initializer = nullptr;
    if (Match({t_TokenType::EQUAL}))
    {
        initializer = std::unique_ptr<t_Expr>(Expression());
    }

    Consume(t_TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    t_VarStmt* stmt = static_cast<t_VarStmt*>(stmt_pool.Allocate());
    new (stmt) t_VarStmt(name.lexeme, std::move(initializer));
    return stmt;
}

t_Stmt *t_Parser::DisplayStatement()
{
    std::vector<std::unique_ptr<t_Expr>> values;

    // Parse the first expression
    values.push_back(std::unique_ptr<t_Expr>(Expression()));

    // Parse additional comma-separated expressions
    while (Match({t_TokenType::COMMA}))
    {
        values.push_back(std::unique_ptr<t_Expr>(Expression()));
    }

    Consume(t_TokenType::SEMICOLON, "Expect ';' after value.");

    // Use the new t_DisplayStmt instead of t_PrintStmt
    t_DisplayStmt* stmt = static_cast<t_DisplayStmt*>(stmt_pool.Allocate());
    new (stmt) t_DisplayStmt(std::move(values));
    return stmt;
}

t_Stmt *t_Parser::ExpressionStatement()
{
    t_Expr *expr = Expression();
    Consume(t_TokenType::SEMICOLON, "Expect ';' after expression.");
    t_ExpressionStmt* stmt = static_cast<t_ExpressionStmt*>(stmt_pool.Allocate());
    new (stmt) t_ExpressionStmt(std::unique_ptr<t_Expr>(expr));
    return stmt;
}

t_Stmt *t_Parser::EmptyStatement()
{
    t_Token semicolon = Previous();
    t_EmptyStmt* stmt = static_cast<t_EmptyStmt*>(stmt_pool.Allocate());
    new (stmt) t_EmptyStmt(semicolon);
    return stmt;
}

t_Stmt *t_Parser::BenchmarkStatement()
{
    Consume(t_TokenType::LEFT_BRACE, "Expect '{' after 'benchmark'.");
    
    // Parse the benchmark body
    std::vector<std::unique_ptr<t_Stmt>> statements;
    while (!Check(t_TokenType::RIGHT_BRACE) && !IsAtEnd())
    {
        statements.push_back(std::unique_ptr<t_Stmt>(Statement()));
    }
    
    Consume(t_TokenType::RIGHT_BRACE, "Expect '}' after benchmark body.");
    
    // Create a block statement for the body
    t_Stmt *body = new t_BlockStmt(std::move(statements));
    
    t_BenchmarkStmt* stmt = static_cast<t_BenchmarkStmt*>(stmt_pool.Allocate());
    new (stmt) t_BenchmarkStmt(std::unique_ptr<t_Stmt>(body));
    return stmt;
}

t_Expr *t_Parser::Expression()
{
    return Assignment();
}

t_Expr *t_Parser::Assignment()
{
    t_Expr *expr = Or();

    if (Match({t_TokenType::EQUAL, t_TokenType::PLUS_EQUAL, t_TokenType::MINUS_EQUAL, 
               t_TokenType::STAR_EQUAL, t_TokenType::SLASH_EQUAL}))
    {
        t_Token equals = Previous();
        t_Expr *value = Assignment();

        if (t_VariableExpr *var_expr = dynamic_cast<t_VariableExpr *>(expr))
        {
            std::string name = var_expr->name;
            t_BinaryExpr* expr_node = static_cast<t_BinaryExpr*>(expr_pool.Allocate());
            new (expr_node) t_BinaryExpr(std::unique_ptr<t_Expr>(expr), equals, std::unique_ptr<t_Expr>(value));
            return expr_node;
        }

        // If we get here, we're trying to assign to a non-variable
        throw std::runtime_error("Invalid assignment target at line " + std::to_string(equals.line));
    }

    return expr;
}

t_Expr *t_Parser::Or()
{
    t_Expr *expr = And();

    while (Match({t_TokenType::OR}))
    {
        t_Token op = Previous();
        t_Expr *right = And();
        t_BinaryExpr* expr_node = static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return expr;
}

t_Expr *t_Parser::And()
{
    t_Expr *expr = Equality();

    while (Match({t_TokenType::AND}))
    {
        t_Token op = Previous();
        t_Expr *right = Equality();
        t_BinaryExpr* expr_node = static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return expr;
}

t_Expr *t_Parser::Equality()
{
    t_Expr *expr = Comparison();

    while (Match({t_TokenType::BANG_EQUAL, t_TokenType::EQUAL_EQUAL}))
    {
        t_Token op = Previous();
        t_Expr *right = Comparison();
        t_BinaryExpr* expr_node = static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return expr;
}

t_Expr *t_Parser::Comparison()
{
    t_Expr *expr = Term();
    while 
    (
        Match
        (
            {
                t_TokenType::GREATER, 
                t_TokenType::GREATER_EQUAL, 
                t_TokenType::LESS, 
                t_TokenType::LESS_EQUAL
            }
        )
    )
    {
        t_Token op = Previous();
        t_Expr *right = Term();
        t_BinaryExpr* expr_node = static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return expr;
}

t_Expr *t_Parser::Term()
{
    t_Expr *expr = Factor();

    while (Match({t_TokenType::MINUS, t_TokenType::PLUS}))
    {
        t_Token op = Previous();
        t_Expr *right = Factor();
        t_BinaryExpr* expr_node = static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return expr;
}

t_Expr *t_Parser::Factor()
{
    t_Expr *expr = Unary();

    while (Match({t_TokenType::SLASH, t_TokenType::STAR}))
    {
        t_Token op = Previous();
        t_Expr *right = Unary();
        t_BinaryExpr* expr_node = static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return expr;
}

t_Expr *t_Parser::Unary()
{
    if (Match({t_TokenType::BANG, t_TokenType::MINUS}))
    {
        t_Token op = Previous();
        t_Expr *right = Unary();
        t_UnaryExpr* expr_node = static_cast<t_UnaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_UnaryExpr(op, std::unique_ptr<t_Expr>(right));
        return expr_node;
    }

    return FinishUnary();
}

t_Expr *t_Parser::FinishUnary()
{
    t_Expr *expr = Primary();

    // Handle postfix increment/decrement
    while (Match({t_TokenType::PLUS_PLUS, t_TokenType::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_PostfixExpr* expr_node = static_cast<t_PostfixExpr*>(expr_pool.Allocate());
        new (expr_node) t_PostfixExpr(std::unique_ptr<t_Expr>(expr), op);
        expr = expr_node;
    }

    return expr;
}

t_Expr *t_Parser::Primary()
{
    if (Match({t_TokenType::FALSE}))
    {
        t_LiteralExpr* expr_node = static_cast<t_LiteralExpr*>(expr_pool.Allocate());
        new (expr_node) t_LiteralExpr("false", t_TokenType::FALSE);
        return expr_node;
    }

    if (Match({t_TokenType::TRUE}))
    {
        t_LiteralExpr* expr_node = static_cast<t_LiteralExpr*>(expr_pool.Allocate());
        new (expr_node) t_LiteralExpr("true", t_TokenType::TRUE);
        return expr_node;
    }

    if (Match({t_TokenType::NIL}))
    {
        t_LiteralExpr* expr_node = static_cast<t_LiteralExpr*>(expr_pool.Allocate());
        new (expr_node) t_LiteralExpr("nil", t_TokenType::NIL);
        return expr_node;
    }

    if (Match({t_TokenType::NUMBER, t_TokenType::STRING, t_TokenType::FORMAT_STRING}))
    {
        t_Token previous = Previous();
        t_LiteralExpr* expr_node = static_cast<t_LiteralExpr*>(expr_pool.Allocate());
        new (expr_node) t_LiteralExpr(previous.literal, previous.type);
        return expr_node;
    }

    // Handle prefix increment/decrement
    if (Match({t_TokenType::PLUS_PLUS, t_TokenType::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_Expr *operand = Primary();
        t_PrefixExpr* expr_node = static_cast<t_PrefixExpr*>(expr_pool.Allocate());
        new (expr_node) t_PrefixExpr(op, std::unique_ptr<t_Expr>(operand));
        return expr_node;
    }

    if (Match({t_TokenType::IDENTIFIER}))
    {
        t_VariableExpr* expr_node = static_cast<t_VariableExpr*>(expr_pool.Allocate());
        new (expr_node) t_VariableExpr(Previous().lexeme);
        return expr_node;
    }

    if (Match({t_TokenType::LEFT_PAREN}))
    {
        t_Expr *expr = Expression();
        Consume
        (
            t_TokenType::RIGHT_PAREN, 
            "Expect ')' after expression."
        );
        t_GroupingExpr* expr_node = static_cast<t_GroupingExpr*>(expr_pool.Allocate());
        new (expr_node) t_GroupingExpr(std::unique_ptr<t_Expr>(expr));
        return expr_node;
    }

    throw std::runtime_error
    (
        "Expect expression at line " + std::to_string(Peek().line)
    );
}
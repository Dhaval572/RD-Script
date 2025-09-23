#include "../include/Parser.h"
#include "AST.h"
#include <stdexcept>
#include <iostream>

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

bool t_Parser::IsAtEnd()
{
    return Peek().type == t_TokenType::EOF_TOKEN;
}

t_Token t_Parser::Advance()
{
    if (!IsAtEnd())
    {
        current++;
    }
    return Previous();
}

bool t_Parser::Check(t_TokenType type)
{
    if (IsAtEnd())
    {
        return false;
    }

    return Peek().type == type;
}

t_Token t_Parser::Peek()
{
    return tokens[current];
}

t_Token t_Parser::Previous()
{
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
    if (Match({t_TokenType::AUTO}))
    {
        return VarDeclaration();
    }

    if (Match({t_TokenType::DISPLAY}))
    {
        return DisplayStatement();
    }

    return ExpressionStatement();
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
    return new t_VarStmt(name.lexeme, std::move(initializer));
}

t_Stmt *t_Parser::DisplayStatement()
{
    t_Expr *value = Expression();
    Consume(t_TokenType::SEMICOLON, "Expect ';' after value.");
    
    // Use the new t_DisplayStmt instead of t_PrintStmt
    return new t_DisplayStmt(std::unique_ptr<t_Expr>(value));
}

t_Stmt *t_Parser::ExpressionStatement()
{
    t_Expr *expr = Expression();
    Consume(t_TokenType::SEMICOLON, "Expect ';' after expression.");
    return new t_ExpressionStmt(std::unique_ptr<t_Expr>(expr));
}

t_Expr *t_Parser::Expression()
{
    return Equality();
}

t_Expr *t_Parser::Equality()
{
    t_Expr *expr = Comparison();

    while (Match({t_TokenType::BANG_EQUAL, t_TokenType::EQUAL_EQUAL}))
    {
        t_Token op = Previous();
        t_Expr *right = Comparison();
        expr = new t_BinaryExpr(std::unique_ptr<t_Expr>(expr), op, std::unique_ptr<t_Expr>(right));
    }

    return expr;
}

t_Expr *t_Parser::Comparison()
{
    t_Expr *expr = Term();

    while (Match({t_TokenType::GREATER, t_TokenType::GREATER_EQUAL, t_TokenType::LESS, t_TokenType::LESS_EQUAL}))
    {
        t_Token op = Previous();
        t_Expr *right = Term();
        expr = new t_BinaryExpr(std::unique_ptr<t_Expr>(expr), op, std::unique_ptr<t_Expr>(right));
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
        expr = new t_BinaryExpr(std::unique_ptr<t_Expr>(expr), op, std::unique_ptr<t_Expr>(right));
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
        expr = new t_BinaryExpr(std::unique_ptr<t_Expr>(expr), op, std::unique_ptr<t_Expr>(right));
    }

    return expr;
}

t_Expr *t_Parser::Unary()
{
    if (Match({t_TokenType::BANG, t_TokenType::MINUS}))
    {
        t_Token op = Previous();
        t_Expr *right = Unary();
        return new t_UnaryExpr(op, std::unique_ptr<t_Expr>(right));
    }

    return Primary();
}

t_Expr *t_Parser::Primary()
{
    if (Match({t_TokenType::FALSE}))
    {
        return new t_LiteralExpr("false");
    }
        
    if (Match({t_TokenType::TRUE}))
    {
        return new t_LiteralExpr("true");
    }
    
    if (Match({t_TokenType::NIL}))
    {
        return new t_LiteralExpr("nil");
    }

    if (Match({t_TokenType::NUMBER, t_TokenType::STRING}))
    {
        return new t_LiteralExpr(Previous().literal);
    }

    if (Match({t_TokenType::IDENTIFIER}))
    {
        return new t_VariableExpr(Previous().lexeme);
    }

    if (Match({t_TokenType::LEFT_PAREN}))
    {
        t_Expr *expr = Expression();
        Consume(t_TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return new t_GroupingExpr(std::unique_ptr<t_Expr>(expr));
    }

    throw std::runtime_error("Expect expression at line " + std::to_string(Peek().line));
}
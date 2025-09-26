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
    return new t_BlockStmt(std::move(statements));
} 

t_Stmt *t_Parser::BreakStatement()
{
    t_Token keyword = Previous();
    Consume(t_TokenType::SEMICOLON, "Expect ';' after 'break'.");
    return new t_BreakStmt(keyword);
}

t_Stmt *t_Parser::ContinueStatement()
{
    t_Token keyword = Previous();
    Consume(t_TokenType::SEMICOLON, "Expect ';' after 'continue'.");
    return new t_ContinueStmt(keyword);
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

    return new t_IfStmt
    (
        std::unique_ptr<t_Expr>(condition), 
        std::unique_ptr<t_Stmt>(then_branch), 
        std::move(else_branch)
    );
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

    return new t_ForStmt
    (
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        std::unique_ptr<t_Stmt>(body)
    );
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
    return new t_DisplayStmt(std::move(values));
}

t_Stmt *t_Parser::ExpressionStatement()
{
    t_Expr *expr = Expression();
    Consume(t_TokenType::SEMICOLON, "Expect ';' after expression.");
    return new t_ExpressionStmt(std::unique_ptr<t_Expr>(expr));
}

t_Stmt *t_Parser::EmptyStatement()
{
    t_Token semicolon = Previous();
    return new t_EmptyStmt(semicolon);
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
    
    return new t_BenchmarkStmt(std::unique_ptr<t_Stmt>(body));
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
            return new t_BinaryExpr(std::unique_ptr<t_Expr>(expr), equals, std::unique_ptr<t_Expr>(value));
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
        expr = new t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
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
        expr = new t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
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
        expr = new t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
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
        expr = new t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
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
        expr = new t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
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
        expr = new t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
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

    return FinishUnary();
}

t_Expr *t_Parser::FinishUnary()
{
    t_Expr *expr = Primary();

    // Handle postfix increment/decrement
    while (Match({t_TokenType::PLUS_PLUS, t_TokenType::MINUS_MINUS}))
    {
        t_Token op = Previous();
        expr = new t_PostfixExpr(std::unique_ptr<t_Expr>(expr), op);
    }

    return expr;
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

    if (Match({t_TokenType::NUMBER, t_TokenType::STRING, t_TokenType::FORMAT_STRING}))
    {
        return new t_LiteralExpr(Previous().literal);
    }

    // Handle prefix increment/decrement
    if (Match({t_TokenType::PLUS_PLUS, t_TokenType::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_Expr *operand = Primary();
        return new t_PrefixExpr(op, std::unique_ptr<t_Expr>(operand));
    }

    if (Match({t_TokenType::IDENTIFIER}))
    {
        return new t_VariableExpr(Previous().lexeme);
    }

    if (Match({t_TokenType::LEFT_PAREN}))
    {
        t_Expr *expr = Expression();
        Consume
        (
            t_TokenType::RIGHT_PAREN, 
            "Expect ')' after expression."
        );
        return new t_GroupingExpr(std::unique_ptr<t_Expr>(expr));
    }

    throw std::runtime_error
    (
        "Expect expression at line " + std::to_string(Peek().line)
    );
}
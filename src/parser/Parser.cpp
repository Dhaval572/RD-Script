#include "../include/Parser.h"
#include "AST.h"
#include "ErrorHandling.h"
#include <iostream>

// Initialize static memory pools with sizes large enough for all node types
namespace 
{
    static size_t MaxStmtSize()
    {
        size_t sizes[] = 
        {
            sizeof(t_BlockStmt),
            sizeof(t_IfStmt),
            sizeof(t_ForStmt),
            sizeof(t_BreakStmt),
            sizeof(t_ContinueStmt),
            sizeof(t_VarStmt),
            sizeof(t_DisplayStmt),
            sizeof(t_BenchmarkStmt),
            sizeof(t_EmptyStmt),
            sizeof(t_ExpressionStmt)
        };
        size_t max_size = 0;
        for (size_t s : sizes) if (s > max_size) max_size = s;
        return max_size;
    }

    static size_t MaxExprSize()
    {
        size_t sizes[] = 
        {
            sizeof(t_BinaryExpr),
            sizeof(t_LiteralExpr),
            sizeof(t_UnaryExpr),
            sizeof(t_GroupingExpr),
            sizeof(t_VariableExpr),
            sizeof(t_PrefixExpr),
            sizeof(t_PostfixExpr)
        };
        size_t max_size = 0;
        for (size_t s : sizes) if (s > max_size) max_size = s;
        return max_size;
    }
}

// Larger block size for statements and expressions to prevent overflow
t_MemoryPool t_Parser::stmt_pool(MaxStmtSize());
t_MemoryPool t_Parser::expr_pool(MaxExprSize());

t_Parser::t_Parser(const std::vector<t_Token> &tokens)
    : tokens(tokens), current(0) {}

t_Expected<std::vector<t_Stmt*>, t_ErrorInfo> t_Parser::Parse()
{
    std::vector<t_Stmt*> statements;
    while (!IsAtEnd())
    {
        t_Expected<t_Stmt*, t_ErrorInfo> result = Statement();
        if (!result.HasValue())
        {
            return t_Expected<std::vector<t_Stmt*>, t_ErrorInfo>(result.Error());
        }
        statements.push_back(result.Value());
    }

    return t_Expected<std::vector<t_Stmt*>, t_ErrorInfo>(statements);
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

t_Expected<t_Token, t_ErrorInfo> t_Parser::Consume
(
    t_TokenType type, const std::string &message
)
{
    if (Check(type))
    {
        return t_Expected<t_Token, t_ErrorInfo>(Advance());
    }

    return t_Expected<t_Token, t_ErrorInfo>
    (
        t_ErrorInfo(t_ErrorType::PARSING_ERROR, message, Peek().line, 0)
    );
}

t_ErrorInfo t_Parser::Error(t_Token token, const std::string &message)
{
    // In a real implementation, we would report the error
    std::cerr << "Error: " 
              << message 
              << " at line " 
              << token.line 
              << std::endl;
    return t_ErrorInfo(t_ErrorType::PARSING_ERROR, message, token.line, 0);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::Statement()
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

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BlockStatement()
{
    std::vector<std::unique_ptr<t_Stmt>> statements;

    while (!Check(t_TokenType::RIGHT_BRACE) && !IsAtEnd())
    {
        t_Expected<t_Stmt*, t_ErrorInfo> result = Statement();
        if (!result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
        }
        statements.push_back(std::unique_ptr<t_Stmt>(result.Value()));
    }

    t_Expected<t_Token, t_ErrorInfo> consume_result = 
    Consume(t_TokenType::RIGHT_BRACE, "Expect '}' after block.");
    if (!consume_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(consume_result.Error());
    }
    
    t_BlockStmt* stmt = static_cast<t_BlockStmt*>(stmt_pool.Allocate());
    new (stmt)t_BlockStmt(std::move(statements));
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
} 

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BreakStatement()
{
    t_Token keyword = Previous();
    t_Expected<t_Token, t_ErrorInfo> result = 
    Consume(t_TokenType::SEMICOLON, "Expect ';' after 'break'.");
    if (!result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
    }
    
    t_BreakStmt* stmt = static_cast<t_BreakStmt*>(stmt_pool.Allocate());
    new (stmt) t_BreakStmt(keyword);
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::ContinueStatement()
{
    t_Token keyword = Previous();
    t_Expected<t_Token, t_ErrorInfo> result = 
    Consume(t_TokenType::SEMICOLON, "Expect ';' after 'continue'.");
    if (!result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
    }
    
    t_ContinueStmt* stmt = static_cast<t_ContinueStmt*>(stmt_pool.Allocate());
    new (stmt) t_ContinueStmt(keyword);
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::IfStatement()
{
    t_Expected<t_Token, t_ErrorInfo> paren_result = 
    Consume(t_TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    if (!paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(paren_result.Error());
    }
    
    t_Expected<t_Expr*, t_ErrorInfo> condition_result = Expression();
    if (!condition_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(condition_result.Error());
    }
    t_Expr *condition = condition_result.Value();
    
    t_Expected<t_Token, t_ErrorInfo> close_paren_result = 
    Consume(t_TokenType::RIGHT_PAREN, "Expect ')' after if condition.");
    if (!close_paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(close_paren_result.Error());
    }

    // Parse the then branch
    t_Expected<t_Stmt*, t_ErrorInfo> then_result = Statement();
    if (!then_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(then_result.Error());
    }
    t_Stmt *then_branch = then_result.Value();

    // Check for else branch
    std::unique_ptr<t_Stmt> else_branch = nullptr;
    if (Match({t_TokenType::ELSE}))
    {
        t_Expected<t_Stmt*, t_ErrorInfo> else_result = Statement();
        if (!else_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(else_result.Error());
        }
        else_branch = std::unique_ptr<t_Stmt>(else_result.Value());
    }

    t_IfStmt* stmt = static_cast<t_IfStmt*>(stmt_pool.Allocate());
    new (stmt) t_IfStmt
    (
        std::unique_ptr<t_Expr>(condition), 
        std::unique_ptr<t_Stmt>(then_branch), 
        std::move(else_branch)
    );
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::ForStatement()
{
    t_Expected<t_Token, t_ErrorInfo> paren_result = 
    Consume(t_TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
    if (!paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(paren_result.Error());
    }

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
        t_Expected<t_Stmt*, t_ErrorInfo> var_result = VarDeclaration();
        if (!var_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(var_result.Error());
        }
        initializer = std::unique_ptr<t_Stmt>(var_result.Value());
        // Note: VarDeclaration already consumes the semicolon
        // So we don't need to consume it here
    }
    else
    {
        // Expression statement
        t_Expected<t_Stmt*, t_ErrorInfo> expr_result = ExpressionStatement();
        if (!expr_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(expr_result.Error());
        }
        initializer = std::unique_ptr<t_Stmt>(expr_result.Value());
    }

    // Parse condition
    std::unique_ptr<t_Expr> condition;
    if (!Check(t_TokenType::SEMICOLON))
    {
        t_Expected<t_Expr*, t_ErrorInfo> condition_result = Expression();
        if (!condition_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(condition_result.Error());
        }
        condition = std::unique_ptr<t_Expr>(condition_result.Value());
    }
    
    t_Expected<t_Token, t_ErrorInfo> semicolon_result = Consume(t_TokenType::SEMICOLON, "Expect ';' after loop condition.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }

    // Parse increment
    std::unique_ptr<t_Expr> increment;
    if (!Check(t_TokenType::RIGHT_PAREN))
    {
        t_Expected<t_Expr*, t_ErrorInfo> increment_result = Expression();
        if (!increment_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(increment_result.Error());
        }
        increment = std::unique_ptr<t_Expr>(increment_result.Value());
    }
    
    t_Expected<t_Token, t_ErrorInfo> close_paren_result = 
    Consume(t_TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");
    if (!close_paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(close_paren_result.Error());
    }

    // Parse body
    t_Expected<t_Stmt*, t_ErrorInfo> body_result = Statement();
    if (!body_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(body_result.Error());
    }
    t_Stmt *body = body_result.Value();

    t_ForStmt* stmt = static_cast<t_ForStmt*>(stmt_pool.Allocate());
    new (stmt) t_ForStmt
    (
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        std::unique_ptr<t_Stmt>(body)
    );
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::VarDeclaration()
{
    t_Expected<t_Token, t_ErrorInfo> name_result = 
    Consume(t_TokenType::IDENTIFIER, "Expect variable name.");
    if (!name_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(name_result.Error());
    }
    t_Token name = name_result.Value();

    std::unique_ptr<t_Expr> initializer = nullptr;
    if (Match({t_TokenType::EQUAL}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> init_result = Expression();
        if (!init_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(init_result.Error());
        }
        initializer = std::unique_ptr<t_Expr>(init_result.Value());
    }

    t_Expected<t_Token, t_ErrorInfo> semicolon_result = 
    Consume(t_TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }
    
    t_VarStmt* stmt = static_cast<t_VarStmt*>(stmt_pool.Allocate());
    new (stmt) t_VarStmt(name.lexeme, std::move(initializer));
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::DisplayStatement()
{
    std::vector<std::unique_ptr<t_Expr>> values;

    // Parse the first expression
    t_Expected<t_Expr*, t_ErrorInfo> first_expr_result = Expression();
    if (!first_expr_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(first_expr_result.Error());
    }
    values.push_back(std::unique_ptr<t_Expr>(first_expr_result.Value()));

    // Parse additional comma-separated expressions
    while (Match({t_TokenType::COMMA}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> expr_result = Expression();
        if (!expr_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(expr_result.Error());
        }
        values.push_back(std::unique_ptr<t_Expr>(expr_result.Value()));
    }

    t_Expected<t_Token, t_ErrorInfo> semicolon_result = 
    Consume(t_TokenType::SEMICOLON, "Expect ';' after value.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }

    // Use the new t_DisplayStmt instead of t_PrintStmt
    t_DisplayStmt* stmt = static_cast<t_DisplayStmt*>(stmt_pool.Allocate());
    new (stmt) t_DisplayStmt(std::move(values));
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::ExpressionStatement()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = Expression();
    if (!expr_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(expr_result.Error());
    }
    t_Expr *expr = expr_result.Value();
    
    t_Expected<t_Token, t_ErrorInfo> semicolon_result = 
    Consume(t_TokenType::SEMICOLON, "Expect ';' after expression.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }
    
    t_ExpressionStmt* stmt = 
    static_cast<t_ExpressionStmt*>(stmt_pool.Allocate());
    new (stmt) t_ExpressionStmt(std::unique_ptr<t_Expr>(expr));
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::EmptyStatement()
{
    t_Token semicolon = Previous();
    t_EmptyStmt* stmt = static_cast<t_EmptyStmt*>(stmt_pool.Allocate());
    new (stmt) t_EmptyStmt(semicolon);
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BenchmarkStatement()
{
    t_Expected<t_Token, t_ErrorInfo> open_brace_result = 
    Consume(t_TokenType::LEFT_BRACE, "Expect '{' after 'benchmark'.");
    if (!open_brace_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(open_brace_result.Error());
    }
    
    // Parse the benchmark body
    std::vector<std::unique_ptr<t_Stmt>> statements;
    while (!Check(t_TokenType::RIGHT_BRACE) && !IsAtEnd())
    {
        t_Expected<t_Stmt*, t_ErrorInfo> stmt_result = Statement();
        if (!stmt_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(stmt_result.Error());
        }
        statements.push_back(std::unique_ptr<t_Stmt>(stmt_result.Value()));
    }
    
    t_Expected<t_Token, t_ErrorInfo> close_brace_result = 
    Consume(t_TokenType::RIGHT_BRACE, "Expect '}' after benchmark body.");
    if (!close_brace_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(close_brace_result.Error());
    }
    
    // Create a block statement for the body
    t_Stmt *body = new t_BlockStmt(std::move(statements));
    
    t_BenchmarkStmt* stmt = static_cast<t_BenchmarkStmt*>(stmt_pool.Allocate());
    new (stmt) t_BenchmarkStmt(std::unique_ptr<t_Stmt>(body));
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Expression()
{
    return Assignment();
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Assignment()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = Or();
    if (!expr_result.HasValue())
    {
        return expr_result;
    }
    t_Expr *expr = expr_result.Value();

    if (Match({t_TokenType::EQUAL, t_TokenType::PLUS_EQUAL, t_TokenType::MINUS_EQUAL, 
               t_TokenType::STAR_EQUAL, t_TokenType::SLASH_EQUAL}))
    {
        t_Token equals = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> value_result = Assignment();
        if (!value_result.HasValue())
        {
            return value_result;
        }
        t_Expr *value = value_result.Value();

        if (t_VariableExpr *var_expr = dynamic_cast<t_VariableExpr *>(expr))
        {
            std::string name = var_expr->name;
            t_BinaryExpr* expr_node = 
            static_cast<t_BinaryExpr*>(expr_pool.Allocate());

            new (expr_node) t_BinaryExpr
            (
                std::unique_ptr<t_Expr>(expr), 
                equals, 
                std::unique_ptr<t_Expr>(value)
            );
            return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
        }

        // If we get here, we're trying to assign to a non-variable
        return t_Expected<t_Expr*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                t_ErrorType::PARSING_ERROR, 
                "Invalid assignment target", 
                equals.line, 
                0
            )
        );
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Or()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = And();
    if (!expr_result.HasValue())
    {
        return expr_result;
    }
    t_Expr *expr = expr_result.Value();

    while (Match({t_TokenType::OR}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = And();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::And()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = Equality();
    if (!expr_result.HasValue())
    {
        return expr_result;
    }
    t_Expr *expr = expr_result.Value();

    while (Match({t_TokenType::AND}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Equality();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Equality()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = Comparison();
    if (!expr_result.HasValue())
    {
        return expr_result;
    }
    t_Expr *expr = expr_result.Value();

    while (Match({t_TokenType::BANG_EQUAL, t_TokenType::EQUAL_EQUAL}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Comparison();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Comparison()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = Term();
    if (!expr_result.HasValue())
    {
        return expr_result;
    }
    t_Expr *expr = expr_result.Value();
    
    while 
    (
        Match
        (
            {
                t_TokenType::GREATER, t_TokenType::GREATER_EQUAL, t_TokenType::LESS, t_TokenType::LESS_EQUAL
            }
        )
    )
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Term();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Term()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = Factor();
    if (!expr_result.HasValue())
    {
        return expr_result;
    }
    t_Expr *expr = expr_result.Value();

    while (Match({t_TokenType::MINUS, t_TokenType::PLUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Factor();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Factor()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = Unary();
    if (!expr_result.HasValue())
    {
        return expr_result;
    }
    t_Expr *expr = expr_result.Value();

    while (Match({t_TokenType::SLASH, t_TokenType::STAR}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Unary();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_BinaryExpr
        (
            std::unique_ptr<t_Expr>(expr), 
            op, 
            std::unique_ptr<t_Expr>(right)
        );
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Unary()
{
    if (Match({t_TokenType::BANG, t_TokenType::MINUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Unary();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_UnaryExpr* expr_node = static_cast<t_UnaryExpr*>(expr_pool.Allocate());
        new (expr_node) t_UnaryExpr(op, std::unique_ptr<t_Expr>(right));
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    return FinishUnary();
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::FinishUnary()
{
    t_Expected<t_Expr*, t_ErrorInfo> expr_result = Primary();
    if (!expr_result.HasValue())
    {
        return expr_result;
    }
    t_Expr *expr = expr_result.Value();

    // Handle postfix increment/decrement
    while (Match({t_TokenType::PLUS_PLUS, t_TokenType::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_PostfixExpr* expr_node = 
        static_cast<t_PostfixExpr*>(expr_pool.Allocate());
        new (expr_node) t_PostfixExpr(std::unique_ptr<t_Expr>(expr), op);
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Primary()
{
    if (Match({t_TokenType::FALSE}))
    {
        t_LiteralExpr* expr_node = 
        static_cast<t_LiteralExpr*>(expr_pool.Allocate());
        new (expr_node) t_LiteralExpr("false", t_TokenType::FALSE);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({t_TokenType::TRUE}))
    {
        t_LiteralExpr* expr_node = 
        static_cast<t_LiteralExpr*>(expr_pool.Allocate());
        new (expr_node) t_LiteralExpr("true", t_TokenType::TRUE);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({t_TokenType::NIL}))
    {
        t_LiteralExpr* expr_node = 
        static_cast<t_LiteralExpr*>(expr_pool.Allocate());
        new (expr_node) t_LiteralExpr("nil", t_TokenType::NIL);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if 
    (
        Match
        (
            {
                t_TokenType::NUMBER, 
                t_TokenType::STRING, 
                t_TokenType::FORMAT_STRING
            }
        )
    )
    {
        t_Token previous = Previous();
        t_LiteralExpr* expr_node = 
        static_cast<t_LiteralExpr*>(expr_pool.Allocate());
        new (expr_node) t_LiteralExpr(previous.literal, previous.type);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    // Handle prefix increment/decrement
    if (Match({t_TokenType::PLUS_PLUS, t_TokenType::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> operand_result = Primary();
        if (!operand_result.HasValue())
        {
            return operand_result;
        }
        t_Expr *operand = operand_result.Value();
        
        t_PrefixExpr* expr_node = 
        static_cast<t_PrefixExpr*>(expr_pool.Allocate());
        new (expr_node) t_PrefixExpr(op, std::unique_ptr<t_Expr>(operand));
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({t_TokenType::IDENTIFIER}))
    {
        t_VariableExpr* expr_node = 
        static_cast<t_VariableExpr*>(expr_pool.Allocate());
        new (expr_node) t_VariableExpr(Previous().lexeme);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({t_TokenType::LEFT_PAREN}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> expr_result = Expression();
        if (!expr_result.HasValue())
        {
            return expr_result;
        }
        t_Expr *expr = expr_result.Value();
        
        t_Expected<t_Token, t_ErrorInfo> paren_result = 
        Consume(t_TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        if (!paren_result.HasValue())
        {
            return t_Expected<t_Expr*, t_ErrorInfo>(paren_result.Error());
        }
        
        t_GroupingExpr* expr_node = 
        static_cast<t_GroupingExpr*>(expr_pool.Allocate());
        new (expr_node) t_GroupingExpr(std::unique_ptr<t_Expr>(expr));
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    return t_Expected<t_Expr*, t_ErrorInfo>
    (
        t_ErrorInfo
        (
            t_ErrorType::PARSING_ERROR, 
            "Expect expression", 
            Peek().line, 
            0
        )
    );
}
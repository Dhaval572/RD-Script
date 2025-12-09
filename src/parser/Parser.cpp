#include "../include/parser.h"
#include "../include/AST.h"
#include "../include/ErrorHandling.h"
#include "../include/ASTContext.h"  // Include ASTContext
#include <iostream>

// Create a static instance of ASTContext to manage the lifecycle
static t_ASTContext ast_context;

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

// Reset the memory pools (now delegated to ASTContext)
void t_Parser::ResetPools()
{
    t_ASTContext::Reset();
}

bool t_Parser::IsAtEnd()
{
    return Peek().type == e_TOKEN_TYPE::EOF_TOKEN;
}

t_Token t_Parser::Advance()
{
    if (!IsAtEnd()) current++;
    return Previous();
}

bool t_Parser::Check(e_TOKEN_TYPE type)
{
    if (IsAtEnd()) return false;
    return Peek().type == type;
}

t_Token t_Parser::Peek()
{
    if (current >= tokens.size())
    {
        // Return EOF token if we're past the end of the tokens vector
        return t_Token(e_TOKEN_TYPE::EOF_TOKEN, "", "", 0);
    }
    return tokens[current];
}

t_Token t_Parser::Previous()
{
    if (current <= 0)
    {
        // Return EOF token if we're at the beginning or before
        return t_Token(e_TOKEN_TYPE::EOF_TOKEN, "", "", 0);
    }
    return tokens[current - 1];
}

bool t_Parser::Match(std::initializer_list<e_TOKEN_TYPE> types)
{
    for (e_TOKEN_TYPE type : types)
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
    e_TOKEN_TYPE type, const std::string &message
)
{
    if (Check(type))
    {
        return t_Expected<t_Token, t_ErrorInfo>(Advance());
    }

    return t_Expected<t_Token, t_ErrorInfo>
    (
        t_ErrorInfo(e_ERROR_TYPE::PARSING_ERROR, message, Peek().line, 0)
    );
}

t_ErrorInfo t_Parser::Error(t_Token token, const std::string &message)
{
    std::cerr << "Error: " 
              << message 
              << " at line " 
              << token.line 
              << std::endl;

    return t_ErrorInfo(e_ERROR_TYPE::PARSING_ERROR, message, token.line, 0);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::Statement()
{
    if (Match({e_TOKEN_TYPE::LEFT_BRACE}))
    {
        return BlockStatement();
    }

    if (Match({e_TOKEN_TYPE::IF}))
    {
        return IfStatement();
    }

    if (Match({e_TOKEN_TYPE::FOR}))
    {
        return ForStatement();
    }

    if (Match({e_TOKEN_TYPE::BREAK}))
    {
        return BreakStatement();
    }

    if (Match({e_TOKEN_TYPE::CONTINUE}))
    {
        return ContinueStatement();
    }

    if (Match({e_TOKEN_TYPE::AUTO}))
    {
        return VarDeclaration();
    }

    if (Match({e_TOKEN_TYPE::DISPLAY}))
    {
        return DisplayStatement();
    }

    if (Match({e_TOKEN_TYPE::BENCHMARK}))
    {
        return BenchmarkStatement();
    }

    if (Match({e_TOKEN_TYPE::SEMICOLON}))
    {
        return EmptyStatement();
    }

    return ExpressionStatement();
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BlockStatement()
{
    std::vector<std::unique_ptr<t_Stmt>> statements;

    while (!Check(e_TOKEN_TYPE::RIGHT_BRACE) && !IsAtEnd())
    {
        t_Expected<t_Stmt*, t_ErrorInfo> result = Statement();
        if (!result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
        }
        statements.push_back(std::unique_ptr<t_Stmt>(result.Value()));
    }

    t_Expected<t_Token, t_ErrorInfo> consume_result = 
    Consume(e_TOKEN_TYPE::RIGHT_BRACE, "Expect '}' after block.");
    if (!consume_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(consume_result.Error());
    }
    
    t_BlockStmt* stmt = static_cast<t_BlockStmt*>
    (
        t_ASTContext::GetStmtPool().Allocate()
    );
    new (stmt)t_BlockStmt(std::move(statements));
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
} 

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BreakStatement()
{
    t_Token keyword = Previous();
    t_Expected<t_Token, t_ErrorInfo> result = 
    Consume(e_TOKEN_TYPE::SEMICOLON, "Expect ';' after 'break'.");
    if (!result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
    }
    
    t_BreakStmt* stmt = static_cast<t_BreakStmt*>
    (
        t_ASTContext::GetStmtPool().Allocate()
    );
    new (stmt) t_BreakStmt(keyword);
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::ContinueStatement()
{
    t_Token keyword = Previous();
    t_Expected<t_Token, t_ErrorInfo> result = 
    Consume(e_TOKEN_TYPE::SEMICOLON, "Expect ';' after 'continue'.");
    if (!result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
    }
    
    t_ContinueStmt* stmt = static_cast<t_ContinueStmt*>
    (
        t_ASTContext::GetStmtPool().Allocate()
    );
    new (stmt) t_ContinueStmt(keyword);
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::IfStatement()
{
    t_Expected<t_Token, t_ErrorInfo> paren_result = 
    Consume(e_TOKEN_TYPE::LEFT_PAREN, "Expect '(' after 'if'.");
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
    Consume(e_TOKEN_TYPE::RIGHT_PAREN, "Expect ')' after if condition.");
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
    if (Match({e_TOKEN_TYPE::ELSE}))
    {
        t_Expected<t_Stmt*, t_ErrorInfo> else_result = Statement();
        if (!else_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(else_result.Error());
        }
        else_branch = std::unique_ptr<t_Stmt>(else_result.Value());
    }

    t_IfStmt* stmt = static_cast<t_IfStmt*>
    (
        t_ASTContext::GetStmtPool().Allocate()
    );
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
    Consume(e_TOKEN_TYPE::LEFT_PAREN, "Expect '(' after 'for'.");
    if (!paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(paren_result.Error());
    }

    // Parse initializer (can be a variable declaration or an expression statement)
    std::unique_ptr<t_Stmt> initializer;
    if (Match({e_TOKEN_TYPE::SEMICOLON}))
    {
        // No initializer
        initializer = nullptr;
    }
    else if (Match({e_TOKEN_TYPE::AUTO}))
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
    if (!Check(e_TOKEN_TYPE::SEMICOLON))
    {
        t_Expected<t_Expr*, t_ErrorInfo> condition_result = Expression();
        if (!condition_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(condition_result.Error());
        }
        condition = std::unique_ptr<t_Expr>(condition_result.Value());
    }
    
    t_Expected<t_Token, t_ErrorInfo> semicolon_result = Consume(e_TOKEN_TYPE::SEMICOLON, "Expect ';' after loop condition.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }

    // Parse increment
    std::unique_ptr<t_Expr> increment;
    if (!Check(e_TOKEN_TYPE::RIGHT_PAREN))
    {
        t_Expected<t_Expr*, t_ErrorInfo> increment_result = Expression();
        if (!increment_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(increment_result.Error());
        }
        increment = std::unique_ptr<t_Expr>(increment_result.Value());
    }
    
    t_Expected<t_Token, t_ErrorInfo> close_paren_result = 
    Consume(e_TOKEN_TYPE::RIGHT_PAREN, "Expect ')' after for clauses.");
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

    t_ForStmt* stmt = static_cast<t_ForStmt*>
    (
        t_ASTContext::GetStmtPool().Allocate()
    );
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
    Consume(e_TOKEN_TYPE::IDENTIFIER, "Expect variable name.");
    if (!name_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(name_result.Error());
    }
    t_Token name = name_result.Value();

    std::unique_ptr<t_Expr> initializer = nullptr;
    if (Match({e_TOKEN_TYPE::EQUAL}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> init_result = Expression();
        if (!init_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(init_result.Error());
        }
        initializer = std::unique_ptr<t_Expr>(init_result.Value());
    }

    t_Expected<t_Token, t_ErrorInfo> semicolon_result = 
    Consume(e_TOKEN_TYPE::SEMICOLON, "Expect ';' after variable declaration.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }
    
    t_VarStmt* stmt = static_cast<t_VarStmt*>
    (
        t_ASTContext::GetStmtPool().Allocate()
    );
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
    while (Match({e_TOKEN_TYPE::COMMA}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> expr_result = Expression();
        if (!expr_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(expr_result.Error());
        }
        values.push_back(std::unique_ptr<t_Expr>(expr_result.Value()));
    }

    t_Expected<t_Token, t_ErrorInfo> semicolon_result = 
    Consume(e_TOKEN_TYPE::SEMICOLON, "Expect ';' after value.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }

    // Use the new t_DisplayStmt instead of t_PrintStmt
    t_DisplayStmt* stmt = static_cast<t_DisplayStmt*>
    (
        t_ASTContext::GetStmtPool().Allocate()
    );
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
    Consume(e_TOKEN_TYPE::SEMICOLON, "Expect ';' after expression.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }
    
    t_ExpressionStmt* stmt = 
    static_cast<t_ExpressionStmt*>(t_ASTContext::GetStmtPool().Allocate());
    new (stmt) t_ExpressionStmt(std::unique_ptr<t_Expr>(expr));
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::EmptyStatement()
{
    t_Token semicolon = Previous();
    t_EmptyStmt* stmt = static_cast<t_EmptyStmt*>
    (
        t_ASTContext::GetStmtPool().Allocate()
    );
    new (stmt) t_EmptyStmt(semicolon);
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BenchmarkStatement()
{
    t_Expected<t_Token, t_ErrorInfo> open_brace_result = 
    Consume(e_TOKEN_TYPE::LEFT_BRACE, "Expect '{' after 'benchmark'.");
    if (!open_brace_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(open_brace_result.Error());
    }
    
    // Parse the benchmark body
    std::vector<std::unique_ptr<t_Stmt>> statements;
    while (!Check(e_TOKEN_TYPE::RIGHT_BRACE) && !IsAtEnd())
    {
        t_Expected<t_Stmt*, t_ErrorInfo> stmt_result = Statement();
        if (!stmt_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(stmt_result.Error());
        }
        statements.push_back(std::unique_ptr<t_Stmt>(stmt_result.Value()));
    }
    
    t_Expected<t_Token, t_ErrorInfo> close_brace_result = 
    Consume(e_TOKEN_TYPE::RIGHT_BRACE, "Expect '}' after benchmark body.");
    if (!close_brace_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(close_brace_result.Error());
    }
    
    // Create a block statement for the body
    t_Stmt *body = new t_BlockStmt(std::move(statements));
    
    t_BenchmarkStmt* stmt = static_cast<t_BenchmarkStmt*>(t_ASTContext::GetStmtPool().Allocate());
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

    if 
    (
        Match
        (
            {
                e_TOKEN_TYPE::EQUAL, 
                e_TOKEN_TYPE::PLUS_EQUAL, 
                e_TOKEN_TYPE::MINUS_EQUAL, 
                e_TOKEN_TYPE::STAR_EQUAL, 
                e_TOKEN_TYPE::SLASH_EQUAL
            }
        )
    )
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
            static_cast<t_BinaryExpr*>(t_ASTContext::GetExprPool().Allocate());

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
                e_ERROR_TYPE::PARSING_ERROR, 
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

    while (Match({e_TOKEN_TYPE::OR}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = And();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(t_ASTContext::GetExprPool().Allocate());
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

    while (Match({e_TOKEN_TYPE::AND}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Equality();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(t_ASTContext::GetExprPool().Allocate());
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

    while (Match({e_TOKEN_TYPE::BANG_EQUAL, e_TOKEN_TYPE::EQUAL_EQUAL}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Comparison();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(t_ASTContext::GetExprPool().Allocate());
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
                e_TOKEN_TYPE::GREATER, 
                e_TOKEN_TYPE::GREATER_EQUAL, 
                e_TOKEN_TYPE::LESS, 
                e_TOKEN_TYPE::LESS_EQUAL
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
        static_cast<t_BinaryExpr*>(t_ASTContext::GetExprPool().Allocate());
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

    while (Match({e_TOKEN_TYPE::MINUS, e_TOKEN_TYPE::PLUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Factor();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(t_ASTContext::GetExprPool().Allocate());
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

    while (Match({e_TOKEN_TYPE::SLASH, e_TOKEN_TYPE::STAR}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Unary();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = 
        static_cast<t_BinaryExpr*>(t_ASTContext::GetExprPool().Allocate());
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
    if (Match({e_TOKEN_TYPE::BANG, e_TOKEN_TYPE::MINUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Unary();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_UnaryExpr* expr_node = static_cast<t_UnaryExpr*>(t_ASTContext::GetExprPool().Allocate());
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
    while (Match({e_TOKEN_TYPE::PLUS_PLUS, e_TOKEN_TYPE::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_PostfixExpr* expr_node = 
        static_cast<t_PostfixExpr*>(t_ASTContext::GetExprPool().Allocate());
        new (expr_node) t_PostfixExpr(std::unique_ptr<t_Expr>(expr), op);
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Primary()
{
    if (Match({e_TOKEN_TYPE::FALSE}))
    {
        t_LiteralExpr* expr_node = 
        static_cast<t_LiteralExpr*>(t_ASTContext::GetExprPool().Allocate());
        new (expr_node) t_LiteralExpr("false", e_TOKEN_TYPE::FALSE);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({e_TOKEN_TYPE::TRUE}))
    {
        t_LiteralExpr* expr_node = 
        static_cast<t_LiteralExpr*>(t_ASTContext::GetExprPool().Allocate());
        new (expr_node) t_LiteralExpr("true", e_TOKEN_TYPE::TRUE);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({e_TOKEN_TYPE::NIL}))
    {
        t_LiteralExpr* expr_node = 
        static_cast<t_LiteralExpr*>(t_ASTContext::GetExprPool().Allocate());
        new (expr_node) t_LiteralExpr("nil", e_TOKEN_TYPE::NIL);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if 
    (
        Match
        (
            {
                e_TOKEN_TYPE::NUMBER, 
                e_TOKEN_TYPE::STRING, 
                e_TOKEN_TYPE::FORMAT_STRING
            }
        )
    )
    {
        t_Token previous = Previous();
        t_LiteralExpr* expr_node = 
        static_cast<t_LiteralExpr*>(t_ASTContext::GetExprPool().Allocate());
        new (expr_node) t_LiteralExpr(previous.literal, previous.type);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    // Handle prefix increment/decrement
    if (Match({e_TOKEN_TYPE::PLUS_PLUS, e_TOKEN_TYPE::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> operand_result = Primary();
        if (!operand_result.HasValue())
        {
            return operand_result;
        }
        t_Expr *operand = operand_result.Value();
        
        t_PrefixExpr* expr_node = 
        static_cast<t_PrefixExpr*>(t_ASTContext::GetExprPool().Allocate());
        new (expr_node) t_PrefixExpr(op, std::unique_ptr<t_Expr>(operand));
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({e_TOKEN_TYPE::IDENTIFIER}))
    {
        t_VariableExpr* expr_node = 
        static_cast<t_VariableExpr*>(t_ASTContext::GetExprPool().Allocate());
        new (expr_node) t_VariableExpr(Previous().lexeme);
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({e_TOKEN_TYPE::LEFT_PAREN}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> expr_result = Expression();
        if (!expr_result.HasValue())
        {
            return expr_result;
        }
        t_Expr *expr = expr_result.Value();
        
        t_Expected<t_Token, t_ErrorInfo> paren_result = 
        Consume(e_TOKEN_TYPE::RIGHT_PAREN, "Expect ')' after expression.");
        if (!paren_result.HasValue())
        {
            return t_Expected<t_Expr*, t_ErrorInfo>(paren_result.Error());
        }
        
        t_GroupingExpr* expr_node = 
        static_cast<t_GroupingExpr*>(t_ASTContext::GetExprPool().Allocate());
        new (expr_node) t_GroupingExpr(std::unique_ptr<t_Expr>(expr));
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    return t_Expected<t_Expr*, t_ErrorInfo>
    (
        t_ErrorInfo
        (
            e_ERROR_TYPE::PARSING_ERROR, 
            "Expect expression", 
            Peek().line, 
            0
        )
    );
}
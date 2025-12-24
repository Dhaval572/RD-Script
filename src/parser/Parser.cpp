#include <rubberduck/Parser.h>
#include <rubberduck/AST.h>
#include <rubberduck/ErrorHandling.h>
#include <rubberduck/ASTContext.h> 
#include <iostream>
#include <string>
#include <cctype>
#include <cmath>

namespace
{
    bool IsIntegerLiteralValue(const std::string &value)
    {
        if (value.empty())
        {
            return false;
        }

        size_t start = 0;
        if (value[0] == '-')
        {
            if (value.size() == 1)
            {
                return false;
            }
            start = 1;
        }

        for (size_t i = start; i < value.size(); i++)
        {
            if (!std::isdigit(static_cast<unsigned char>(value[i])))
            {
                return false;
            }
        }

        return true;
    }

    bool TryGetNumberLiteral(t_Expr *expr, double &out_value)
    {
        t_LiteralExpr *literal = dynamic_cast<t_LiteralExpr *>(expr);
        if (!literal || literal->token_type != e_TokenType::NUMBER)
        {
            return false;
        }

        try
        {
            out_value = std::stod(literal->value);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    t_LiteralExpr *MakeNumberLiteral(double value, t_ASTContext& context)
    {
        std::string text = std::to_string(value);
        text.erase(text.find_last_not_of('0') + 1, std::string::npos);
        text.erase(text.find_last_not_of('.') + 1, std::string::npos);

        return context.CreateExpr<t_LiteralExpr>
        (
            text, 
            e_TokenType::NUMBER
        );
    }
}

t_Parser::t_Parser
(
    const std::vector<t_Token> &tokens, 
    t_ASTContext& context
)
    : m_Tokens(tokens), m_Current(0), m_Context(context) {}

t_Expected<std::vector<t_PoolPtr<t_Stmt>>, t_ErrorInfo> t_Parser::Parse()
{
    std::vector<t_PoolPtr<t_Stmt>> statements;
    while (!IsAtEnd())
    {
        t_Expected<t_Stmt*, t_ErrorInfo> result = Statement();
        if (!result.HasValue())
        {
            return t_Expected<std::vector<t_PoolPtr<t_Stmt>>, t_ErrorInfo>
            (
                result.Error()
            );
        }
        statements.push_back(t_PoolPtr<t_Stmt>(result.Value()));
    }

    return t_Expected<std::vector<t_PoolPtr<t_Stmt>>, t_ErrorInfo>
    (
        std::move(statements)
    );
}


bool t_Parser::IsAtEnd()
{
    return Peek().type == e_TokenType::EOF_TOKEN;
}

t_Token t_Parser::Advance()
{
    if (!IsAtEnd()) m_Current++;
    return Previous();
}

bool t_Parser::Check(e_TokenType type)
{
    if (IsAtEnd()) return false;
    return Peek().type == type;
}

t_Token t_Parser::Peek()
{
    if (static_cast<size_t>(m_Current) >= m_Tokens.size())
    {
        // Return EOF token if we're past the end of the tokens vector
        return t_Token(e_TokenType::EOF_TOKEN, "", "", 0);
    }
    return m_Tokens[m_Current];
}

t_Token t_Parser::Previous()
{
    if (m_Current <= 0)
    {
        // Return EOF token if we're at the beginning or before
        return t_Token(e_TokenType::EOF_TOKEN, "", "", 0);
    }
    return m_Tokens[m_Current - 1];
}

bool t_Parser::Match(std::initializer_list<e_TokenType> types)
{
    for (e_TokenType type : types)
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
    e_TokenType type, const std::string &message
)
{
    if (Check(type))
    {
        return t_Expected<t_Token, t_ErrorInfo>(Advance());
    }

    return t_Expected<t_Token, t_ErrorInfo>
    (
        t_ErrorInfo(e_ErrorType::PARSING_ERROR, message, Peek().line, 0)
    );
}

t_ErrorInfo t_Parser::Error(t_Token token, const std::string &message)
{
    std::cerr << "Error: " 
              << message 
              << " at line " 
              << token.line 
              << std::endl;

    return t_ErrorInfo(e_ErrorType::PARSING_ERROR, message, token.line, 0);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::Statement()
{
    if (Match({e_TokenType::LEFT_BRACE}))
    {
        return BlockStatement();
    }

    if (Match({e_TokenType::FUN}))
    {
        return FunDeclaration();
    }

    if (Match({e_TokenType::IF}))
    {
        return IfStatement();
    }

    if (Match({e_TokenType::FOR}))
    {
        return ForStatement();
    }

    if (Match({e_TokenType::BREAK}))
    {
        return BreakStatement();
    }

    if (Match({e_TokenType::CONTINUE}))
    {
        return ContinueStatement();
    }

    if (Match({e_TokenType::AUTO}))
    {
        return VarDeclaration();
    }

    if (Match({e_TokenType::DISPLAY}))
    {
        return DisplayStatement();
    }

    if (Match({e_TokenType::BENCHMARK}))
    {
        return BenchmarkStatement();
    }

    if (Match({e_TokenType::GETIN}))
    {
        return GetinStatement();
    }

    if (Match({e_TokenType::RETURN}))
    {
        return ReturnStatement();
    }

    if (Match({e_TokenType::SEMICOLON}))
    {
        return EmptyStatement();
    }

    return ExpressionStatement();
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BlockStatement()
{
    std::vector<t_PoolPtr<t_Stmt>> statements;

    while (!Check(e_TokenType::RIGHT_BRACE) && !IsAtEnd())
    {
        t_Expected<t_Stmt*, t_ErrorInfo> result = Statement();
        if (!result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
        }
        statements.push_back(t_PoolPtr<t_Stmt>(result.Value()));
    }

    t_Expected<t_Token, t_ErrorInfo> consume_result = 
    Consume(e_TokenType::RIGHT_BRACE, "Expect '}' after block.");
    if (!consume_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(consume_result.Error());
    }
    
    t_BlockStmt* stmt = 
    m_Context.CreateStmt<t_BlockStmt>(std::move(statements));
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                Peek().line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
} 

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BreakStatement()
{
    t_Token keyword = Previous();
    t_Expected<t_Token, t_ErrorInfo> result = 
    Consume(e_TokenType::SEMICOLON, "Expect ';' after 'break'.");
    if (!result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
    }
    
    t_BreakStmt* stmt = m_Context.CreateStmt<t_BreakStmt>(keyword);
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                keyword.line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::ContinueStatement()
{
    t_Token keyword = Previous();
    t_Expected<t_Token, t_ErrorInfo> result = 
    Consume(e_TokenType::SEMICOLON, "Expect ';' after 'continue'.");
    if (!result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
    }
    
    t_ContinueStmt* stmt = m_Context.CreateStmt<t_ContinueStmt>(keyword);
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                keyword.line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::IfStatement()
{
    t_Expected<t_Token, t_ErrorInfo> paren_result = 
    Consume(e_TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
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
    Consume(e_TokenType::RIGHT_PAREN, "Expect ')' after if condition.");
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
    t_PoolPtr<t_Stmt> else_branch = nullptr;
    if (Match({e_TokenType::ELSE}))
    {
        t_Expected<t_Stmt*, t_ErrorInfo> else_result = Statement();
        if (!else_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(else_result.Error());
        }
        else_branch = t_PoolPtr<t_Stmt>(else_result.Value());
    }

    t_IfStmt* stmt = m_Context.CreateStmt<t_IfStmt>
    (
        t_PoolPtr<t_Expr>(condition), 
        t_PoolPtr<t_Stmt>(then_branch), 
        std::move(else_branch)
    );
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                Peek().line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::ForStatement()
{
    t_Expected<t_Token, t_ErrorInfo> paren_result = 
    Consume(e_TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
    if (!paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(paren_result.Error());
    }

    // Parse initializer (must be a variable declaration or empty)
    t_PoolPtr<t_Stmt> initializer;
    if (Match({e_TokenType::SEMICOLON}))
    {
        initializer = nullptr;
    }
    else if (Match({e_TokenType::AUTO}))
    {
        // Variable declaration
        t_Expected<t_Stmt*, t_ErrorInfo> var_result = VarDeclaration();
        if (!var_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(var_result.Error());
        }
        initializer = t_PoolPtr<t_Stmt>(var_result.Value());
        // Note: VarDeclaration already consumes the semicolon
        // So we don't need to consume it here
    }
    else
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            Error
            (
                Peek(),
                "Expect 'auto' variable declaration or ';' in for-loop initializer."
            )
        );
    }

    // Parse condition
    t_PoolPtr<t_Expr> condition;
    if (!Check(e_TokenType::SEMICOLON))
    {
        t_Expected<t_Expr*, t_ErrorInfo> condition_result = Expression();
        if (!condition_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                condition_result.Error()
            );
        }
        condition = t_PoolPtr<t_Expr>(condition_result.Value());
    }
    
    t_Expected<t_Token, t_ErrorInfo> semicolon_result = 
    Consume(e_TokenType::SEMICOLON, "Expect ';' after loop condition.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }

    // Parse increment
    t_PoolPtr<t_Expr> increment;
    if (!Check(e_TokenType::RIGHT_PAREN))
    {
        t_Expected<t_Expr*, t_ErrorInfo> increment_result = Expression();
        if (!increment_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                increment_result.Error()
            );
        }
        increment = t_PoolPtr<t_Expr>(increment_result.Value());
    }
    
    t_Expected<t_Token, t_ErrorInfo> close_paren_result = 
    Consume(e_TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");
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

    t_ForStmt* stmt = m_Context.CreateStmt<t_ForStmt>
    (
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        t_PoolPtr<t_Stmt>(body)
    );
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                Peek().line, 
                0
            )
        );
    }

    t_VarStmt *init_var = nullptr;
    if (stmt->initializer)
    {
        init_var = dynamic_cast<t_VarStmt *>(stmt->initializer.get());
        if (!init_var || !init_var->initializer)
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                Error
                (
                    Previous(),
                    "For-loop initializer must declare an int variable with an initializer."
                )
            );
        }

        t_LiteralExpr *init_literal =
        dynamic_cast<t_LiteralExpr *>(init_var->initializer.get());

        if 
        (
            !init_literal                                    || 
            init_literal->token_type != e_TokenType::NUMBER  ||
            !IsIntegerLiteralValue(init_literal->value)
        )
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                Error
                (
                    Previous(),
                    "For-loop variable must be initialized with an int literal"
                )
            );
        }

        if (!stmt->condition)
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                Error(Previous(), "For-loop condition is required.")
            );
        }

        t_BinaryExpr *condi = dynamic_cast<t_BinaryExpr *>
        (
            stmt->condition.get()
        );
        if (!condi)
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                Error(Previous(), "For-loop condition must be a comparison.")
            );
        }

        if 
        (
            condi->op.type != e_TokenType::LESS         &&
            condi->op.type != e_TokenType::LESS_EQUAL   &&
            condi->op.type != e_TokenType::GREATER      &&
            condi->op.type != e_TokenType::GREATER_EQUAL
        )
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                Error
                (
                    Previous(), "For-loop condition must be <, <=, >, or >=."
                )
            );
        }

        t_VariableExpr *cond_var = 
        dynamic_cast<t_VariableExpr *>(condi->left.get());

        t_LiteralExpr *cond_lit = 
        dynamic_cast<t_LiteralExpr *>(condi->right.get());

        if 
        (
            !cond_var                                    || 
            cond_var->name != init_var->name             || 
            !cond_lit                                    ||
            cond_lit->token_type != e_TokenType::NUMBER  ||
            !IsIntegerLiteralValue(cond_lit->value)
        )
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                Error
                (
                    Previous(), "For-loop condition must compare loop variable to an int literal."
                )
            );
        }

        if (!stmt->increment)
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                Error(Previous(), "For-loop increment is required.")
            );
        }

        bool valid_increment = false;
        if 
        (
            t_PostfixExpr *post = dynamic_cast<t_PostfixExpr *>
            (
                stmt->increment.get()
            )
        )
        {
            t_VariableExpr *var = 
            dynamic_cast<t_VariableExpr *>(post->operand.get());
            if 
            (
                var && var->name == init_var->name &&
                (
                    post->op.type == e_TokenType::PLUS_PLUS ||
                    post->op.type == e_TokenType::MINUS_MINUS
                )
            )
            {
                valid_increment = true;
            }
        }
        else if 
        (
            t_PrefixExpr *pre = 
            dynamic_cast<t_PrefixExpr *>(stmt->increment.get())
        )
        {
            t_VariableExpr *var = 
            dynamic_cast<t_VariableExpr *>(pre->operand.get());
            if 
            (
                var && var->name == init_var->name &&
                (
                    pre->op.type == e_TokenType::PLUS_PLUS ||
                    pre->op.type == e_TokenType::MINUS_MINUS
                )
            )
            {
                valid_increment = true;
            }
        }
        else if 
        (
            t_BinaryExpr *assign = 
            dynamic_cast<t_BinaryExpr *>(stmt->increment.get())
        )
        {
            if (assign->op.type == e_TokenType::PLUS_EQUAL ||
                assign->op.type == e_TokenType::MINUS_EQUAL)
            {
                t_VariableExpr *lhs = 
                dynamic_cast<t_VariableExpr *>(assign->left.get());
                t_LiteralExpr *rhs = 
                dynamic_cast<t_LiteralExpr *>(assign->right.get());
                
                if 
                (
                    lhs && rhs && lhs->name == init_var->name &&
                    rhs->token_type == e_TokenType::NUMBER   &&
                    IsIntegerLiteralValue(rhs->value)
                )
                {
                    valid_increment = true;
                }
            }
        }

        if (!valid_increment)
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                Error
                (
                    Previous(),
                    "For-loop increment must be ++/-- or +=/-= with an int literal."
                )
            );
        }
    }

    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::VarDeclaration()
{
    t_Expected<t_Token, t_ErrorInfo> name_result = 
    Consume(e_TokenType::IDENTIFIER, "Expect variable name.");
    if (!name_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(name_result.Error());
    }
    t_Token name = name_result.Value();

    t_PoolPtr<t_Expr> initializer = nullptr;
    if (Match({e_TokenType::EQUAL}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> init_result = Expression();
        if (!init_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(init_result.Error());
        }
        initializer = t_PoolPtr<t_Expr>(init_result.Value());
    }

    t_Expected<t_Token, t_ErrorInfo> semicolon_result = 
    Consume
    (
        e_TokenType::SEMICOLON, 
        "Expect ';' after variable declaration."
    );
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }
    
    t_VarStmt* stmt = m_Context.CreateStmt<t_VarStmt>
    (
        name.lexeme, 
        std::move(initializer)
    );
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                name.line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::DisplayStatement()
{
    std::vector<t_PoolPtr<t_Expr>> values;

    // Parse the first expression
    t_Expected<t_Expr*, t_ErrorInfo> first_expr_result = Expression();
    if (!first_expr_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(first_expr_result.Error());
    }
    values.push_back(t_PoolPtr<t_Expr>(first_expr_result.Value()));

    // Parse additional comma-separated expressions
    while (Match({e_TokenType::COMMA}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> expr_result = Expression();
        if (!expr_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(expr_result.Error());
        }
        values.push_back(t_PoolPtr<t_Expr>(expr_result.Value()));
    }

    t_Expected<t_Token, t_ErrorInfo> semicolon_result = 
    Consume(e_TokenType::SEMICOLON, "Expect ';' after value.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }

    // Use the new t_DisplayStmt instead of t_PrintStmt
    t_DisplayStmt* stmt = 
    m_Context.CreateStmt<t_DisplayStmt>(std::move(values));
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                Peek().line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::GetinStatement()
{
    // Use Previous() instead of Advance()
    t_Token getin_identifier = Previous(); 

    t_Expected<t_Token, t_ErrorInfo> open_paren_result =
    Consume
    (
        e_TokenType::LEFT_PAREN,
        "Expect '(' after 'getin'."
    );
    if (!open_paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            open_paren_result.Error()
        );
    }

    t_Expected<t_Token, t_ErrorInfo> name_result =
    Consume
    (
        e_TokenType::IDENTIFIER,
        "Expect variable name in getin()."
    );
    if (!name_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            name_result.Error()
        );
    }
    t_Token variable_token = name_result.Value();

    t_Expected<t_Token, t_ErrorInfo> close_paren_result =
    Consume
    (
        e_TokenType::RIGHT_PAREN,
        "Expect ')' after variable name in getin()."
    );
    if (!close_paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            close_paren_result.Error()
        );
    }

    t_Expected<t_Token, t_ErrorInfo> semicolon_result =
    Consume
    (
        e_TokenType::SEMICOLON,
        "Expect ';' after getin() statement."
    );
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            semicolon_result.Error()
        );
    }

    t_GetinStmt* stmt = m_Context.CreateStmt<t_GetinStmt>
    (
        getin_identifier, 
        variable_token.lexeme
    );
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                getin_identifier.line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::FunDeclaration()
{
    t_Expected<t_Token, t_ErrorInfo> name_result =
    Consume
    (
        e_TokenType::IDENTIFIER,
        "Expect function name after 'fun'."
    );
    if (!name_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            name_result.Error()
        );
    }
    t_Token name_token = name_result.Value();

    t_Expected<t_Token, t_ErrorInfo> open_paren_result =
    Consume
    (
        e_TokenType::LEFT_PAREN,
        "Expect '(' after function name."
    );
    if (!open_paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            open_paren_result.Error()
        );
    }

    std::vector<std::string> parameters;

    if (!Check(e_TokenType::RIGHT_PAREN))
    {
        while (true)
        {
            t_Expected<t_Token, t_ErrorInfo> auto_result =
            Consume
            (
                e_TokenType::AUTO,
                "Expect 'auto' before parameter name."
            );
            if (!auto_result.HasValue())
            {
                return t_Expected<t_Stmt*, t_ErrorInfo>
                (
                    auto_result.Error()
                );
            }

            t_Expected<t_Token, t_ErrorInfo> param_name_result =
            Consume
            (
                e_TokenType::IDENTIFIER,
                "Expect parameter name."
            );
            if (!param_name_result.HasValue())
            {
                return t_Expected<t_Stmt*, t_ErrorInfo>
                (
                    param_name_result.Error()
                );
            }

            parameters.push_back(param_name_result.Value().lexeme);

            if (!Match({e_TokenType::COMMA}))
            {
                break;
            }
        }
    }

    t_Expected<t_Token, t_ErrorInfo> close_paren_result =
    Consume
    (
        e_TokenType::RIGHT_PAREN,
        "Expect ')' after function parameters."
    );
    if (!close_paren_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            close_paren_result.Error()
        );
    }

    // Function can be declared with a body or as a prototype;
    // if the next token is '{', parse a body statement, otherwise
    // require a terminating semicolon.
    if (Check(e_TokenType::LEFT_BRACE))
    {
        t_Expected<t_Stmt*, t_ErrorInfo> body_result = Statement();
        if (!body_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                body_result.Error()
            );
        }

        t_Stmt *body_stmt = body_result.Value();

        t_FunStmt* stmt = m_Context.CreateStmt<t_FunStmt>
        (
            name_token.lexeme,
            std::move(parameters),
            t_PoolPtr<t_Stmt>(body_stmt)
        );
        if (!stmt)
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    name_token.line, 
                    0
                )
            );
        }
        return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
    }

    t_Expected<t_Token, t_ErrorInfo> semicolon_result =
    Consume
    (
        e_TokenType::SEMICOLON,
        "Expect ';' after function declaration."
    );
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            semicolon_result.Error()
        );
    }

    t_FunStmt* stmt = m_Context.CreateStmt<t_FunStmt>
    (
        name_token.lexeme,
        std::move(parameters),
        t_PoolPtr<t_Stmt>(nullptr)
    );
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                name_token.line, 
                0
            )
        );
    }
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
    Consume(e_TokenType::SEMICOLON, "Expect ';' after expression.");
    if (!semicolon_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(semicolon_result.Error());
    }
    
    t_ExpressionStmt* stmt = 
    m_Context.CreateStmt<t_ExpressionStmt>(t_PoolPtr<t_Expr>(expr));
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                Peek().line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::EmptyStatement()
{
    t_Token semicolon = Previous();
    t_EmptyStmt* stmt = m_Context.CreateStmt<t_EmptyStmt>(semicolon);
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                semicolon.line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::BenchmarkStatement()
{
    t_Expected<t_Token, t_ErrorInfo> open_brace_result = 
    Consume(e_TokenType::LEFT_BRACE, "Expect '{' after 'benchmark'.");
    if (!open_brace_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(open_brace_result.Error());
    }
    
    // Parse the benchmark body
    std::vector<t_PoolPtr<t_Stmt>> statements;
    while (!Check(e_TokenType::RIGHT_BRACE) && !IsAtEnd())
    {
        t_Expected<t_Stmt*, t_ErrorInfo> stmt_result = Statement();
        if (!stmt_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(stmt_result.Error());
        }
        statements.push_back(t_PoolPtr<t_Stmt>(stmt_result.Value()));
    }
    
    t_Expected<t_Token, t_ErrorInfo> close_brace_result = 
    Consume(e_TokenType::RIGHT_BRACE, "Expect '}' after benchmark body.");
    if (!close_brace_result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(close_brace_result.Error());
    }
    
    // Create a block statement for the body
    t_Stmt *body = m_Context.CreateStmt<t_BlockStmt>(std::move(statements));
    if (!body)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                Peek().line, 
                0
            )
        );
    }
    
    t_BenchmarkStmt* stmt = 
    m_Context.CreateStmt<t_BenchmarkStmt>(t_PoolPtr<t_Stmt>(body));
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory", 
                Peek().line, 
                0
            )
        );
    }
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
                e_TokenType::EQUAL, 
                e_TokenType::PLUS_EQUAL, 
                e_TokenType::MINUS_EQUAL, 
                e_TokenType::STAR_EQUAL, 
                e_TokenType::SLASH_EQUAL,
                e_TokenType::MODULUS_EQUAL
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
            t_BinaryExpr* expr_node = m_Context.CreateExpr<t_BinaryExpr>
            (
                t_PoolPtr<t_Expr>(expr), 
                equals, 
                t_PoolPtr<t_Expr>(value)
            );
            if (!expr_node)
            {
                return t_Expected<t_Expr*, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Out of memory", 
                        equals.line, 
                        0
                    )
                );
            }
            return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
        }

        // If we get here, we're trying to assign to a non-variable
        return t_Expected<t_Expr*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::PARSING_ERROR, 
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

    while (Match({e_TokenType::OR}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = And();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();

        t_BinaryExpr* expr_node = m_Context.CreateExpr<t_BinaryExpr>
        (
            t_PoolPtr<t_Expr>(expr), 
            op, 
            t_PoolPtr<t_Expr>(right)
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    op.line, 
                    0
                )
            );
        }
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

    while (Match({e_TokenType::AND}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Equality();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = m_Context.CreateExpr<t_BinaryExpr>
        (
            t_PoolPtr<t_Expr>(expr), 
            op, 
            t_PoolPtr<t_Expr>(right)
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    op.line, 
                    0
                )
            );
        }
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

    while (Match({e_TokenType::BANG_EQUAL, e_TokenType::EQUAL_EQUAL}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Comparison();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        t_BinaryExpr* expr_node = m_Context.CreateExpr<t_BinaryExpr>
        (
            t_PoolPtr<t_Expr>(expr), 
            op, 
            t_PoolPtr<t_Expr>(right)
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    op.line, 
                    0
                )
            );
        }
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
                e_TokenType::GREATER, 
                e_TokenType::GREATER_EQUAL, 
                e_TokenType::LESS, 
                e_TokenType::LESS_EQUAL
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
        
        t_BinaryExpr* expr_node = m_Context.CreateExpr<t_BinaryExpr>
        (
            t_PoolPtr<t_Expr>(expr), 
            op, 
            t_PoolPtr<t_Expr>(right)
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    op.line, 
                    0
                )
            );
        }
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

    while (Match({e_TokenType::MINUS, e_TokenType::PLUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Factor();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();

        double left_num = 0.0;
        double right_num = 0.0;
        if 
        (
            TryGetNumberLiteral(expr, left_num) && 
            TryGetNumberLiteral(right, right_num)
        )
        {
            double result = 0.0;
            if (op.type == e_TokenType::PLUS)
            {
                result = left_num + right_num;
            }
            else
            {
                result = left_num - right_num;
            }
            expr = MakeNumberLiteral(result, m_Context);
        }
        else
        {
            t_BinaryExpr* expr_node = m_Context.CreateExpr<t_BinaryExpr>
            (
                t_PoolPtr<t_Expr>(expr), 
                op, 
                t_PoolPtr<t_Expr>(right)
            );
            if (!expr_node)
            {
                return t_Expected<t_Expr*, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Out of memory", 
                        op.line, 
                        0
                    )
                );
            }
            expr = expr_node;
        }
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

    while (Match({e_TokenType::SLASH, e_TokenType::STAR, e_TokenType::MODULUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Unary();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();

        double left_num = 0.0;
        double right_num = 0.0;
        if 
        (
            TryGetNumberLiteral(expr, left_num) && 
            TryGetNumberLiteral(right, right_num)
        )
        {
            double result = 0.0;
            if (op.type == e_TokenType::STAR)
            {
                result = left_num * right_num;
            }
            else if (op.type == e_TokenType::MODULUS)
            {
                if (right_num == 0)
                {
                    return t_Expected<t_Expr*, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::PARSING_ERROR, 
                            "Modulus by zero",
                            op.line,
                            0
                        )
                    );
                }
                result = std::fmod(left_num, right_num);
            }
            else if (op.type == e_TokenType::MODULUS)
            {
                if (right_num == 0)
                {
                    return t_Expected<t_Expr*, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::PARSING_ERROR, 
                            "Modulus by zero",
                            op.line,
                            0
                        )
                    );
                }
                result = std::fmod(left_num, right_num);
            }
            else
            {
                result = left_num / right_num;
            }
            expr = MakeNumberLiteral(result, m_Context);
        }
        else
        {
            t_BinaryExpr* expr_node = m_Context.CreateExpr<t_BinaryExpr>
            (
                t_PoolPtr<t_Expr>(expr), 
                op, 
                t_PoolPtr<t_Expr>(right)
            );
            if (!expr_node)
            {
                return t_Expected<t_Expr*, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Out of memory", 
                        op.line, 
                        0
                    )
                );
            }
            expr = expr_node;
        }
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Unary()
{
    if (Match({e_TokenType::BANG, e_TokenType::MINUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> right_result = Unary();
        if (!right_result.HasValue())
        {
            return right_result;
        }
        t_Expr *right = right_result.Value();
        
        if (op.type == e_TokenType::MINUS)
        {
            double value = 0.0;
            if (TryGetNumberLiteral(right, value))
            {
                return t_Expected<t_Expr*, t_ErrorInfo>
                (
                    MakeNumberLiteral(-value, m_Context)
                );
            }
        }

        t_UnaryExpr* expr_node = 
        m_Context.CreateExpr<t_UnaryExpr>(op, t_PoolPtr<t_Expr>(right));
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    op.line, 
                    0
                )
            );
        }
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
    while (Match({e_TokenType::PLUS_PLUS, e_TokenType::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_PostfixExpr* expr_node = 
        m_Context.CreateExpr<t_PostfixExpr>
        (
            t_PoolPtr<t_Expr>(expr), op
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    op.line, 
                    0
                )
            );
        }
        expr = expr_node;
    }

    return t_Expected<t_Expr*, t_ErrorInfo>(expr);
}

t_Expected<t_Expr*, t_ErrorInfo> t_Parser::Primary()
{
    if (Match({e_TokenType::FALSE}))
    {
        t_LiteralExpr* expr_node = 
        m_Context.CreateExpr<t_LiteralExpr>
        (
            "false", 
            e_TokenType::FALSE
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    Peek().line, 
                    0
                )
            );
        }
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({e_TokenType::TRUE}))
    {
        t_LiteralExpr* expr_node = 
        m_Context.CreateExpr<t_LiteralExpr>
        (
            "true", 
            e_TokenType::TRUE
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    Peek().line, 
                    0
                )
            );
        }
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({e_TokenType::NIL}))
    {
        t_LiteralExpr* expr_node = 
        m_Context.CreateExpr<t_LiteralExpr>
        (
            "nil", 
            e_TokenType::NIL
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    Peek().line, 
                    0
                )
            );
        }
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if 
    (
        Match
        (
            {
                e_TokenType::NUMBER, 
                e_TokenType::STRING, 
                e_TokenType::FORMAT_STRING
            }
        )
    )
    {
        t_Token previous = Previous();
        t_LiteralExpr* expr_node = 
        m_Context.CreateExpr<t_LiteralExpr>
        (
            previous.literal, previous.type
        );
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    previous.line, 
                    0
                )
            );
        }
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    // Handle prefix increment/decrement
    if (Match({e_TokenType::PLUS_PLUS, e_TokenType::MINUS_MINUS}))
    {
        t_Token op = Previous();
        t_Expected<t_Expr*, t_ErrorInfo> operand_result = Primary();
        if (!operand_result.HasValue())
        {
            return operand_result;
        }
        t_Expr *operand = operand_result.Value();
        
        t_PrefixExpr* expr_node = 
        m_Context.CreateExpr<t_PrefixExpr>(op, t_PoolPtr<t_Expr>(operand));
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    op.line, 
                    0
                )
            );
        }
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({e_TokenType::IDENTIFIER}))
    {
        t_Token identifier = Previous();

        if (Match({e_TokenType::LEFT_PAREN}))
        {
            std::vector<t_PoolPtr<t_Expr>> arguments;

            if (!Check(e_TokenType::RIGHT_PAREN))
            {
                while (true)
                {
                    t_Expected<t_Expr*, t_ErrorInfo> argument_result =
                    Expression();
                    if (!argument_result.HasValue())
                    {
                        return argument_result;
                    }

                    arguments.push_back
                    (
                        t_PoolPtr<t_Expr>(argument_result.Value())
                    );

                    if (!Match({e_TokenType::COMMA}))
                    {
                        break;
                    }
                }
            }

            t_Expected<t_Token, t_ErrorInfo> close_paren_result =
            Consume
            (
                e_TokenType::RIGHT_PAREN,
                "Expect ')' after function arguments."
            );
            if (!close_paren_result.HasValue())
            {
                return t_Expected<t_Expr*, t_ErrorInfo>
                (
                    close_paren_result.Error()
                );
            }

            t_CallExpr* call_expr = m_Context.CreateExpr<t_CallExpr>
            (
                identifier.lexeme, 
                std::move(arguments),
                identifier.line
            );
            if (!call_expr)
            {
                return t_Expected<t_Expr*, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Out of memory", 
                        identifier.line, 
                        0
                    )
                );
            }
            return t_Expected<t_Expr*, t_ErrorInfo>(call_expr);
        }

        t_VariableExpr* expr_node = 
        m_Context.CreateExpr<t_VariableExpr>(identifier.lexeme);
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    identifier.line, 
                    0
                )
            );
        }
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    if (Match({e_TokenType::LEFT_PAREN}))
    {
        t_Expected<t_Expr*, t_ErrorInfo> expr_result = Expression();
        if (!expr_result.HasValue())
        {
            return expr_result;
        }
        t_Expr *expr = expr_result.Value();
        
        t_Expected<t_Token, t_ErrorInfo> paren_result = 
        Consume(e_TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        if (!paren_result.HasValue())
        {
            return t_Expected<t_Expr*, t_ErrorInfo>(paren_result.Error());
        }
        
        t_GroupingExpr* expr_node = 
        m_Context.CreateExpr<t_GroupingExpr>(t_PoolPtr<t_Expr>(expr));
        if (!expr_node)
        {
            return t_Expected<t_Expr*, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Out of memory", 
                    Peek().line,
                    0
                )
            );
        }
        return t_Expected<t_Expr*, t_ErrorInfo>(expr_node);
    }

    return t_Expected<t_Expr*, t_ErrorInfo>
    (
        t_ErrorInfo
        (
            e_ErrorType::PARSING_ERROR, 
            "Expect expression", 
            Peek().line, 
            0
        )
    );
}

t_Expected<t_Stmt*, t_ErrorInfo> t_Parser::ReturnStatement()
{
    t_Token keyword = Previous();
    
    // Parse the return value expression (if any)
    t_PoolPtr<t_Expr> value = nullptr;
    if (!Check(e_TokenType::SEMICOLON))
    {
        t_Expected<t_Expr*, t_ErrorInfo> value_result = Expression();
        if (!value_result.HasValue())
        {
            return t_Expected<t_Stmt*, t_ErrorInfo>(value_result.Error());
        }
        value = t_PoolPtr<t_Expr>(value_result.Value());
    }
    
    t_Expected<t_Token, t_ErrorInfo> result = 
    Consume(e_TokenType::SEMICOLON, "Expect ';' after return value.");
    if (!result.HasValue())
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>(result.Error());
    }
    
    t_ReturnStmt* stmt = 
    m_Context.CreateStmt<t_ReturnStmt>(std::move(value));
    if (!stmt)
    {
        return t_Expected<t_Stmt*, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Out of memory",
                keyword.line, 
                0
            )
        );
    }
    return t_Expected<t_Stmt*, t_ErrorInfo>(stmt);
}
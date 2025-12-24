#pragma once

#include <vector>
#include <memory>
#include <rubberduck/Token.h>
#include <rubberduck/AST.h>
#include <rubberduck/ASTContext.h>
#include <rubberduck/ErrorHandling.h>

class t_Parser
{
private:
    std::vector<t_Token> m_Tokens;
    int m_Current;
    t_ASTContext& m_Context;

    bool IsAtEnd();
    t_Token Advance();
    bool Check(e_TokenType type);
    t_Token Peek();
    t_Token Previous();
    bool Match(std::initializer_list<e_TokenType> types);
    
    t_Expected<t_Token, t_ErrorInfo> Consume
    (
        e_TokenType type, 
        const std::string &message
    );
    t_ErrorInfo Error(t_Token token, const std::string &message);

    t_Expected<t_Stmt*, t_ErrorInfo> Statement();
    t_Expected<t_Stmt*, t_ErrorInfo> BlockStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> BreakStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> ContinueStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> IfStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> ForStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> VarDeclaration();
    t_Expected<t_Stmt*, t_ErrorInfo> DisplayStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> GetinStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> FunDeclaration();
    t_Expected<t_Stmt*, t_ErrorInfo> ExpressionStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> EmptyStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> BenchmarkStatement();
    t_Expected<t_Stmt*, t_ErrorInfo> ReturnStatement();

    t_Expected<t_Expr*, t_ErrorInfo> Assignment();
    t_Expected<t_Expr*, t_ErrorInfo> Or();
    t_Expected<t_Expr*, t_ErrorInfo> And();
    t_Expected<t_Expr*, t_ErrorInfo> Equality();
    t_Expected<t_Expr*, t_ErrorInfo> Comparison();
    t_Expected<t_Expr*, t_ErrorInfo> Term();
    t_Expected<t_Expr*, t_ErrorInfo> Factor();
    t_Expected<t_Expr*, t_ErrorInfo> Unary();
    t_Expected<t_Expr*, t_ErrorInfo> FinishUnary();
    t_Expected<t_Expr*, t_ErrorInfo> Primary();

public:
    t_Expected<t_Expr*, t_ErrorInfo> Expression();  // Moved from private to public section
    explicit t_Parser
    (
        const std::vector<t_Token> &tokens, 
        t_ASTContext& context
    );
    t_Expected<std::vector<t_PoolPtr<t_Stmt>>, t_ErrorInfo> Parse();
};
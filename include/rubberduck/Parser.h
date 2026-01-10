#pragma once

#include <vector>
#include <memory>
#include <rubberduck/Token.h>
#include <rubberduck/AST.h>
#include <rubberduck/ASTContext.h>
#include <rubberduck/ErrorHandling.h>

class Parser
{
private:
    std::vector<t_Token> m_Tokens;
    int m_Current;
    ASTContext& m_Context;

    bool IsAtEnd();
    t_Token Advance();
    bool Check(e_TokenType type);
    t_Token Peek();
    t_Token Previous();
    bool Match(std::initializer_list<e_TokenType> types);
    
    Expected<t_Token, t_ErrorInfo> Consume
    (
        e_TokenType type, 
        const std::string &message
    );
    t_ErrorInfo Error(t_Token token, const std::string &message);

    Expected<t_Stmt*, t_ErrorInfo> Statement();
    Expected<t_Stmt*, t_ErrorInfo> BlockStatement();
    Expected<t_Stmt*, t_ErrorInfo> BreakStatement();
    Expected<t_Stmt*, t_ErrorInfo> ContinueStatement();
    Expected<t_Stmt*, t_ErrorInfo> IfStatement();
    Expected<t_Stmt*, t_ErrorInfo> ForStatement();
    Expected<t_Stmt*, t_ErrorInfo> VarDeclaration();
    Expected<t_Stmt*, t_ErrorInfo> DisplayStatement();
    Expected<t_Stmt*, t_ErrorInfo> GetinStatement();
    Expected<t_Stmt*, t_ErrorInfo> FunDeclaration();
    Expected<t_Stmt*, t_ErrorInfo> ExpressionStatement();
    Expected<t_Stmt*, t_ErrorInfo> EmptyStatement();
    Expected<t_Stmt*, t_ErrorInfo> BenchmarkStatement();
    Expected<t_Stmt*, t_ErrorInfo> ReturnStatement();

    Expected<t_Expr*, t_ErrorInfo> Assignment();
    Expected<t_Expr*, t_ErrorInfo> Or();
    Expected<t_Expr*, t_ErrorInfo> And();
    Expected<t_Expr*, t_ErrorInfo> Equality();
    Expected<t_Expr*, t_ErrorInfo> Comparison();
    Expected<t_Expr*, t_ErrorInfo> Term();
    Expected<t_Expr*, t_ErrorInfo> Factor();
    Expected<t_Expr*, t_ErrorInfo> Unary();
    Expected<t_Expr*, t_ErrorInfo> FinishUnary();
    Expected<t_Expr*, t_ErrorInfo> Primary();

public:
    Expected<t_Expr*, t_ErrorInfo> Expression();
    explicit Parser
    (
        const std::vector<t_Token> &tokens, 
        ASTContext& context
    );
    Expected<std::vector<PoolPtr<t_Stmt>>, t_ErrorInfo> Parse();
};
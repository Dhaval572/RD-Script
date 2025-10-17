#pragma once

#include <vector>
#include <memory>
#include "Token.h"
#include "AST.h"
#include "MemoryPool.h"

class t_Parser
{
private:
    std::vector<t_Token> tokens;
    int current;
    
    // Memory pool for statement allocations
    static t_MemoryPool stmt_pool;
    static t_MemoryPool expr_pool;

    bool IsAtEnd();
    t_Token Advance();
    bool Check(t_TokenType type);
    t_Token Peek();
    t_Token Previous();
    bool Match(std::initializer_list<t_TokenType> types);
    t_Token Consume(t_TokenType type, const std::string &message);
    std::string Error(t_Token token, const std::string &message);

    t_Stmt *Statement();
    t_Stmt *BlockStatement();
    t_Stmt *BreakStatement();
    t_Stmt *ContinueStatement();
    t_Stmt *IfStatement();
    t_Stmt *ForStatement();
    t_Stmt *VarDeclaration();
    t_Stmt *DisplayStatement();
    t_Stmt *ExpressionStatement();
    t_Stmt *EmptyStatement();
    t_Stmt *BenchmarkStatement();

    t_Expr *Assignment();
    t_Expr *Or();
    t_Expr *And();
    t_Expr *Equality();
    t_Expr *Comparison();
    t_Expr *Term();
    t_Expr *Factor();
    t_Expr *Unary();
    t_Expr *FinishUnary();
    t_Expr *Primary();

public:
    t_Expr *Expression();  // Moved from private to public section
    explicit t_Parser(const std::vector<t_Token> &tokens);
    std::vector<t_Stmt *> Parse();
    
    // Reset the memory pools
    static void ResetPools();
};
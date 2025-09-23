#pragma once
#include <vector>
#include "Token.h"

// Forward declarations for AST nodes
class t_Expr;
class t_Stmt;

class t_Parser
{
private:
    std::vector<t_Token> tokens;
    int current;

    // Helper functions
    bool IsAtEnd();
    t_Token Advance();
    bool Check(t_TokenType type);
    t_Token Peek();
    t_Token Previous();
    bool Match(std::initializer_list<t_TokenType> types);

    // Error handling
    t_Token Consume(t_TokenType type, const std::string &message);
    std::string Error(t_Token token, const std::string &message);

    // Recursive descent parsing functions
    t_Expr *Equality();
    t_Expr *Comparison();
    t_Expr *Term();
    t_Expr *Factor();
    t_Expr *Unary();
    t_Expr *Primary();

    // Statements
    t_Stmt *Statement();
    t_Stmt *DisplayStatement();
    t_Stmt *ExpressionStatement();
    t_Stmt *VarDeclaration();

public:
    t_Parser(const std::vector<t_Token> &tokens);
    std::vector<t_Stmt *> Parse();
    t_Expr *Expression(); // Moved to public section
};
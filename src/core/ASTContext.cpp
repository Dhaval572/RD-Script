#include <rubberduck/ASTContext.h>
#include <algorithm>
#include <memory>  

static std::unique_ptr<t_MemoryPool> stmt_pool_instance = nullptr;
static std::unique_ptr<t_MemoryPool> expr_pool_instance = nullptr;

namespace 
{
    size_t MaxStmtSize()
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
            sizeof(t_GetinStmt),
            sizeof(t_FunStmt),
            sizeof(t_BenchmarkStmt),
            sizeof(t_EmptyStmt),
            sizeof(t_ExpressionStmt)
        };
        size_t max_size = 0;
        for (size_t s : sizes) if (s > max_size) max_size = s;
        return max_size;
    }

    size_t MaxExprSize()
    {
        size_t sizes[] = 
        {
            sizeof(t_BinaryExpr),
            sizeof(t_LiteralExpr),
            sizeof(t_UnaryExpr),
            sizeof(t_GroupingExpr),
            sizeof(t_VariableExpr),
            sizeof(t_PrefixExpr),
            sizeof(t_PostfixExpr),
            sizeof(t_CallExpr)
        };
        size_t max_size = 0;
        for (size_t s : sizes) if (s > max_size) max_size = s;
        return max_size;
    }
}

t_ASTContext::t_ASTContext()
{
    // Initialize memory pools only if they haven't been initialized yet
    if (stmt_pool_instance == nullptr) 
    {
        stmt_pool_instance = std::make_unique<t_MemoryPool>(MaxStmtSize());
    }
    
    if (expr_pool_instance == nullptr) 
    {
        expr_pool_instance = std::make_unique<t_MemoryPool>(MaxExprSize());
    }
}

t_ASTContext::~t_ASTContext()
{
    // Reset the memory pools when the context is destroyed
    Reset();
}

t_MemoryPool& t_ASTContext::GetStmtPool()
{
    // Ensure pools are initialized
    if (stmt_pool_instance == nullptr) 
    {
        stmt_pool_instance = std::make_unique<t_MemoryPool>(MaxStmtSize());
    }
    return *stmt_pool_instance;
}

t_MemoryPool& t_ASTContext::GetExprPool()
{
    // Ensure pools are initialized
    if (expr_pool_instance == nullptr) 
    {
        expr_pool_instance = std::make_unique<t_MemoryPool>(MaxExprSize());
    }
    return *expr_pool_instance;
}

void t_ASTContext::Reset()
{
    if (stmt_pool_instance != nullptr) 
    {
        stmt_pool_instance->Reset();
    }
    
    if (expr_pool_instance != nullptr) 
    {
        expr_pool_instance->Reset();
    }
}
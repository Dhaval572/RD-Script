#include <rubberduck/ASTContext.h>

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
    : m_StmtPool(std::make_unique<t_MemoryPool>(MaxStmtSize()))
    , m_ExprPool(std::make_unique<t_MemoryPool>(MaxExprSize()))
{
}

t_ASTContext::~t_ASTContext()
{
    Reset();
}

t_MemoryPool& t_ASTContext::GetStmtPool()
{
    return *m_StmtPool;
}

t_MemoryPool& t_ASTContext::GetExprPool()
{
    return *m_ExprPool;
}

void t_ASTContext::Reset()
{
    if (m_StmtPool)
    {
        m_StmtPool->Reset();
    }
    
    if (m_ExprPool)
    {
        m_ExprPool->Reset();
    }
}
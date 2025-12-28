#include <rubberduck/ASTContext.h>

t_ASTContext::t_ASTContext()
    : m_StmtPool(std::make_unique<MemoryPool>(sizeof(t_StmtVariant)))
    , m_ExprPool(std::make_unique<MemoryPool>(sizeof(t_ExprVariant)))
{
}

t_ASTContext::~t_ASTContext()
{
    Reset();
}

MemoryPool& t_ASTContext::GetStmtPool()
{
    return *m_StmtPool;
}

MemoryPool& t_ASTContext::GetExprPool()
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
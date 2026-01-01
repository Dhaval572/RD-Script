#include <rubberduck/ASTContext.h>

ASTContext::ASTContext()
    : m_StmtPool(std::make_unique<MemoryPool>(sizeof(StmtVariant)))
    , m_ExprPool(std::make_unique<MemoryPool>(sizeof(ExprVariant)))
{
}

ASTContext::~ASTContext()
{
    Reset();
}

void ASTContext::Reset()
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
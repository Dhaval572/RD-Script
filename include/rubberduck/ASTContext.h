#pragma once

#include <rubberduck/MemoryPool.h>
#include <rubberduck/AST.h>
#include <memory>

// RAII wrapper for managing the lifecycle of memory pools
class t_ASTContext
{
private:
    std::unique_ptr<t_MemoryPool> m_StmtPool;
    std::unique_ptr<t_MemoryPool> m_ExprPool;

public:
    t_ASTContext();
    ~t_ASTContext();

    // Non-copyable, but movable
    t_ASTContext(const t_ASTContext&) = delete;
    t_ASTContext& operator=(const t_ASTContext&) = delete;
    t_ASTContext(t_ASTContext&&) = default;
    t_ASTContext& operator=(t_ASTContext&&) = default;

    t_MemoryPool& GetStmtPool();
    t_MemoryPool& GetExprPool();
    void Reset();
};
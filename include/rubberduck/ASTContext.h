#pragma once

#include <rubberduck/MemoryPool.h>
#include <rubberduck/AST.h>
#include <memory>
#include <utility>

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

    template<typename T, typename... Args>
    T* CreateStmt(Args&&... args)
    {
        void* mem = m_StmtPool->Allocate();
        if (!mem)
        {
            return nullptr;
        }
        return new (mem) T(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T* CreateExpr(Args&&... args)
    {
        void* mem = m_ExprPool->Allocate();
        if (!mem)
        {
            return nullptr;
        }
        return new (mem) T(std::forward<Args>(args)...);
    }
};
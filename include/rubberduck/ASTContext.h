#pragma once

#include <rubberduck/MemoryPool.h>
#include <rubberduck/AST.h>
#include <memory>

// RAII wrapper for managing the lifecycle of memory pools
class ASTContext
{
private:
    std::unique_ptr<MemoryPool> m_StmtPool;
    std::unique_ptr<MemoryPool> m_ExprPool;

public:
    ASTContext();
    ~ASTContext();

    // Non-copyable, but movable
    explicit ASTContext(const ASTContext&) = delete;
    ASTContext& operator=(const ASTContext&) = delete;
    explicit ASTContext(ASTContext&&) = default;
    ASTContext& operator=(ASTContext&&) = default;
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
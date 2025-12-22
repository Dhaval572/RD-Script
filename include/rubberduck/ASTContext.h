#pragma once

#include <rubberduck/MemoryPool.h>
#include <rubberduck/AST.h>

// RAII wrapper for managing the lifecycle of memory pools
class t_ASTContext
{
public:
    t_ASTContext();
    ~t_ASTContext();

    static t_MemoryPool& GetStmtPool();
    static t_MemoryPool& GetExprPool();
    static void Reset();
};
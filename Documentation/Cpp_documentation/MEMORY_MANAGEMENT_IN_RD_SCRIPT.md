# Memory Management in RD Script

This document explains how memory is managed in the RD Script interpreter, which is important for developers to understand for performance and safety.

## Overview

The RD Script interpreter uses a **memory pool-based approach** with custom smart pointers for automatic ownership management:

- **Memory Pools**: Both statement and expression objects are allocated from custom memory pools (`t_MemoryPool`) for better performance and cache locality.

- **Factory Methods**: Objects are constructed via factory methods in `t_ASTContext` (`CreateStmt` and `CreateExpr`), which handle allocation and placement `new` internally.

- **Ownership via `t_PoolPtr`**: Child nodes are managed via `t_PoolPtr`, a specialized `std::unique_ptr` that calls destructors without attempting to free the memory (which is handled by the pool).

- **RAII with `t_ASTContext`**: A `t_ASTContext` instance manages the lifecycle of memory pools and ensures they are reset or destroyed properly.

## Core Components

### 1. Memory Pool (`t_MemoryPool`)

The [t_MemoryPool](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/include/rubberduck/MemoryPool.h) class provides fast allocation by managing chunks and a free list:

```cpp
class t_MemoryPool
{
    void* Allocate();     // Get memory for one object
    void Deallocate(void* ptr);  // Return memory to pool
    void Reset();         // Reset entire pool (bulk cleanup)
};
```

**Key features:**
- Manages an internal linked list of chunks.
- Block size is determined at runtime based on the largest AST node type.
- Maintains a free list for O(1) allocation.
- Properly aligned for all AST node types.

### 2. AST Context (`t_ASTContext`)

The [t_ASTContext](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/include/rubberduck/ASTContext.h) manages individual memory pools for statements and expressions. It provides template factory methods for safe allocation:

```cpp
class t_ASTContext
{
public:
    template<typename T, typename... Args>
    T* CreateStmt(Args&&... args);

    template<typename T, typename... Args>
    T* CreateExpr(Args&&... args);
};
```

Unlike previous versions, `t_ASTContext` is now a local RAII object, typically instantiated in `main.cpp` and passed to the parser.

### 3. Pool Smart Pointers (`t_PoolPtr`)

In [AST.h](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/include/rubberduck/AST.h), we define `t_PoolPtr` to manage ownership:

```cpp
template<typename T>
struct t_PoolDeleter 
{
    void operator()(T* ptr) const 
    {
        if (ptr) ptr->~T(); // Call destructor only
    }
};

template<typename T>
using t_PoolPtr = std::unique_ptr<T, t_PoolDeleter<T>>;
```

`t_PoolPtr` ensures that nested structures have their destructors called correctly when the parent is destroyed, while the actual memory remains managed by the pool.

## How Allocation Works

### Statement and Expression Allocation

Allocations are performed using the `m_Context` reference in the [Parser](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/src/parser/Parser.cpp):

```cpp
// Example: Variable Declaration
t_VarStmt* stmt = m_Context.CreateStmt<t_VarStmt>(name.lexeme, std::move(initializer));

// Example: Binary Expression
return m_Context.CreateExpr<t_BinaryExpr>
(
    std::move(left), 
    op, 
    std::move(right)
);
```

### Ownership Management

Parent nodes own child nodes via `t_PoolPtr`. For example:

```cpp
struct t_BinaryExpr : public t_Expr
{
    t_PoolPtr<t_Expr> left;
    t_Token op;
    t_PoolPtr<t_Expr> right;
};
```

## Memory Lifecycle

1. **Initialization**: A `t_ASTContext` is created (e.g., in [main.cpp](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/main.cpp)). It calculates the maximum node size and initializes internal pools.
2. **Parsing Phase**: The parser uses `CreateStmt` and `CreateExpr` to build the AST.
3. **Interpretation**: The interpreter traverses the AST.
4. **Cleanup**: When `t_ASTContext` is destroyed (at the end of `main`), all memory chunks are freed at once. Destructors for individual nodes are called as `t_PoolPtr` objects go out of scope or are reset.

## Key Points for Developers

1. **Use Factory Methods**: Always use `context.CreateStmt<T>(...)` or `context.CreateExpr<T>(...)`. Never use `new` directly.
2. **Use `t_PoolPtr`**: For any ownership of a pool-allocated AST node, use `t_PoolPtr<T>`.
3. **Efficiency**: This design combines the safety of smart pointers with the performance of pool allocators.
4. **Bulk Reset**: Calling `context.Reset()` can recycle all memory in the pools instantly, which is useful for long-running processes or multiple parsing passes.

## Why This Design?

- **Performance**: Pool allocation is much faster than the standard heap allocator.
- **Safety**: `t_PoolPtr` prevents resource leaks (e.g., if an AST node owns a `std::string`) while maintaining the speed of pool deallocation.
- **Cache Locality**: Nodes of similar types are grouped together in memory chunks.
- **Simplicity**: Developers don't need to manually manage `delete` or placement `new`.

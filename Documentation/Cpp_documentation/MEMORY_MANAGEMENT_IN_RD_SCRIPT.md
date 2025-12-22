# Memory Management in RD Script

This document explains how memory is managed in the RD Script interpreter, which is important for beginner developers to understand.

## Overview

The RD Script interpreter uses a **memory pool-based approach** with smart pointers for automatic ownership management:

- **Memory Pools**: Both statement and expression objects are allocated from custom memory pools (`t_MemoryPool`) for better performance
- **Placement New**: Objects are constructed in-place using placement `new` on pool-allocated memory
- **Ownership via `std::unique_ptr`**: Child nodes are managed via `std::unique_ptr` for automatic cleanup
- **RAII with `t_ASTContext`**: A static `t_ASTContext` manages the lifecycle of memory pools

## Core Components

### 1. Memory Pool (`t_MemoryPool`)

The [t_MemoryPool](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/include/rubberduck/MemoryPool.h) class provides fast allocation by pre-allocating large chunks and managing a free list:

```cpp
class t_MemoryPool
{
    static const size_t BLOCK_SIZE = 1024 * 256; // 256KB chunks
    
    void* Allocate();     // Get memory for one object
    void Deallocate(void* ptr);  // Return memory to pool
    void Reset();         // Reset entire pool (bulk cleanup)
};
```

**Key features:**
- Pre-allocates 256KB chunks to minimize system calls
- Maintains a free list for O(1) allocation
- Properly aligned for all AST node types
- Can be reset in bulk without individual deallocations

### 2. AST Context (`t_ASTContext`)

The [t_ASTContext](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/include/rubberduck/ASTContext.h) manages two separate memory pools:

```cpp
class t_ASTContext
{
public:
    static t_MemoryPool& GetStmtPool();  // Pool for statement nodes
    static t_MemoryPool& GetExprPool();  // Pool for expression nodes
    static void Reset();                 // Reset both pools
};
```

A **static instance** in [main.cpp](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/main.cpp) ensures pools are initialized at startup and cleaned up at program exit:

```cpp
static t_ASTContext ast_context;  // RAII manages pool lifecycle
```

## How Allocation Works

### Statement Allocation

Statements are allocated from the statement pool using placement `new`:

```cpp
// In Parser.cpp - VarDeclaration()
t_VarStmt* stmt = static_cast<t_VarStmt*>
(
    t_ASTContext::GetStmtPool().Allocate()
);
new (stmt) t_VarStmt(name.lexeme, std::move(initializer));
```

### Expression Allocation

Expressions are allocated from the expression pool similarly:

```cpp
// In Parser.cpp - Or()
t_BinaryExpr* expr_node = static_cast<t_BinaryExpr*>
(
    t_ASTContext::GetExprPool().Allocate()
);
new (expr_node) t_BinaryExpr
(
    std::unique_ptr<t_Expr>(left),
    op,
    std::unique_ptr<t_Expr>(right)
);
```

### Ownership Management

Child nodes are wrapped in `std::unique_ptr` within parent nodes. For example, in [AST.h](file:///c:/Users/LENOVO/Documents/RD%20Script/RD-Script/include/rubberduck/AST.h):

```cpp
struct t_BinaryExpr : public t_Expr
{
    std::unique_ptr<t_Expr> left;   // Owns left operand
    t_Token op;
    std::unique_ptr<t_Expr> right;  // Owns right operand
    
    t_BinaryExpr(
        std::unique_ptr<t_Expr> left, 
        t_Token op, 
        std::unique_ptr<t_Expr> right
    ) 
    : left(std::move(left)), 
      op(op), 
      right(std::move(right)) {}
};
```

## Memory Lifecycle

### 1. Initialization
When the program starts, `static t_ASTContext ast_context` in main.cpp creates the memory pools.

### 2. Parsing Phase
The parser allocates AST nodes from the pools:
1. Get raw memory from the appropriate pool (`GetStmtPool()` or `GetExprPool()`)
2. Use placement `new` to construct the object in that memory
3. Wrap child nodes in `std::unique_ptr` to establish ownership

### 3. Interpretation Phase
The interpreter traverses the AST. No memory operations needed since objects remain in pools.

### 4. Cleanup
When `ast_context` goes out of scope (program exit), its destructor calls `Reset()` which recycles all pool memory. The pools' destructors then free the underlying chunks.

## Example: Full Allocation Flow

For the statement `auto x = 5 + 3;`:

1. **Parser allocates `t_LiteralExpr("5")`** from expr pool
2. **Parser allocates `t_LiteralExpr("3")`** from expr pool  
3. **Parser allocates `t_BinaryExpr`** from expr pool, wrapping both literals in `unique_ptr`
4. **Parser allocates `t_VarStmt`** from stmt pool, wrapping binary expr in `unique_ptr`
5. **Interpreter executes** the statement
6. **At program exit**, `ast_context` destructor resets both pools

## Key Points for Developers

1. **No manual `delete` needed** - Memory pools and `unique_ptr` handle cleanup automatically

2. **All allocations go through pools** - Never use `new t_VarStmt(...)` directly; always use `t_ASTContext::GetStmtPool().Allocate()` + placement new

3. **Ownership flows inward** - Child nodes are owned by their parents via `unique_ptr`

4. **Pools reset in bulk** - Individual `Deallocate()` calls are possible but typically `Reset()` is called once at the end

5. **Two separate pools** - Statements and expressions have different size requirements, so separate pools optimize memory usage

## Why This Design?

This memory pool approach was chosen because:

- **Performance**: Pool allocation is O(1) vs. system allocator overhead
- **Cache locality**: AST nodes are contiguous in memory, improving CPU cache performance
- **Bulk cleanup**: Resetting pools is faster than many individual `delete` calls
- **No fragmentation**: Pools prevent memory fragmentation during parsing
- **Safety**: `unique_ptr` ownership prevents memory leaks and dangling pointers
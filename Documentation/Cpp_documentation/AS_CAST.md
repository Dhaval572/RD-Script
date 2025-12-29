# AST Type Casting Guide

## The Basics
Use `As<T>()` to check types. It returns the pointer if it matches, `nullptr` if not.

```cpp
// Check if a statement is a block
if (t_BlockStmt* block = As<t_BlockStmt>(stmt))
{
    // block is valid here
    for (auto& inner : block->statements)
    {
        // Process statements inside block
    }
}
```

## Common Examples

### 1. Check Statement Type
```cpp
if (t_IfStmt* if_stmt = As<t_IfStmt>(stmt))
{
    // It's an if statement
    Evaluate(if_stmt->condition.get());
}
else if (t_ForStmt* for_stmt = As<t_ForStmt>(stmt))
{
    // It's a for loop
    Execute(for_stmt->body.get());
}
```

### 2. Find Function Declarations
```cpp
for (auto& stmt : statements)
{
    if (t_FunStmt* fun = As<t_FunStmt>(stmt.get()))
    {
        // Store function
        m_Functions[fun->name] = fun;
    }
}
```

### 3. Get Loop Initializer
```cpp
t_ForStmt* for_stmt = As<t_ForStmt>(stmt);
if (for_stmt)
{
    t_VarStmt* init = As<t_VarStmt>(for_stmt->initializer.get());
    if (init)
    {
        // Variable declaration in for loop
    }
}
```

## When to Use `.get()`

**With `t_PoolPtr<T>`** - Use `.get()`:
```cpp
t_PoolPtr<t_Stmt> stmt = ...;
t_IfStmt* if_stmt = As<t_IfStmt>(stmt.get());
```

**With raw pointers** - No `.get()` needed:
```cpp
t_Stmt* stmt = ...;
t_IfStmt* if_stmt = As<t_IfStmt>(stmt);
```

## Simple Pattern

```cpp
void Process(t_Stmt* stmt)
{
    if (t_IfStmt* if_stmt = As<t_IfStmt>(stmt))
    {
        HandleIf(if_stmt);
    }
    else if (t_ForStmt* for_stmt = As<t_ForStmt>(stmt))
    {
        HandleFor(for_stmt);
    }
    else if (t_VarStmt* var_stmt = As<t_VarStmt>(stmt))
    {
        HandleVar(var_stmt);
    }
}
```

## Why Not dynamic_cast?
Our memory pool doesn't support `dynamic_cast`. Use `As<T>()` instead - same syntax, actually works.

## Quick Reference

```cpp
// Check expression type
if (t_LiteralExpr* lit = As<t_LiteralExpr>(expr))
{
    // It's a literal like "42"
}

// Check variable reference
if (t_VariableExpr* var = As<t_VariableExpr>(expr))
{
    // It's a variable like "x"
}

// Check binary operation
if (t_BinaryExpr* bin = As<t_BinaryExpr>(expr))
{
    // It's something like "x + y"
}
```

## Rule to Remember
Always check if `As<T>()` returned non-null before using the pointer.

That's all you need to know.
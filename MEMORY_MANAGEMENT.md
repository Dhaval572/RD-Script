# Memory Management in RD Script

This document explains how memory is managed in the RD Script interpreter, which is important for beginner developers to understand.

## Overview

The RD Script interpreter uses a hybrid approach to memory management:
- **Statement objects** are created with `new` and must be explicitly deleted
- **Expression objects** are automatically managed by `std::unique_ptr` and cleaned up automatically

## How It Works

### 1. Statement Objects (Manually Managed)
Statement objects like [t_VarStmt](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L79-L86), [t_DisplayStmt](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L69-L77), and [t_ExpressionStmt](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L59-L66) are created with `new` in the parser:

```cpp
return new t_VarStmt(name.lexeme, std::move(initializer));
```

These objects must be explicitly deleted in main.cpp:

```cpp
// Clean up AST nodes
for (t_Stmt* stmt : statements) 
{
    delete stmt;  // This deletes the statement object
}
```

### 2. Expression Objects (Automatically Managed)
Expression objects like [t_BinaryExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L13-L28), [t_UnaryExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L38-L44), [t_LiteralExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L30-L35), etc. are also created with `new`, but they are **immediately wrapped in `std::unique_ptr`**:

```cpp
return new t_UnaryExpr(op, std::unique_ptr<t_Expr>(right));
```

### 3. Automatic Cleanup Chain
When a statement object is deleted, here's what happens automatically:

1. `delete stmt;` in main.cpp deletes the statement object
2. The statement's destructor runs automatically
3. Any `std::unique_ptr` members in the statement automatically delete their contained expressions
4. If those expressions contain other expressions (like a [t_BinaryExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L13-L28) containing two more expressions), their destructors run
5. Their `std::unique_ptr` members automatically delete their contained expressions
6. This continues recursively until all expressions are cleaned up

## Example Cleanup Flow

For a statement like `auto x = 5 + 3;`:

1. Parser creates: `new t_VarStmt("x", new t_BinaryExpr(new t_LiteralExpr("5"), +, new t_LiteralExpr("3")))`
2. The [t_BinaryExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L13-L28) constructor wraps the two [t_LiteralExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L30-L35) objects in `std::unique_ptr`
3. The [t_VarStmt](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L79-L86) constructor wraps the [t_BinaryExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L13-L28) in a `std::unique_ptr`
4. Later, `delete stmt;` deletes the [t_VarStmt](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L79-L86)
5. [t_VarStmt](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L79-L86)'s destructor automatically deletes the [t_BinaryExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L13-L28)
6. [t_BinaryExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L13-L28)'s destructor automatically deletes the two [t_LiteralExpr](file:///c%3A/Users/LENOVO/Documents/RD%20Script/RD-Script/include/AST.h#L30-L35) objects

## Key Points for Beginners

1. **Statement objects require manual deletion** - They're created with `new` and must be explicitly deleted in main.cpp

2. **Expression objects are automatically cleaned up** - Even though they're created with `new`, they're immediately wrapped in `std::unique_ptr`, which automatically cleans them up

3. **The cleanup chain is automatic** - When you delete a statement, all its contained expressions are automatically cleaned up recursively

4. **This pattern prevents memory leaks** - As long as statement objects are properly deleted, no memory leaks will occur

## Why This Design?

This hybrid approach was chosen because:
- It separates the concerns of statement management (manual) and expression management (automatic)
- It allows for clear ownership of statement objects in the main function
- It leverages smart pointers for the complex tree structure of expressions
- It's a common pattern in interpreter implementations
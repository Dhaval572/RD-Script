# AST (Abstract Syntax Tree) Class Diagram


## Visual Hierarchy Tree

```
                                     ┌─────────────────────┐
                                     │      t_Expr         │
                                     │   (Abstract)        │
                                     └─────────────────────┘
                                               △
                ┌─────────────────────────────────────────────────────────────┐
                │                               │                             │
    ┌─────────────────────┐         ┌─────────────────────┐         ┌─────────────────────┐
    │    t_BinaryExpr     │         │     t_UnaryExpr     │         │   t_GroupingExpr    │
    └─────────────────────┘         └─────────────────────┘         └─────────────────────┘
                                                                               △
    ┌─────────────────────┐         ┌─────────────────────┐         ┌─────────────────────┐
    │   t_LiteralExpr     │         │   t_VariableExpr    │         │     t_PrefixExpr    │
    └─────────────────────┘         └─────────────────────┘         └─────────────────────┘
                                                                               △
    ┌─────────────────────┐         ┌─────────────────────┐
    │    t_PostfixExpr    │         │                     │
    └─────────────────────┘         │                     │
                                    │                     │
                                     └─────────────────────┘


                                     ┌─────────────────────┐
                                     │      t_Stmt         │
                                     │   (Abstract)        │
                                     └─────────────────────┘
                                               △
    ┌─────────────────────────────────────────────────────────────────────────────────────┐
    │                                                                                     │
┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐
│  t_ExpressionStmt   │ │     t_EmptyStmt     │ │    t_DisplayStmt    │ │      t_VarStmt      │
└─────────────────────┘ └─────────────────────┘ └─────────────────────┘ └─────────────────────┘

┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐
│     t_BlockStmt     │ │      t_IfStmt       │ │      t_ForStmt      │ │     t_BreakStmt     │
└─────────────────────┘ └─────────────────────┘ └─────────────────────┘ └─────────────────────┘

┌─────────────────────┐ ┌─────────────────────┐
│   t_ContinueStmt    │ │   t_BenchmarkStmt   │
└─────────────────────┘ └─────────────────────┘
```

## ASCII Art Hierarchy Representation

```
                             ┌─────────────────┐
                             │     t_Expr      │
                             │   (Abstract)    │
                             └─────────────────┘
                                     △
    ┌─────────────────────────────────────────────────────────────┐
    │     │     │     │      │      │      │     │                │
┌─────┐ ┌─────┐ ┌─────┐ ┌──────┐ ┌──────┐ ┌─────┐ ┌──────┐       │
│Binary│ │Unary│ │Group│ │Literal│ │Variable││Prefix│ │Postfix│       │
└─────┘ └─────┘ └─────┘ └──────┘ └──────┘ └─────┘ └──────┘       │
                                                                   │
                             ┌─────────────────┐                   │
                             │     t_Stmt      │                   │
                             │   (Abstract)    │                   │
                             └─────────────────┘                   │
                                     △                             │
    ┌─────────────────────────────────────────────────────────────┐
    │                                                             │
┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐   │
│Express│ │Empty │ │Display│ │Var   │ │Block │ │If    │ │For   │   │
│ionStmt│ │Stmt  │ │Stmt   │ │Stmt  │ │Stmt  │ │Stmt  │ │Stmt  │   │
└──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘   │
                                                                   │
┌──────┐ ┌──────┐ ┌──────────┐                                   │
│Break │ │Continue││Benchmark │                                   │
│Stmt  │ │Stmt   ││Stmt      │                                   │
└──────┘ └──────┘ └──────────┘                                   │
                                                                  │
```

## Key Relationships

1. **Inheritance Hierarchy**:
   - All expression types inherit from `t_Expr`
   - All statement types inherit from `t_Stmt`

2. **Composition Relationships**:
   - `t_BinaryExpr`, `t_UnaryExpr`, `t_GroupingExpr` contain other expressions
   - `t_DisplayStmt` contains multiple expressions
   - `t_BlockStmt` contains multiple statements
   - `t_IfStmt`, `t_ForStmt` contain both expressions and statements

3. **Specialized Types**:
   - `t_PrefixExpr` and `t_PostfixExpr` for increment/decrement operations
   - `t_BenchmarkStmt` for performance measurement
   - `t_BreakStmt` and `t_ContinueStmt` for loop control flow

This AST structure supports a comprehensive language with expressions, control flow, variable declarations, and specialized statements for display and benchmarking.
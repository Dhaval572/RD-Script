# AST (Abstract Syntax Tree) Class Diagram

## Complete Hierarchy Visualization

### Expression Hierarchy

```
                                     ┌─────────────────────────┐
                                     │       t_Expr            │
                                     │      (Abstract)         │
                                     └─────────────────────────┘
                                               │
                ┌──────────────────────────────────────────────────────┐
                │                                                      │
        ┌───────┼───────┐                                              │
        │               │                                              │
    ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐
    │ Binary  │   │ Unary   │   │Grouping │   │Literal  │   │Variable │
    │ Expr    │   │ Expr    │   │ Expr    │   │ Expr    │   │ Expr    │
    └─────────┘   └─────────┘   └─────────┘   └─────────┘   └─────────┘
        │                                                              │
        │                                                              │
    ┌─────────┐                                                  ┌─────────┐
    │ Prefix  │                                                  │Postfix  │
    │ Expr    │                                                  │ Expr    │
    └─────────┘                                                  └─────────┘
```

### Statement Hierarchy

```
                                     ┌─────────────────────────┐
                                     │       t_Stmt            │
                                     │      (Abstract)         │
                                     └─────────────────────────┘
                                               │
        ┌──────────────────────────────────────────────────────────────────┐
        │                                                                  │
    ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐     │
    │Expression│ │ Empty  │ │Display │ │ Var   │ │ Block │ │ If    │     │
    │  Stmt   │ │ Stmt   │ │ Stmt   │ │ Stmt  │ │ Stmt  │ │ Stmt  │     │
    └───────┘  └───────┘  └───────┘  └───────┘  └───────┘  └───────┘     │
        │                                                                 │
        │                                                                 │
    ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐                │
    │ For   │  │Break  │  │Continue│  │Benchmark│                         │
    │ Stmt  │  │ Stmt  │  │ Stmt  │  │ Stmt   │                         │
    └───────┘  └───────┘  └───────┘  └───────┘                          │
```

## Detailed Tree Structure

### Expression Tree
```
                              ┌──────────────────────────┐
                              │        t_Expr            │
                              └────────────┬─────────────┘
                                           │
      ┌────────────┬────────────┬──────────┼──────────┬──────────┬──────────┐
      │            │            │          │          │          │          │
  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐
  │Binary  │  │Unary   │  │Grouping│  │Literal │  │Variable│  │Prefix  │  │Postfix │
  │Expr    │  │Expr    │  │Expr    │  │Expr    │  │Expr    │  │Expr    │  │Expr    │
  └────────┘  └────────┘  └────────┘  └────────┘  └────────┘  └────────┘  └────────┘
      │            │            │          │          │          │            │
      ├────────────┘            │          │          │          └────────────┤
      │                         │          │          │                       │
  ┌───┴───┐               ┌─────┴─────┐    │    ┌─────┴─────┐           ┌───┴───┐
  │ left  │               │expression │    │    │  name     │           │operand│
  │ op    │               └───────────┘    │    └───────────┘           │ op    │
  │ right │                                │                            └───────┘
  └───────┘                          ┌─────┴─────┐
                                     │ value     │
                                     │token_type │
                                     └───────────┘
```

### Statement Tree
```
                              ┌──────────────────────────┐
                              │        t_Stmt            │
                              └────────────┬─────────────┘
                                           │
      ┌────────────────────────────────────┼─────────────────────────────────────┐
      │                                    │                                     │
  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  │  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐
  │Expression││Empty ││Display││ Var  │  │  │Block ││ If   ││ For  ││Break │
  │  Stmt   ││Stmt  ││Stmt   ││Stmt  │  │  │Stmt  ││Stmt  ││Stmt  ││Stmt  │
  └──────┘  └──────┘  └──────┘  └──────┘  │  └──────┘  └──────┘  └──────┘  └──────┘
      │         │         │         │      │     │         │         │         │
      │         │         │         │      │     │         │         │         │
  ┌───┴───┐ ┌───┴───┐ ┌───┴───┐ ┌──┴──┐  │  ┌──┴──┐  ┌──┴──┐  ┌──┴──┐  ┌───┴───┐
  │expression││semicolon││expressions││name│  │statements││condition││initializer││keyword│
  └───────┘ └───────┘ └────────┘ │initializer││          ││then_branch││condition│└───────┘
                                 └──────────┘│          ││else_branch││increment│
                                             └──────────┘│          ││ body   │
                                                         └──────────┘└────────┘
                                                                            │
                                                                ┌───────────┼───────────┐
                                                                │           │           │
                                                            ┌──────┐    ┌──────┐    ┌──────┐
                                                            │Continue│    │Benchmark│
                                                            │ Stmt  │    │ Stmt  │
                                                            └──────┘    └──────┘
                                                                 │           │
                                                             ┌───┴───┐   ┌───┴───┐
                                                             │keyword│   │ body  │
                                                             └───────┘   └───────┘
```

## Clean ASCII Box Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                AST Hierarchy                                │
└─────────────────────────────────────────────────────────────────────────────┘

          ┌─────────────────────────┐       ┌─────────────────────────┐
          │        t_Expr           │       │        t_Stmt           │
          │       (Abstract)        │       │       (Abstract)        │
          └───────────┬─────────────┘       └───────────┬─────────────┘
                      │                                 │
      ┌───────────────┼───────────────┐   ┌─────────────┼─────────────────┐
      │               │               │   │             │                 │
  ┌─────┴─────┐   ┌───┴───┐   ┌───────┴───────┐   ┌─────┴─────┐   ┌───────┴───────┐
  │  Binary   │   │ Unary │   │  Grouping     │   │Expression │   │    Empty      │
  └─────┬─────┘   └───┬───┘   └───────┬───────┘   └─────┬─────┘   └───────┬───────┘
        │             │               │                 │                 │
  ┌─────┴─────┐   ┌───┴───┐   ┌───────┴───────┐   ┌─────┴─────┐   ┌───────┴───────┐
  │  Literal  │   │Variable│   │   Prefix     │   │  Display  │   │     Var       │
  └─────┬─────┘   └───┬───┘   └───────┬───────┘   └─────┬─────┘   └───────┬───────┘
        │             │               │                 │                 │
  ┌─────┴─────┐   ┌───┴───┐       ┌───┴───┐       ┌─────┴─────┐       ┌───┴───┐
  │  Postfix  │   │       │       │       │       │   Block   │       │  If   │
  └───────────┘   │       │       │       │       └─────┬─────┘       └───┬───┘
                  │       │       │       │             │                 │
              ┌───┴───┐   │       │   ┌───┴───┐     ┌───┴───┐         ┌───┴───┐
              │       │   │       │   │       │     │  For  │         │ Break │
              │       │   │       │   │       │     └───┬───┘         └───┬───┘
              │       │   │       │   │       │         │                 │
          ┌───┴───┐   │   │   ┌───┴───┐       │     ┌───┴───┐         ┌───┴───┐
          │       │   │   │   │       │       │     │Continue│         │Benchmark│
          │       │   │   │   │       │       │     └────────┘         └────────┘
          └───────┘   └───┘   └───────┘       └──────────────────────────────┘
```

## Relationship Matrix

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Inheritance Relationships                          │
├─────────────────────┬──────────────────────────────────────────────────────┤
│     Base Class      │                   Derived Classes                     │
├─────────────────────┼──────────────────────────────────────────────────────┤
│      t_Expr         │ ┌─────────┬─────────┬─────────┬─────────┬─────────┐  │
│    (Abstract)       │ │ Binary  │ Unary   │Grouping │Literal  │Variable │  │
│                     │ │ Expr    │ Expr    │ Expr    │ Expr    │ Expr    │  │
│                     │ ├─────────┼─────────┼─────────┼─────────┼─────────┤  │
│                     │ │ Prefix  │Postfix  │         │         │         │  │
│                     │ │ Expr    │ Expr    │         │         │         │  │
│                     │ └─────────┴─────────┴─────────┴─────────┴─────────┘  │
├─────────────────────┼──────────────────────────────────────────────────────┤
│      t_Stmt         │ ┌─────────┬─────────┬─────────┬─────────┬─────────┐  │
│    (Abstract)       │ │Expression│ Empty   │Display  │ Var     │ Block  │  │
│                     │ │ Stmt     │ Stmt    │ Stmt    │ Stmt    │ Stmt   │  │
│                     │ ├─────────┼─────────┼─────────┼─────────┼─────────┤  │
│                     │ │ If      │ For     │ Break   │Continue │Benchmark│  │
│                     │ │ Stmt    │ Stmt    │ Stmt    │ Stmt    │ Stmt    │  │
│                     │ └─────────┴─────────┴─────────┴─────────┴─────────┘  │
└─────────────────────┴──────────────────────────────────────────────────────┘
```

## Composition Relationships

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Composition Relationships                          │
├─────────────────────┬──────────────────────────────────────────────────────┤
│     Container       │                  Contains                             │
├─────────────────────┼──────────────────────────────────────────────────────┤
│   t_BinaryExpr      │ left(t_Expr), op(Token), right(t_Expr)               │
│   t_UnaryExpr       │ op(Token), right(t_Expr)                             │
│   t_GroupingExpr    │ expression(t_Expr)                                   │
│   t_LiteralExpr     │ value(string), token_type(e_TOKEN_TYPE)              │
│   t_VariableExpr    │ name(string)                                         │
│   t_PrefixExpr      │ op(Token), operand(t_Expr)                           │
│   t_PostfixExpr     │ operand(t_Expr), op(Token)                           │
│   t_ExpressionStmt  │ expression(t_Expr)                                   │
│   t_EmptyStmt       │ semicolon(Token)                                     │
│   t_DisplayStmt     │ expressions(vector<t_Expr>)                          │
│   t_VarStmt         │ name(string), initializer(t_Expr)                    │
│   t_BlockStmt       │ statements(vector<t_Stmt>)                           │
│   t_IfStmt          │ condition(t_Expr), then_branch(t_Stmt),              │
│                     │ else_branch(t_Stmt)                                  │
│   t_ForStmt         │ initializer(t_Stmt), condition(t_Expr),              │
│                     │ increment(t_Expr), body(t_Stmt)                      │
│   t_BreakStmt       │ keyword(Token)                                       │
│   t_ContinueStmt    │ keyword(Token)                                       │
│   t_BenchmarkStmt   │ body(t_Stmt)                                         │
└─────────────────────┴──────────────────────────────────────────────────────┘
```

This complete diagram provides a perfect visual representation of the AST hierarchy with clear inheritance and composition relationships.
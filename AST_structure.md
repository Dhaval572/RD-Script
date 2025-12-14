# AST (Abstract Syntax Tree) Class Diagram

```mermaid
classDiagram`
    class t_Expr {
        <<abstract>>
        +~t_Expr() virtual
    }
    
    class t_Stmt {
        <<abstract>>
        +~t_Stmt() virtual
    }
    
    class t_BinaryExpr {
        +unique_ptr~t_Expr~ left
        +t_Token op
        +unique_ptr~t_Expr~ right
        +t_BinaryExpr(left, op, right)
    }
    
    class t_LiteralExpr {
        +string value
        +e_TOKEN_TYPE token_type
        +t_LiteralExpr(value, type)
    }
    
    class t_UnaryExpr {
        +t_Token op
        +unique_ptr~t_Expr~ right
        +t_UnaryExpr(op, right)
    }
    
    class t_GroupingExpr {
        +unique_ptr~t_Expr~ expression
        +t_GroupingExpr(expression)
    }
    
    class t_VariableExpr {
        +string name
        +t_VariableExpr(name)
    }
    
    class t_PrefixExpr {
        +t_Token op
        +unique_ptr~t_Expr~ operand
        +t_PrefixExpr(op, operand)
    }
    
    class t_PostfixExpr {
        +unique_ptr~t_Expr~ operand
        +t_Token op
        +t_PostfixExpr(operand, op)
    }
    
    class t_ExpressionStmt {
        +unique_ptr~t_Expr~ expression
        +t_ExpressionStmt(expression)
    }
    
    class t_EmptyStmt {
        +t_Token semicolon
        +t_EmptyStmt(semicolon)
    }
    
    class t_DisplayStmt {
        +vector~unique_ptr~t_Expr~~ expressions
        +t_DisplayStmt(expressions)
    }
    
    class t_VarStmt {
        +string name
        +unique_ptr~t_Expr~ initializer
        +t_VarStmt(name, initializer)
    }
    
    class t_BlockStmt {
        +vector~unique_ptr~t_Stmt~~ statements
        +t_BlockStmt(statements)
    }
    
    class t_IfStmt {
        +unique_ptr~t_Expr~ condition
        +unique_ptr~t_Stmt~ then_branch
        +unique_ptr~t_Stmt~ else_branch
        +t_IfStmt(condition, then_branch, else_branch)
    }
    
    class t_ForStmt {
        +unique_ptr~t_Stmt~ initializer
        +unique_ptr~t_Expr~ condition
        +unique_ptr~t_Expr~ increment
        +unique_ptr~t_Stmt~ body
        +t_ForStmt(initializer, condition, increment, body)
    }
    
    class t_BreakStmt {
        +t_Token keyword
        +t_BreakStmt(keyword)
    }
    
    class t_ContinueStmt {
        +t_Token keyword
        +t_ContinueStmt(keyword)
    }
    
    class t_BenchmarkStmt {
        +unique_ptr~t_Stmt~ body
        +t_BenchmarkStmt(body)
    }
    
    t_Expr <|-- t_BinaryExpr
    t_Expr <|-- t_LiteralExpr
    t_Expr <|-- t_UnaryExpr
    t_Expr <|-- t_GroupingExpr
    t_Expr <|-- t_VariableExpr
    t_Expr <|-- t_PrefixExpr
    t_Expr <|-- t_PostfixExpr
    
    t_Stmt <|-- t_ExpressionStmt
    t_Stmt <|-- t_EmptyStmt
    t_Stmt <|-- t_DisplayStmt
    t_Stmt <|-- t_VarStmt
    t_Stmt <|-- t_BlockStmt
    t_Stmt <|-- t_IfStmt
    t_Stmt <|-- t_ForStmt
    t_Stmt <|-- t_BreakStmt
    t_Stmt <|-- t_ContinueStmt
    t_Stmt <|-- t_BenchmarkStmt
    
    t_BinaryExpr --> t_Expr : contains
    t_UnaryExpr --> t_Expr : contains
    t_GroupingExpr --> t_Expr : contains
    t_PrefixExpr --> t_Expr : contains
    t_PostfixExpr --> t_Expr : contains
    
    t_ExpressionStmt --> t_Expr : contains
    t_DisplayStmt --> t_Expr : contains
    t_VarStmt --> t_Expr : contains
    t_IfStmt --> t_Expr : contains
    t_IfStmt --> t_Stmt : contains
    t_ForStmt --> t_Expr : contains
    t_ForStmt --> t_Stmt : contains
    t_BlockStmt --> t_Stmt : contains
    t_BenchmarkStmt --> t_Stmt : contains
```

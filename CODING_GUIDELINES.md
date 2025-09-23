# Rubber Duck Scripting Language - C++ Coding Guidelines

## Naming Conventions

1. **Struct/Class Names**: Use `t_` prefix followed by PascalCase
   - Example: `t_Token`, `t_Lexer`, `t_Parser`

2. **Variable Names**: Use snake_case
   - Example: `token_type`, `current_line`, `file_content`

3. **Function Names**: Use PascalCase
   - Example: `ScanTokens`, `ParseExpression`, `EvaluateNode`

4. **Constants**: Use UPPER_SNAKE_CASE
   - Example: `MAX_TOKENS`, `DEFAULT_BUFFER_SIZE`

5. **Enum Names**: Use PascalCase
   - Example: `TokenType`, `ParserState`

6. **Enum Values**: Use UPPER_SNAKE_CASE
   - Example: `TOKEN_EOF`, `PARSER_ERROR`

## Formatting Style

1. **Braces**: Use Allman style (opening brace on the next line)
   ```cpp
   if (condition)
   {
       // code
   }
   else
   {
       // code
   }
   ```

2. **Indentation**: Use 4 spaces (no tabs)

3. **Line Length**: Maximum 100 characters per line

4. **Spacing**:
   - No space between function name and opening parenthesis: `FunctionName()`
   - Space after keywords: `if (condition)`, `for (int i = 0; i < 10; i++)`
   - Space around operators: `a = b + c;`

5. **Pointer and Reference Declarations**:
   ```cpp
   int* pointer;
   int& reference;
   ```

## File Organization

1. **Header Files (.h)**:
   - Use include guards: `#pragma once`
   - Declare classes, functions, and constants
   - Include minimal necessary headers

2. **Implementation Files (.cpp)**:
   - Include corresponding header first
   - Follow with project headers
   - Follow with standard library headers
   - Use comments to separate sections of code

## Comments

1. **File Headers**: Each file should have a comment block at the top describing its purpose
2. **Function Comments**: Describe what the function does, its parameters, and return value
3. **Inline Comments**: Use sparingly and only when code is not self-explanatory
4. **TODO Comments**: Mark incomplete implementations with `// TODO:`

## Error Handling

1. Use exceptions for error reporting
2. Provide meaningful error messages with context
3. Clean up resources in destructors or use RAII

## Language Design Principles

1. **Static Typing**: All variables must be explicitly declared using the `auto` keyword
2. **C-like Syntax**: Familiar syntax for developers coming from C/C++/Java/C#
3. **Automatic Memory Management**: No manual memory management required
4. **Consistent Naming**: Language keywords use lowercase (e.g., `auto`, `linkedlist`, `dynamicarray`)
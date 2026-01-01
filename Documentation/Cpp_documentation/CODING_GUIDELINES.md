# Rubber Duck Scripting Language - C++ Coding Guidelines

## Naming Conventions

1. **Struct Names**: Use `t_` prefix followed by PascalCase
   - Example: `t_Token`

2. **Class Names**: Use PascalCase
   - Example: `Parser`, `Interpreter`

3. **Variable Names**: Use snake_case
   - Example: `token_type`, `current_line`, `file_content`

4. **Function Names**: Use PascalCase
   - Example: `ScanTokens`, `ParseExpression`, `EvaluateNode`

5. **Private Member Variables**: Use `m_` prefix followed by PascalCase
   - Example: `m_CurrentLine`, `m_FileContent`

6. **Constants**: Use UPPER_SNAKE_CASE
   - Example: `MAX_TOKENS`, `DEFAULT_BUFFER_SIZE`

7. **Enum Names**: Use PascalCase
   - Example: `e_TokenType`, `e_ParserState`

8. **Enum Values**: Use UPPER_SNAKE_CASE
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

3. **Line Length**: Maximum 75 characters per line

4. **Function Formmating:** If function argument is big and going to new line then do this:

For example:
```cpp
void func(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k); // to long
```

```cpp
void func
(
    int a,
    int b, 
    // And so on
)
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

1. **Function Comments**: Describe what the function does, its parameters, and return value
2. **Inline Comments**: Use sparingly and only when code is not self-explanatory

3. **TODO Comments**: Mark incomplete implementations with `// TODO:`

## Some Important Rules

## Error Handling
- **Exception-less design**: Use return values, error codes, or `std::optional` instead of exceptions
- Prefer error handling through return status

## Type Declarations
- **Avoid `auto`**: Use explicit types except in obvious contexts like iterators
- Prefer clarity over brevity in type declarations

## Data Structures
- **Prefer arrays**: Use `std::array` or fixed arrays for known sizes
- Reserve vectors for dynamic collections only

## Safety Practices
- **Bounds checking**: Always validate indices before array/container access
- Validate all inputs and handle edge cases explicitly
- Initialize all variables

## Memory Management
- **Smart pointers only**: Use `std::unique_ptr` and `std::shared_ptr`
- No raw `new`/`delete` operations
- And here you can use `PoolPtr<>` which is custom memory pool for AST. ( Automatically manages memory )

## Concurrency
- **No multithreading**: Avoid `std::thread`, mutexes, atomics
- Design for single-threaded execution only

# Pre-Pull Request Validation

## Core Language Feature Verification

Before any pull request, **MUST test** that existing Rubber Duck language syntax still works correctly:

### 1. **Basic Syntax Validation**
```
./rd.bat basic_syntax.rd      # Test core language constructs
./rd.bat control_flow.rd      # Test if/else, loops
./rd.bat functions.rd         # Test function definitions/calls
```

### 2. **Language Semantics Check**
- Verify all existing language features parse and execute correctly
- Ensure no breaking changes to the language syntax
- Confirm all operators work as expected

### 3. **Edge Case Verification**
```
./rd.bat edge_cases.rd        # Test boundary conditions
./rd.bat error_cases.rd       # Test error handling
```

### 4. **Regression Prevention**
- The existing Rubber Duck codebase must compile/run without changes
- All standard library functions (if any) must work
- Language semantics must remain consistent

**Rule**: If `./rd.bat` fails on any existing `.rd` file that previously worked, fix before pull request.
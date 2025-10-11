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

1. **Static Typing**: All variables must be explicitly declared using the `auto` keyword and maintain their type throughout their lifetime
2. **C-like Syntax**: Familiar syntax for developers coming from C/C++/Java/C#
3. **Automatic Memory Management**: No manual memory management required
4. **Consistent Naming**: Language keywords use lowercase (e.g., `auto`, `linkedlist`, `dynamicarray`)

## Language Constructs

### Variable Declarations

In RD Script, all variables must be explicitly declared using the `auto` keyword before they can be used. This enforces static typing and prevents accidental variable creation.

Syntax:
```rubberduck
auto variable_name = initial_value;
```

Examples:
```rubberduck
auto x = 10;
auto name = "Rubber Duck";
auto is_active = true;
```

Attempting to use a variable without declaring it with `auto` will result in a runtime error:
```rubberduck
x = 10;  // Error: Variable 'x' must be declared with 'auto' keyword before use
```

### Static Typing Enforcement

RD Script enforces static typing similar to C++. Once a variable is declared with a specific type, it cannot be assigned values of a different type:

```rubberduck
auto x = 10;     // x is a number
x = 20;          // Valid - assigning another number
x = "Hello";     // Error: x is number, cannot assign string

auto name = "Rubber Duck";  // name is a string
name = "Programming";       // Valid - assigning another string
name = 42;                  // Error: name is string, cannot assign number
```

This type safety prevents common programming errors and makes the code more robust.

### String Escape Sequences

RD Script supports common escape sequences in string literals to represent special characters:

1. **`\n`** - Newline character
2. **`\t`** - Tab character
3. **`\r`** - Carriage return
4. **`\\`** - Literal backslash
5. **`\"`** - Literal double quote

These escape sequences work in both regular strings and format strings:

```rubberduck
// Regular strings with escape sequences
auto multiline = "Line 1\nLine 2\nLine 3";
auto tabbed = "Column 1\tColumn 2\tColumn 3";
auto path = "C:\\Users\\Documents\\file.txt";
auto quote = "She said \"Hello World!\"";

// Format strings with escape sequences
auto name = "John";
auto age = 30;
auto formatted = $"Hello {name}!\nYou are {age} years old.";
```

Unrecognized escape sequences are preserved as literal characters:
```rubberduck
auto unknown = "Backslash-x: \x";  // Results in "Backslash-x: \x"
```

### For Loop Syntax

The for loop in RD Script follows C++ style syntax with the following structure:

```rubberduck
for (initialization; condition; increment) {
    // body statements
}
```

Example:
```rubberduck
for (auto i = 0; i < 10; i++) {
    display "Number: ", i;
}
```

The for loop consists of three parts:
1. **Initialization**: Executed once before the loop starts (typically variable declaration)
2. **Condition**: Evaluated before each iteration; loop continues if true
3. **Increment**: Executed at the end of each iteration

All three parts are optional. An infinite loop can be created with:
```rubberduck
for (;;) {
    // infinite loop body
}
```

### Increment and Decrement Operators

RD Script supports both prefix and postfix increment (`++`) and decrement (`--`) operators:

1. **Prefix Increment**: `++variable`
   - Increments the variable and returns the new value
   - Example: `++i` increments i and returns the incremented value

2. **Postfix Increment**: `variable++`
   - Returns the current value and then increments the variable
   - Example: `i++` returns the current value of i and then increments it

3. **Prefix Decrement**: `--variable`
   - Decrements the variable and returns the new value
   - Example: `--i` decrements i and returns the decremented value

4. **Postfix Decrement**: `variable--`
   - Returns the current value and then decrements the variable
   - Example: `i--` returns the current value of i and then decrements it

Examples:
```rubberduck
auto x = 5;
display ++x;  // Displays 6 (prefix increment)
display x;    // Displays 6

auto y = 5;
display y++;  // Displays 5 (postfix increment)
display y;    // Displays 6

auto z = 10;
display --z;  // Displays 9 (prefix decrement)
display z;    // Displays 9

auto w = 10;
display w--;  // Displays 10 (postfix decrement)
display w;    // Displays 9
```

These operators can be used in for loops for more concise code:
```rubberduck
for (auto i = 0; i < 10; i++) {
    display "Number: ", i;
}
```

### Assignment Expressions

RD Script supports assignment expressions using the `=` operator, but all variables must be declared with the `auto` keyword before they can be assigned to, and they maintain their declared type:

```rubberduck
auto x = 10;     // x is declared as a number
x = x + 5;       // Assigns 15 to x (valid - still a number)

auto name = "John";  // name is declared as a string
name = "Jane";       // Valid - assigning another string
```

Assignment expressions can be used in statements and can be chained:

```rubberduck
auto a = 5;
auto b = 10;
auto c = 15;
a = b = c;  // Assigns 15 to both a and b
```

Attempting to assign a value of a different type will result in an error:
```rubberduck
auto x = 10;     // x is a number
x = "Hello";     // Error: x is number, cannot assign string
```

Attempting to assign to an undeclared variable will result in an error:
```rubberduck
x = 10;  // Error: Variable 'x' must be declared with 'auto' keyword before use
```

### Benchmarking

RD Script includes a built-in benchmarking tool that measures execution time with high precision using C++ chrono library. The benchmark statement executes a block of code and reports the execution time in multiple units.

Syntax:
```rubberduck
benchmark {
    // code to benchmark
}
```

Example:
```rubberduck
benchmark {
    for (auto i = 0; i < 1000; i++) {
        display "Iteration: ", i;
    }
}
```

The benchmark statement will output timing information in the following format:
```
Benchmark Results:
  Execution time: X nanoseconds
  Execution time: Y microseconds
  Execution time: Z milliseconds
  Execution time: W seconds
```

This provides an easy way to measure performance, similar to Python's timeit module but with the speed of C++ chrono.
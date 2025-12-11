# Rubber Duck Scripting Language

Name: Rubber Duck Script
Extension: .rd

## Features

* Static typing with C-like syntax
* Automatic memory management
* Standard programming constructs (loops, branching, etc.)
* Dynamic array and linked list support( TODO )

## Coding Guidelines

This project follows specific C++ coding guidelines documented in [CODING_GUIDELINES.md](CODING_GUIDELINES.md):

* Struct names: `t_StructName`
* Variable names: `var_name`
* Function names: `FunctionName`
* Braces style: Allman style (opening brace on the next line)

## Memory Management

See [MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md) for a detailed explanation of how memory is managed in this interpreter

## Current Implementation Status

The interpreter currently supports:
* Variable declarations (`auto name = value;`)
* Expressions with arithmetic operations (`+`,`-`, `*`, `/`)
* Comparison operations (`==`, `!=`, `<`, `>`, `<=`, `>=`)
* Logical operations (`!`)
* Display statements with comma-separated values (`display(value1, value2, ...);`)
* Input statements using standard C++ input wrapped in `getin(variable_name);`
* Grouping with parentheses
* String and number literals with escape sequence support (`\n`, `\t`, `\r`, `\\`, `\"`)
* Variable references
* Control structures (`if`, `else`, `for`)

Not yet implemented:
* Loops (`while`)
* Functions
* Data structures (dynamic arrays, linked lists)

## Building the Project

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Running Scripts

```bash
./rubberduck script.rd
```

## Language Examples

### Hello World
```cpp
display("Hello, Rubber Duck!");
```

### Variables
```cpp
auto name = "Rubber Duck";
auto age = 5;
display name, "is", age, "years old";
```

### Expressions
```cpp
auto a = 10;
auto b = 20;
auto difference = a - b;
auto product = a * b;
display($"Difference: {difference}");
display "Product:", product;
```

### For Loop
```cpp
// Simple for loop counting from 0 to 9
for (auto i = 0; i < 10; ++i)
{
    display "Number: ", i;
}
```

### If-Else Statements
```cpp
auto x = 10;
auto y = 20;

if (x < y)
{
    display "x is less than y";
}

if (x > y)
{
    display "x is greater than y";
}
else
{
    display "x is not greater than y";
}
```

### Function 
```cpp
fun Testing(auto arg1, auto arg2)
{
    return some_value;
}
```

### User Input with getin()
The `getin(variable_name);` statement reads a single token from standard input and stores it in an already-declared variable.

```cpp
auto age = 0;
display "Enter your age:";
getin(age);
display "You are", age, "years old.";

auto name = "";
display "Enter your name:";
getin(name);
display "Hello,", name, "!";
```

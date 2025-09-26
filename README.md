# Rubber Duck Scripting Language

Name: Rubber Duck Script
Extension: .rd

## Features

* Static typing with C-like syntax
* Automatic memory management
* Standard programming constructs (loops, branching, etc.)
* Dynamic array and linked list support
* Designed for modern system automation

## Project Structure

```
.
├── CMakeLists.txt              # Build configuration
├── README.md                   # This file
├── CODING_GUIDELINES.md        # C++ coding standards for the project
├── MEMORY_MANAGEMENT.md        # Memory management explanation
├── main.cpp                    # Main entry point
├── include/                    # Header files
│   ├── Token.h                 # Token definitions
│   ├── Lexer.h                 # Lexical analyzer
│   ├── Parser.h                # Parser
│   ├── AST.h                   # Abstract Syntax Tree nodes
│   └── Interpreter.h           # Interpreter
├── src/                        # Source files
│   ├── core/
│   │   └── Lexer.cpp           # Lexical analyzer implementation
│   ├── parser/
│   │   └── Parser.cpp          # Parser implementation
│   ├── interpreter/
│   │   └── Interpreter.cpp     # Interpreter implementation
├── templates/                  # Example scripts
│   ├── hello_world.rd          # Hello world example
│   ├── variables.rd            # Variable usage
│   ├── static_typing.rd        # Static typing example
│   ├── simple_example.rd       # More comprehensive example
│   ├── control_structures.rd   # Control structures (not yet implemented)
│   ├── data_structures.rd      # Data structures (not yet implemented)
│   ├── functions.rd            # Function definitions (not yet implemented)
│   └── system_automation.rd    # System automation examples (not yet implemented)
└── Test.rd                     # Test file
```

## Coding Guidelines

This project follows specific C++ coding guidelines documented in [CODING_GUIDELINES.md](CODING_GUIDELINES.md):

* Struct names: `t_StructName`
* Variable names: `var_name`
* Function names: `FunctionName`
* Braces style: Allman style (opening brace on the next line)

## Memory Management

See [MEMORY_MANAGEMENT.md](MEMORY_MANAGEMENT.md) for a detailed explanation of how memory is managed in this interpreter, especially helpful for beginner developers.

## Current Implementation Status

The interpreter currently supports:
* Variable declarations (`auto name = value;`)
* Expressions with arithmetic operations (`-`, `*`, `/`)
* Comparison operations (`==`, `!=`, `<`, `>`, `<=`, `>=`)
* Logical operations (`!`)
* Display statements with comma-separated values (`display(value1, value2, ...);`)
* Grouping with parentheses
* String and number literals
* Variable references
* Control structures (`if`, `else`)

Not yet implemented:
* Loops (`while`, `for`)
* Functions
* Data structures (dynamic arrays, linked lists)
* System automation features

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
```rubberduck
display("Hello, Rubber Duck!");
```

### Variables
```rubberduck
auto name = "Rubber Duck";
auto age = 5;
display(name, "is", age, "years old");
```

### Expressions
```rubberduck
auto a = 10;
auto b = 20;
auto difference = a - b;
auto product = a * b;
display("Difference:", difference);
display("Product:", product);
```

### If-Else Statements
```rubberduck
auto x = 10;
auto y = 20;

if (x < y) {
    display "x is less than y";
}

if (x > y) {
    display "x is greater than y";
} else {
    display "x is not greater than y";
}
```
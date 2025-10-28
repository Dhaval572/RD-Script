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
* Expressions with arithmetic operations (`-`, `*`, `/`)
* Comparison operations (`==`, `!=`, `<`, `>`, `<=`, `>=`)
* Logical operations (`!`)
* Display statements with comma-separated values (`display(value1, value2, ...);`)
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
display(name, "is", age, "years old");
```

### Expressions
```cpp
auto a = 10;
auto b = 20;
auto difference = a - b;
auto product = a * b;
display("Difference:", difference);
display("Product:", product);
```

### String Escape Sequences
```cpp
// Escape sequences in regular strings
auto multiline = "Line 1\nLine 2\nLine 3";
auto tabbed = "Column 1\tColumn 2\tColumn 3";
auto path = "C:\\Users\\Documents\\file.txt";
auto quote = "She said \"Hello World!\"";

display(multiline);
display(tabbed);
display(path);
display(quote);

// Escape sequences also work in format strings
auto name = "John";
auto age = 30;
auto formatted = $"Hello {name}!\nYou are {age} years old.";
display(formatted);
```

### For Loop
```cpp
// Simple for loop counting from 0 to 9
for (auto i = 0; i < 10; i = i + 1) {
    display "Number: ", i;
}

// Infinite loop with break
auto j = 0;
for (;;) {
    display "j = ", j;
    j = j + 1;
    if (j >= 3) {
        break;
    }
}
```

### If-Else Statements
```cpp
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

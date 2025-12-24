# Rubber Duck Scripting Language

A statically-typed scripting language with C-like syntax, featuring automatic memory management and intuitive programming constructs.

## Overview

**Name:** Rubber Duck Script
**Extension:** `.rd`
**Philosophy:** Simple, expressive, and efficient

## Key Features

* **Static typing** with type inference using `auto` keyword
* **Automatic memory management** - no manual allocation/deallocation
* **C-like syntax** - familiar and easy to learn
* **Performance benchmarking** - built-in timing utilities
* **Standard I/O support** - `display()` and `getin()` for user interaction

## Quick Start

### Building the Project

```bash
./rd.bat
```

> **Note:** When making changes to the project, delete the `build` folder before rebuilding.

### Running Scripts

```bash
./rd.bat script.rd
```

## Language Syntax

### Variables and Data Types

```cpp
auto name = "Rubber Duck";
auto age = 5;
auto price = 19.99;
auto is_active = true;
```

### Arithmetic Operators

```cpp
auto sum = 10 + 5;        // Addition
auto diff = 10 - 5;       // Subtraction
auto product = 10 * 5;    // Multiplication
auto quotient = 10 / 5;   // Division
```

### Comparison Operators

```cpp
auto is_equal = (a == b);
auto not_equal = (a != b);
auto less_than = (a < b);
auto greater_than = (a > b);
auto less_or_equal = (a <= b);
auto greater_or_equal = (a >= b);
```

### Logical Operators

```cpp
auto and_result = (x > 5 && y < 10);   // AND
auto or_result = (x == 0 || y == 0);   // OR
auto not_result = !(x > 5);            // NOT
```

### Output with display()

**Method 1: String interpolation with `prefix`**

```cpp
auto name = "Duck";
auto age = 5;
display($"Hello, my name is {name} and I am {age} years old!");
display($"Result: {10 + 20}");
display($"{name} says: 'Quack!'");
```

**Method 2: Comma-separated concatenation**

```cpp
display "Hello, World!";
display "Name:", name, "Age:", age;
display "Sum of ", a, " and ", b, " is: ", a + b;
display name, " is ", age, " years old";
```

**Method 3: Simple string literal**

```cpp
display("Simple message");
```

### Input with getin()

```cpp
auto user_input = "";
display("Enter your name:");
getin(user_input);
display ($"Hello {user_input}!");
```

### Conditional Statements

```cpp
if (temperature > 30)
{
    display("It's hot outside!");
}
else if (temperature > 20)
{
    display("Nice weather!");
}
else
{
    display("It's cold!");
}
```

### For Loops

**Standard for loop:**

```cpp
for (auto i = 0; i < 10; ++i)
{
    display "Iteration:", i;
}
```

**Infinite for loop:**

```cpp
for (;;)
{
    display("Running forever...");
    if (some_condition)
    {
        break;
    }
}
```

**Conditional for loop:**

```cpp
for (auto i = 0; i < 100; i++)
{
    if (i == 50)
    {
        break;      // Exit the loop
    }
    
    if (i % 2 == 0)
    {
        continue;   // Skip even numbers
    }
    
    display "Odd number:", i;
}
```

### Break and Continue

```cpp
for (auto i = 0; i < 10; ++i)
{
    if (i == 5)
    {
        break;      // Exit loop when i equals 5
    }
    
    if (i % 2 == 0)
    {
        continue;   // Skip to next iteration for even numbers
    }
    
    display(i);     // Only displays odd numbers: 1, 3
}
```

### Functions

```cpp
fun CalculateSum(auto a, auto b)
{
    auto result = a + b;
    return result;
}

auto total = CalculateSum(10, 20);
display "Total:", total;
```

## Benchmark Feature

### Benchmark Blocks

Measure execution time of code blocks with automatic formatting:

```cpp
benchmark
{
    auto sum = 0;
    for (auto i = 0; i < 1000000; ++i)
    {
        sum = sum + i;
    }
    display "Sum:", sum;
}
```

**Output format:**

* Nanoseconds (ns)
* Microseconds (μs)
* Milliseconds (ms)
* Seconds (s)

### Performance Comparison

**Example Benchmark Results:**
![Benchmark Output](Benchmark/print.jpeg)

**Performance Test: Printing 1,000,000 numbers**

**Python Execution:**
```python
start_time = time.time()
for i in range(0, 1000000):
    print(i)
end_time = time.time()
print(f"Time taken: {end_time - start_time}")
```
**Result:** 348.94 seconds

**Rubber Duck Script Execution:**
```cpp
benchmark
{
    for(auto i = 0; i < 1000000; i++)
    {
        display(i);
    }
}
```
**Result:** 23.4382 seconds

**Performance Improvement:** **14.9x faster than Python!**

## String Features

**Escape sequences:**

```cpp
display("Line 1\nLine 2");          // Newline
display("Column1\tColumn2");        // Tab
display("Quote: \"Hello\"");        // Escaped quotes
display("Backslash: \\");           // Escaped backslash
```

## Example Programs

### Calculator

```cpp
auto num1 = 0;
auto num2 = 0;

display("Enter first number:");
getin(num1);

display("Enter second number:");
getin(num2);

display($"Sum:", {num1 + num2});
display($"Difference:", {num1 - num2});
display($"Product:", {num1 * num2});
display($"Quotient:", {num1 / num2});
```

### FizzBuzz

```cpp
for (auto i = 1; i <= 100; ++i)
{
    if (i % 15 == 0)
    {
        display("FizzBuzz");
    }
    else if (i % 3 == 0)
    {
        display("Fizz");
    }
    else if (i % 5 == 0)
    {
        display("Buzz");
    }
    else
    {
        display(i);
    }
}
```

## Implementation Status

### ✅ Current Features

* Variable declarations with `auto`
* Arithmetic operators (`+`, `-`, `*`, `/`)
* Comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`)
* Logical operators (`&&`, `||`, `!`)
* Control flow (`if`, `else if`, `else`)
* For loops (standard, infinite, conditional)
* Loop control (`break`, `continue`)
* Functions with return values
* Input/output (`getin()`, `display()`)
* String literals with escape sequences
* Benchmark blocks for performance measurement

## Coding Standards

For detailed coding conventions and guidelines, see [CODING_GUIDELINES.md](Documentation/Cpp_documentation/CODING_GUIDELINES.md).

## Memory Management

Rubber Duck Script features automatic memory management. For implementation details, refer to [MEMORY_MANAGEMENT.md](Documentation/Cpp_documentation/MEMORY_MANAGEMENT_IN_RD_SCRIPT.md)   

## Contributing

Contributions are welcome! Please follow the coding guidelines and ensure all tests pass before submitting a pull request.

---

**Happy scripting with Rubber Duck!** 🦆

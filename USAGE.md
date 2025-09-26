# RD Script Runner

This directory contains a simple batch script to run RD Script files with one command.

## Usage

Open Command Prompt (cmd) in this directory and run:

```
run_rd.bat <script_file.rd>
```

### Examples:

```
run_rd.bat test.rd
run_rd.bat comprehensive_test.rd
run_rd.bat templates/control_structures.rd
```

## How it works

1. The script checks if the RD Script interpreter is already built
2. If not, it automatically builds the interpreter using CMake
3. Runs the interpreter with your script file

## Prerequisites

- CMake (version 3.10 or higher)
- A C++ compiler (Visual Studio, MinGW, etc.)
- Command Prompt (cmd) - comes with Windows

## Notes

- The first run may take a moment as it builds the interpreter
- Subsequent runs will be faster as the build is cached
- The build files are stored in the `build` directory

## Language Features

### Control Structures

The language now supports if-else statements:

```rubberduck
if (condition) {
    // code to execute if condition is true
} else {
    // code to execute if condition is false
}
```

Logical operators are also supported:
- `&&` for logical AND
- `||` for logical OR
- `!` for logical NOT

Example:
```rubberduck
auto temperature = 75;
auto is_raining = false;

if (temperature > 70 && !is_raining) {
    display "Perfect weather for outdoor activities!";
}
```

### Display Statement

The display statement outputs values to the console. It supports comma-separated values without parentheses:

```rubberduck
display "Hello, World!";
display "The value of x is", x;
display "Result:", a + b, "units";
```

Note: Parentheses are not used with the display statement. Values are separated by commas.
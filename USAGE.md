# RD Script Runner

This directory contains a simple batch script to run RD Script files with one command.

## Usage

Open Command Prompt (cmd) in this directory and run:

```
run_rd.bat <script_file.rd>
```

### Examples:

```cmd
./run_rd.bat test.rd
./run_rd.bat comprehensive_test.rd
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
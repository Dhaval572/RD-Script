cmake_minimum_required(VERSION 3.10)
project(RubberDuck)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/core/Lexer.cpp
    src/parser/Parser.cpp
    src/interpreter/Interpreter.cpp
    main.cpp
)

# Create executable
add_executable(rubberduck ${SOURCES})

# Compiler-specific options
if(MSVC)
    target_compile_options(rubberduck PRIVATE /W4)
else()
    target_compile_options(rubberduck PRIVATE -Wall -Wextra -pedantic)
endif()
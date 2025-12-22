cmake_minimum_required(VERSION 3.27)
project(RubberDuck LANGUAGES CXX)

add_executable(
    rubberduck
    src/core/Lexer.cpp
    src/core/ErrorHandling.cpp
    src/core/ASTContext.cpp
    src/parser/Parser.cpp
    src/interpreter/Interpreter.cpp
    main.cpp
)


# Require modern C++ (can be raised to cxx_std_23 when desired)
target_compile_features(rubberduck PRIVATE cxx_std_20)


# Include directories (scoped to the target)
target_include_directories(
    rubberduck
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# Compiler-specific warnings
if (MSVC)
target_compile_options(rubberduck PRIVATE /W4)
else()
target_compile_options(rubberduck PRIVATE -Wall -Wextra -pedantic)
endif()
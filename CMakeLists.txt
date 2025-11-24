cmake_minimum_required(VERSION 3.27)
project(RubberDuck)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(include ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(SOURCES
    src/core/Lexer.cpp
    src/core/ErrorHandling.cpp
    src/parser/Parser.cpp
    src/interpreter/Interpreter.cpp
    main.cpp
)

add_executable(rubberduck ${SOURCES})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    orcjit
    native
    mcjit
    support
)

target_link_libraries(rubberduck ${LLVM_LIBS})
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/Interpreter.h"

std::string ReadFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[])
{
    if (argc < 2) 
    {
        std::cerr << "Usage: rubberduck <script.rd>\n";
        return 1;
    }

    std::string source = ReadFile(argv[1]);
    if (source.empty())
    {
        std::cerr << "Error: file is empty or could not be read.\n";
        return 1; 
    }

    try 
    {
        // Lexical analysis
        t_Lexer lexer(source);
        std::vector<t_Token> tokens = lexer.ScanTokens();

        // Parsing with memory pool optimization
        t_Parser parser(tokens);
        std::vector<t_Stmt*> statements = parser.Parse();

        // Interpretation
        t_Interpreter interpreter;
        interpreter.Interpret(statements);

        // No need to manually delete statements - they're managed by the memory pool
        // Reset the pools for the next run
        t_Parser::ResetPools();
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
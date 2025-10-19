#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/Interpreter.h"
#include "../include/ErrorHandling.h"

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

    // Lexical analysis
    t_Lexer lexer(source);
    t_ParsingResult tokens_result = lexer.ScanTokens();
    if (!tokens_result.HasValue())
    {
        ReportError(tokens_result.Error());
        return 1;
    }
    std::vector<t_Token> tokens = tokens_result.Value();

    // Parsing with memory pool optimization
    t_Parser parser(tokens);
    t_Expected<std::vector<t_Stmt*>, t_ErrorInfo> statements_result = parser.Parse();
    if (!statements_result.HasValue())
    {
        ReportError(statements_result.Error());
        t_Parser::ResetPools();
        return 1;
    }
    std::vector<t_Stmt*> statements = statements_result.Value();

    // Interpretation
    t_Interpreter interpreter;
    t_InterpretationResult interpret_result = interpreter.Interpret(statements);
    if (!interpret_result.HasValue())
    {
        // Error already reported in Interpret method
        t_Parser::ResetPools();
        return 1;
    }

    // No need to manually delete statements - they're managed by the memory pool
    // Reset the pools for the next run
    t_Parser::ResetPools();
    
    return 0;
}
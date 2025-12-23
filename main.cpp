#include <fstream>
#include <iostream>
#include <memory>
#include <rubberduck/Lexer.h>
#include <rubberduck/Parser.h>
#include <rubberduck/Interpreter.h>
#include <rubberduck/ErrorHandling.h>
#include <rubberduck/ASTContext.h>

static std::string ReadFile(const std::string &filename)
{
    if (filename.find(".rd") == std::string::npos)
    {
        std::cerr << "Error: File name must contain .rd extension.\n";
        return "";
    }

    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return "";
    }

    // Check size and decide whether to pre-allocate
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::string content;
    
    // Optimize: pre-allocate only for files > 8KB
    if (size > 8192)
    {
        content.reserve(size);
    }
    
    content.assign
    (
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
    
    return content;
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

    // Create AST context for memory pool management
    t_ASTContext ast_context;

    // Parsing with memory pool optimization
    t_Parser parser(tokens, ast_context);
    t_Expected<std::vector<t_PoolPtr<t_Stmt>>, t_ErrorInfo> statements_result = 
    parser.Parse();
    if (!statements_result.HasValue())
    {
        ReportError(statements_result.Error());
        return 1;
    }
    std::vector<t_PoolPtr<t_Stmt>> statements = std::move(statements_result.Value());

    // Interpretation
    t_Interpreter interpreter;
    t_InterpretationResult interpret_result = interpreter.Interpret(statements);
    if (!interpret_result.HasValue())
    {
        // Error already reported in Interpret method
        return 1;
    }
    
    return 0;
}
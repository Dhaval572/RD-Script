#include "../include/ErrorHandling.h"
#include <iostream>

void ReportError(const t_ErrorInfo& error)
{
    std::string_view error_type;
    switch (error.type)
    {
        case t_ErrorType::LEXING_ERROR:
            error_type = "Lexing Error";
            break;
        case t_ErrorType::PARSING_ERROR:
            error_type = "Parsing Error";
            break;
        case t_ErrorType::RUNTIME_ERROR:
            error_type = "Runtime Error";
            break;
        case t_ErrorType::TYPE_ERROR:
            error_type = "Type Error";
            break;
        default:
            error_type = "Unknown Error";
            break;
    }

    if (error.line > 0)
    {
        std::cerr << "[" << error_type << "] " << error.message 
                  << " at line " << error.line;
        if (error.column > 0)
        {
            std::cerr << ", column " << error.column;
        }
        std::cerr << std::endl;
    }
    else
    {
        std::cerr << "[" << error_type << "] " << error.message << std::endl;
    }
}
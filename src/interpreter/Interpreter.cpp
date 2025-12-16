#include "Interpreter.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <stack>
#include <cctype>
#include <iomanip>
#include <unordered_set>
#include <limits>
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/ErrorHandling.h"

// Helper: Assigns variable to its visible scope.
static void AssignToVisibleVariable
(
    const std::string &name, 
    const t_TypedValue &value, 
    std::unordered_map<std::string, 
    t_TypedValue> &environment, 
    std::vector<std::unordered_map<std::string, 
    t_TypedValue>> &scope_stack
)
{
    for
    (
        auto scope_it = scope_stack.rbegin(); 
        scope_it != scope_stack.rend(); 
        ++scope_it
    )
    {
        auto &scope = *scope_it;
        if (scope.find(name) != scope.end())
        {
            scope[name] = value;
            return;
        }
    }
// Not found in stack: update current environment
    environment[name] = value;
}

t_Interpreter::t_Interpreter()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    // Initialize with global scope
    PushScope();
    m_BufferOutput = false;
    m_OutputBuffer.clear();
}

void t_Interpreter::WriteOutput(const std::string& text)
{
    if (m_BufferOutput)
    {
        m_OutputBuffer.append(text);

        static const std::size_t flush_threshold = 4 * 1024 * 1024;
        if (m_OutputBuffer.size() >= flush_threshold)
        {
            std::cout.write
            (
                m_OutputBuffer.data(),
                static_cast<std::streamsize>(m_OutputBuffer.size())
            );
            m_OutputBuffer.clear();
        }
    }
    else
    {
        std::cout.write
        (
            text.data(), 
            static_cast<std::streamsize>(text.size())
        );
    }
}

void t_Interpreter::FlushOutput()
{
    if (!m_OutputBuffer.empty())
    {
        std::cout.write
        (
            m_OutputBuffer.data(),
            static_cast<std::streamsize>(m_OutputBuffer.size())
        );
        std::cout.flush();
        m_OutputBuffer.clear();
    }
}

t_InterpretationResult t_Interpreter::Interpret
(
    const std::vector<t_Stmt *> &statements
)
{
    functions.clear();

    for (t_Stmt *statement : statements)
    {
        if (t_FunStmt *fun_stmt = dynamic_cast<t_FunStmt *>(statement))
        {
            functions[fun_stmt->name] = fun_stmt;
        }
    }

    for (t_Stmt *statement : statements)
    {
        try
        {
            t_Expected<int, t_ErrorInfo> result = Execute(statement);
            if (!result.HasValue())
            {
                // Report the error and stop execution
                ReportError(result.Error());
                return t_InterpretationResult(result.Error());
            }
        }
        catch (const std::string &control_flow)
        {
            // Handle misuse of break/continue outside of loops
            if (control_flow == "break" || 
                control_flow == "continue")
            {
                t_ErrorInfo err
                (
                    e_ErrorType::RUNTIME_ERROR,
                    control_flow == "break" ?
                    "'break' used outside of a loop" :
                    "'continue' used outside of a loop"
                );
                ReportError(err);
                return t_InterpretationResult(err);
            }
            // Convert to runtime error
            t_ErrorInfo err
            (
                e_ErrorType::RUNTIME_ERROR,
                "Unhandled control-flow exception"
            );
            ReportError(err);
            return t_InterpretationResult(err);
        }
        catch (const std::exception& ex)
        {
            t_ErrorInfo err
            (
                e_ErrorType::RUNTIME_ERROR,
                std::string("Unhandled std::exception: ") + ex.what()
            );
            ReportError(err);
            return t_InterpretationResult(err);
        }
        catch (...)
        {
            t_ErrorInfo err
            (
                e_ErrorType::RUNTIME_ERROR,
                "Unhandled unknown exception during execution"
            );
            ReportError(err);
            return t_InterpretationResult(err);
        }
    }
    
    return t_InterpretationResult(0); // Success represented by 0
}

// Scope management functions
void t_Interpreter::PushScope()
{
    // Save the current state of the environment
    scope_stack.emplace_back(environment);
}

void t_Interpreter::PopScope()
{
    if (!scope_stack.empty()) 
    {
        // Restore the environment to its previous state
        environment = std::move(scope_stack.back());
        scope_stack.pop_back();
    }
}

t_Expected<int, t_ErrorInfo> t_Interpreter::DeclareVariable
(
    const std::string& name, int line
)
{
    // Check if variable is already declared in current scope
    // We do this by checking if it exists in the current environment
    // and comparing with the previous scope's environment if it exists
    if (!environment.count(name)) 
    {
        // Variable not declared yet, this is fine
        return t_Expected<int, t_ErrorInfo>(0); // Success
    }
    
    // Variable already exists, check if it was declared in current scope
    if (!scope_stack.empty()) 
    {
        const auto& previous_scope = scope_stack.back();
        if (previous_scope.find(name) == previous_scope.end()) 
        {
            // Variable exists but was declared in current scope - this is an error
            return t_Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Variable '" + name + "' has already been declared in this scope", 
                    line
                )
            );
        }
    }
    
    // Variable exists but was declared in a previous scope - this is shadowing, which is allowed
    return t_Expected<int, t_ErrorInfo>(0); // Success
}

// Helper function to format numbers (removes trailing zeros)
std::string t_Interpreter::FormatNumber(double value)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(15) << value;
    std::string str = oss.str();
    
    // Remove trailing zeros and decimal point if not needed
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    str.erase(str.find_last_not_of('.') + 1, std::string::npos);
    
    return str;
}

// Helper function to detect the type of a value
e_ValueType t_Interpreter::DetectType(const std::string& value)
{
    if (value == "nil")
    {
        return e_ValueType::NIL;
    }
    
    if (value == "true" || value == "false")
    {
        return e_ValueType::BOOLEAN;
    }
    
    // Check if it's a number (integer or floating point)
    if (!value.empty())
    {
        bool is_number = true;
        bool has_decimal = false;
        size_t start = 0;
        
        // Handle negative numbers
        if (value[0] == '-')
        {
            start = 1;
        }
        
        // Check each character
        for (size_t i = start; i < value.length(); i++)
        {
            if (value[i] == '.')
            {
                if (has_decimal)
                {
                    is_number = false;
                    break;
                }
                has_decimal = true;
            }
            else if (!std::isdigit(value[i]))
            {
                is_number = false;
                break;
            }
        }
        
        // Make sure we have at least one digit
        if (is_number && value.length() > start)
        {
            return e_ValueType::NUMBER;
        }
    }
    
    // Default to string for all other values
    return e_ValueType::STRING;
}

// Helper function to check if a string represents an integer
bool t_Interpreter::IsInteger(const std::string& value)
{
    if (value.empty()) return false;
    
    size_t start = 0;
    if (value[0] == '-' || value[0] == '+') 
    {
        if (value.length() == 1) return false;
        start = 1;
    }
    
    for (size_t i = start; i < value.length(); i++) 
    {
        if (!std::isdigit(value[i])) 
        {
            return false;
        }
    }
    
    return true;
}

// Helper function to check if a string represents a float
bool t_Interpreter::IsFloat(const std::string& value)
{
    if (value.empty()) return false;
    
    int start = 0;
    if (value[0] == '-' || value[0] == '+') 
    {
        if (value.length() == 1) return false;
        start = 1;
    }
    
    bool has_decimal = false;
    for (size_t i = start; i < value.size(); i++) 
    {
        if (value[i] == '.') 
        {
            if (has_decimal) return false;
            has_decimal = true;
        } 
        else if (!std::isdigit(value[i])) 
        {
            return false;
        }
    }
    
    return has_decimal && (static_cast<size_t>(value.length()) > static_cast<size_t>(start + 1));
}

// Comparison operations are kept as they are more complex
t_Expected<bool, t_ErrorInfo> t_Interpreter::PerformComparison
(
    const t_TypedValue& left, 
    const e_TokenType op,
    const t_TypedValue& right
)
{
    // If both values have precomputed numeric values, use them directly
    if (left.has_numeric_value && right.has_numeric_value)
    {
        switch (op)
        {
        case e_TokenType::GREATER:
            return t_Expected<bool, t_ErrorInfo>
            (
                left.numeric_value > right.numeric_value
            );
        
        case e_TokenType::GREATER_EQUAL:
            return t_Expected<bool, t_ErrorInfo>
            (
                left.numeric_value >= right.numeric_value
            );
        
        case e_TokenType::LESS:
            return t_Expected<bool, t_ErrorInfo>
            (
                left.numeric_value < right.numeric_value
            );
        
        case e_TokenType::LESS_EQUAL:
            return t_Expected<bool, t_ErrorInfo>
            (
                left.numeric_value <= right.numeric_value
            );
        
        case e_TokenType::EQUAL_EQUAL:
            return t_Expected<bool, t_ErrorInfo>
            (
                left.numeric_value == right.numeric_value
            );
        
        case e_TokenType::BANG_EQUAL:
            return t_Expected<bool, t_ErrorInfo>
            (
                left.numeric_value != right.numeric_value
            );
        
        default:
            return t_Expected<bool, t_ErrorInfo>
            (
                false
            );

        }
    }
    
    // Fallback to string-based conversion
    try
    {
        double left_num = left.has_numeric_value ? 
        left.numeric_value : std::stod(left.value);

        double right_num = right.has_numeric_value ? 
        right.numeric_value : std::stod(right.value);
        
        switch (op)
        {
        case e_TokenType::GREATER:
            return t_Expected<bool, t_ErrorInfo>(left_num > right_num);
        case e_TokenType::GREATER_EQUAL:
            return t_Expected<bool, t_ErrorInfo>(left_num >= right_num);
        case e_TokenType::LESS:
            return t_Expected<bool, t_ErrorInfo>(left_num < right_num);
        case e_TokenType::LESS_EQUAL:
            return t_Expected<bool, t_ErrorInfo>(left_num <= right_num);
        case e_TokenType::EQUAL_EQUAL:
            return t_Expected<bool, t_ErrorInfo>(left_num == right_num);
        case e_TokenType::BANG_EQUAL:
            return t_Expected<bool, t_ErrorInfo>(left_num != right_num);
        default:
            return t_Expected<bool, t_ErrorInfo>(false);
        }
    }
    catch (...)
    {
        // For non-numeric comparisons, fallback to string comparison
        switch (op)
        {
        case e_TokenType::EQUAL_EQUAL:
            return t_Expected<bool, t_ErrorInfo>(left.value == right.value);
        case e_TokenType::BANG_EQUAL:
            return t_Expected<bool, t_ErrorInfo>(left.value != right.value);
        default:
            return t_Expected<bool, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Cannot compare non-numeric values"
                )
            );
        }
    }
}

t_Expected<int, t_ErrorInfo> t_Interpreter::Execute(t_Stmt *stmt)
{
    if (t_BlockStmt *block_stmt = dynamic_cast<t_BlockStmt *>(stmt))
    {
        // Push a new scope for the block
        PushScope();
		try
		{
			for (const auto &statement : block_stmt->statements)
			{
				t_Expected<int, t_ErrorInfo> result = Execute(statement.get());
				if (!result.HasValue())
				{
					PopScope(); // Clean up scope before returning
					return t_Expected<int, t_ErrorInfo>(result.Error());
				}

                // If a control signal was raised inside this block (break/continue),
                // stop executing further statements in this block and propagate upward.
                if (!control_signal.empty())
                {
                    // Pop the scope to clean up variables declared in this block
                    PopScope();
                    return t_Expected<int, t_ErrorInfo>(0);
                }
                
                // If we're returning from a function, stop executing further statements
                // and propagate the return signal upward.
                if (is_returning)
                {
                    PopScope();
                    return t_Expected<int, t_ErrorInfo>(0);
                }
			}

			// Pop the block scope, which will automatically clean up variables declared in this scope
			PopScope();
		}
		catch (...)
		{
			// Ensure scope is popped and convert to runtime error
			PopScope();
			return t_Expected<int, t_ErrorInfo>
			(
				t_ErrorInfo
				(
					e_ErrorType::RUNTIME_ERROR,
					"Unhandled exception in block"
				)
			);
		}
    }
    else if (dynamic_cast<t_BreakStmt *>(stmt))
    {
        // Validate that we're inside a loop
        if (loop_depth <= 0)
        {
            return t_Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "'break' used outside of a loop"
                )
            );
        }
        // Signal break without throwing
        control_signal = "break";
        return t_Expected<int, t_ErrorInfo>(0);
    }
    else if (dynamic_cast<t_ContinueStmt *>(stmt))
    {
        // Validate that we're inside a loop
        if (loop_depth <= 0)
        {
            return t_Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "'continue' used outside of a loop"
                )
            );
        }
        // Signal continue without throwing
        control_signal = "continue";
        return t_Expected<int, t_ErrorInfo>(0);
    }
    else if (t_IfStmt *if_stmt = dynamic_cast<t_IfStmt *>(stmt))
    {
        t_Expected<std::string, t_ErrorInfo> condition_result = Evaluate
        (
            if_stmt->condition.get()
        );
        if (!condition_result.HasValue())
        {
            return t_Expected<int, t_ErrorInfo>(condition_result.Error());
        }
        
        if (IsTruthy(condition_result.Value()))
        {
            t_Expected<int, t_ErrorInfo> then_result = Execute(if_stmt->then_branch.get());
            if (!then_result.HasValue())
            {
                return then_result;
            }
            
            // If we're returning from a function, propagate the return
            if (is_returning)
            {
                return t_Expected<int, t_ErrorInfo>(0);
            }
        }
        else if (if_stmt->else_branch)
        {
            t_Expected<int, t_ErrorInfo> else_result = 
            Execute(if_stmt->else_branch.get());

            if (!else_result.HasValue())
            {
                return else_result;
            }
            
            // If we're returning from a function, propagate the return
            if (is_returning)
            {
                return t_Expected<int, t_ErrorInfo>(0);
            }
        }
    }
    else if (t_ForStmt *for_stmt = dynamic_cast<t_ForStmt *>(stmt))
    {
        // Check for ultra-fast accumulation loop pattern: for (auto i = 0; i < N; i++) { var += i; }
        if (IsSimpleAccumulationLoop(for_stmt))
        {
            t_Expected<int, t_ErrorInfo> result = 
            ExecuteAccumulationLoop(for_stmt);

            if (!result.HasValue())
            {
                return result;
            }
        }
        // Check for specialized numeric loop pattern: for (auto i = 0; i < N; i++)
        else if (IsSimpleNumericLoop(for_stmt))
        {
            t_Expected<int, t_ErrorInfo> result = 
            ExecuteSimpleNumericLoop(for_stmt);

            if (!result.HasValue())
            {
                return result;
            }
        }
        else
        {
            // Store variables that existed before the loop
            std::unordered_set<std::string> pre_loop_variables;
            for (const auto& pair : environment) 
            {
                pre_loop_variables.insert(pair.first);
            }

            // Push a new scope for the loop
            PushScope();
            loop_depth++;
            try
            {
                // Execute initializer (if any)
                if (for_stmt->initializer)
                {
                    t_Expected<int, t_ErrorInfo> init_result = 
                    Execute(for_stmt->initializer.get());

                    if (!init_result.HasValue())
                    {
                        PopScope(); // Clean up scope before returning
                        loop_depth--;
                        return init_result;
                    }
                }

                // Loop while condition is true (or forever if no condition)
                while (true)
                {
                    // Check condition (if any)
                    if (for_stmt->condition)
                    {
                        t_Expected<std::string, t_ErrorInfo> condition_result = 
                        Evaluate(for_stmt->condition.get());
                        if (!condition_result.HasValue())
                        {
                            PopScope(); // Clean up scope before returning
                            loop_depth--;
                            return t_Expected<int, t_ErrorInfo>
                            (
                                condition_result.Error()
                            );
                        }

                        if (!IsTruthy(condition_result.Value()))
                        {
                            break; // Exit loop if condition is false
                        }
                    }
                    else if (!for_stmt->condition && !for_stmt->increment)
                    {
                        // Special case: for (;;) should run forever unless broken
                    }

                    // Execute body
                    {
                        control_signal.clear();
                        t_Expected<int, t_ErrorInfo> body_result = 
                        Execute(for_stmt->body.get());

                        if (!body_result.HasValue())
                        {
                            PopScope(); // Clean up scope before returning
                            loop_depth--;
                            return body_result;
                        }
                        if (control_signal == "break")
                        {
                            control_signal.clear();
                            break;
                        }
                        if (control_signal == "continue")
                        {
                            // Skip increment and start next iteration
                            control_signal.clear();
                            continue; // Continue to the next loop iteration without incrementing
                        }
                        
                        // If we're returning from a function, stop the loop and propagate the return
                        if (is_returning)
                        {
                            PopScope();
                            loop_depth--;
                            return t_Expected<int, t_ErrorInfo>(0);
                        }
                    }

                    // Execute increment (if any)
                    if (for_stmt->increment && control_signal.empty())
                    {
                        t_Expected<std::string, t_ErrorInfo> increment_result= 
                        Evaluate(for_stmt->increment.get());

                        if (!increment_result.HasValue())
                        {
                            PopScope(); // Clean up scope before returning
                            loop_depth--;
                            return t_Expected<int, t_ErrorInfo>(increment_result.Error());
                        }
                    }
                }

                // Before popping scope, preserve modifications to pre-existing variables
                std::unordered_map<std::string, t_TypedValue> modified_pre_existing;

                for (const auto& pair : environment)
                {
                    if (pre_loop_variables.count(pair.first) > 0)
                    {
                        modified_pre_existing[pair.first] = pair.second;
                    }
                }

                // Pop the loop scope
                PopScope();
                
                // Restore the modified values of pre-existing variables
                for (const auto& pair : modified_pre_existing)
                {
                    environment[pair.first] = pair.second;
                }
                
                loop_depth--;
            }
			catch (...)
			{
				// Ensure scope is cleaned and convert to runtime error
				PopScope();
				loop_depth--;
				return t_Expected<int, t_ErrorInfo>
				(
					t_ErrorInfo
					(
						e_ErrorType::RUNTIME_ERROR,
						"Unhandled exception in for-loop"
					)
				);
			}
        }
    }
    else if 
    (
        t_VarStmt *var_stmt = dynamic_cast<t_VarStmt *>(stmt)
    )
    {
        // Check if variable is already declared in current scope
        t_Expected<int, t_ErrorInfo> declare_result =
        DeclareVariable(var_stmt->name, 0); 

        if (!declare_result.HasValue())
        {
            return t_Expected<int, t_ErrorInfo>(declare_result.Error());
        }

        t_TypedValue typed_value;
        if (var_stmt->initializer)
        {
            t_Expected<std::string, t_ErrorInfo> value_result = 
            Evaluate(var_stmt->initializer.get());

            if (!value_result.HasValue())
            {
                return t_Expected<int, t_ErrorInfo>(value_result.Error());
            }
            
            std::string value = value_result.Value();
            e_ValueType type = DetectType(value);
            // Use optimized TypedValue constructor
            typed_value = t_TypedValue(value, type);
        }
        environment[var_stmt->name] = typed_value;
    }
    else if (t_DisplayStmt *display_stmt = dynamic_cast<t_DisplayStmt *>(stmt))
    {
        bool first = true;
        for (const auto &expr : display_stmt->expressions)
        {
            if (!first)
            {
                WriteOutput(" ");
            }
            first = false;

            t_Expected<std::string, t_ErrorInfo> value_result = 
            Evaluate(expr.get());

            if (!value_result.HasValue())
            {
                return t_Expected<int, t_ErrorInfo>(value_result.Error());
            }
            
            std::string value = value_result.Value();
            WriteOutput(value);
        }
        WriteOutput("\n");
    }
    else if (t_GetinStmt *getin_stmt = dynamic_cast<t_GetinStmt *>(stmt))
    {
        const std::string &var_name = getin_stmt->variable_name;

        auto it = environment.find(var_name);
        if (it == environment.end())
        {
            return t_Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "Variable '" + var_name +
                    "' must be declared with 'auto' keyword before use"
                )
            );
        }
        t_TypedValue &current_value = it->second;

        switch (current_value.type)
        {
        case e_ValueType::NUMBER:
            {
                double number_value = 0.0;
                if (!(std::cin >> number_value))
                {
                    std::cin.clear();
                    std::cin.ignore
                    (
                        std::numeric_limits<std::streamsize>::max(),
                        '\n'
                    );

                    return t_Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR,
                            "Failed to read number input for variable '" +
                            var_name + "'"
                        )
                    );
                }

                AssignToVisibleVariable
                (
                    var_name,
                    t_TypedValue(number_value),
                    environment,
                    scope_stack
                );
                break;
            }

        case e_ValueType::BOOLEAN:
            {
                bool bool_value = false;
                if (!(std::cin >> std::boolalpha >> bool_value))
                {
                    std::cin.clear();
                    std::cin.ignore
                    (
                        std::numeric_limits<std::streamsize>::max(),
                        '\n'
                    );

                    return t_Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR,
                            "Failed to read boolean input for variable '" +
                            var_name + "'"
                        )
                    );
                }

                std::string stored_value = bool_value ? "true" : "false";

                AssignToVisibleVariable
                (
                    var_name,
                    t_TypedValue(stored_value, e_ValueType::BOOLEAN),
                    environment,
                    scope_stack
                );
                break;
            }

        case e_ValueType::STRING:
            {
                std::string input;

                if (std::cin.peek() == '\n')
                {
                    std::cin.ignore
                    (
                        std::numeric_limits<std::streamsize>::max(),
                        '\n'
                    );
                }

                if (!std::getline(std::cin, input))
                {
                    std::cin.clear();

                    return t_Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR,
                            "Failed to read string input for variable '" +
                            var_name + "'"
                        )
                    );
                }

                AssignToVisibleVariable
                (
                    var_name,
                    t_TypedValue(input, e_ValueType::STRING),
                    environment,
                    scope_stack
                );
                break;
            }

        case e_ValueType::NIL:
        default:
            {
                std::string input;

                if (std::cin.peek() == '\n')
                {
                    std::cin.ignore
                    (
                        std::numeric_limits<std::streamsize>::max(),
                        '\n'
                    );
                }

                if (!std::getline(std::cin, input))
                {
                    std::cin.clear();

                    return t_Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR,
                            "Failed to read input for variable '" +
                            var_name + "'"
                        )
                    );
                }

                e_ValueType inferred_type = DetectType(input);

                AssignToVisibleVariable
                (
                    var_name,
                    t_TypedValue(input, inferred_type),
                    environment,
                    scope_stack
                );
                break;
            }
        }
    }
    else if 
    (
        t_BenchmarkStmt *benchmark_stmt = 
        dynamic_cast<t_BenchmarkStmt *>(stmt)
    )
    {
        bool previous_buffering = m_BufferOutput;
        m_BufferOutput = true;

        if (!previous_buffering)
        {
            m_OutputBuffer.clear();
            m_OutputBuffer.reserve(1024 * 1024);
        }

        // Record start time
        auto start_time = std::chrono::steady_clock::now();
        
        // Execute the benchmark body
        t_Expected<int, t_ErrorInfo> body_result = 
        Execute(benchmark_stmt->body.get());

        FlushOutput();
        m_BufferOutput = previous_buffering;
        
        if (!body_result.HasValue())
        {
            return body_result;
        }
        
        // Record end time
        auto end_time = std::chrono::steady_clock::now();
        
        // Calculate duration
        auto duration = 
        std::chrono::duration_cast<std::chrono::nanoseconds>
        (
            end_time - start_time
        );
        
        // Display benchmark results
        std::cout << "Benchmark Results:\n";

        std::cout << "  Execution time: " 
                  << duration.count() 
                  << " nanoseconds\n";
                  
        std::cout << "  Execution time: " 
                  << duration.count() / 1000.0 
                  << " microseconds\n";

        std::cout << "  Execution time: " 
                  << duration.count() / 1000000.0 
                  << " milliseconds\n";

        std::cout << "  Execution time: " 
                  << duration.count() / 1000000000.0 
                  << " seconds\n";
    }
    else if (dynamic_cast<t_EmptyStmt *>(stmt))
    {
        // Do nothing for empty statements
    }
    else if 
    (
        t_FunStmt *fun_stmt =
        dynamic_cast<t_FunStmt *>(stmt)
    )
    {
        // Native functions are implemented in C++.
        // The fun declaration is currently a no-op placeholder.
        (void)fun_stmt;
    }
    else if 
    (
        t_ExpressionStmt *expr_stmt = 
        dynamic_cast<t_ExpressionStmt *>(stmt)
    )
    {
        t_Expected<std::string, t_ErrorInfo> result =
        Evaluate(expr_stmt->expression.get());

        if (!result.HasValue())
        {
            return t_Expected<int, t_ErrorInfo>(result.Error());
        }
    }
    else if 
    (
        t_ReturnStmt *return_stmt = 
        dynamic_cast<t_ReturnStmt *>(stmt)
    )
    {
        // Evaluate the return value expression (if any)
        if (return_stmt->value)
        {
            t_Expected<std::string, t_ErrorInfo> result =
            Evaluate(return_stmt->value.get());

            if (!result.HasValue())
            {
                return t_Expected<int, t_ErrorInfo>(result.Error());
            }
            
            return_value = result.Value();
        }
        else
        {
            // No return value specified, return nil
            return_value = "nil";
        }
        
        // Set the returning flag to indicate we should stop execution
        is_returning = true;
        return t_Expected<int, t_ErrorInfo>(0);
    }
    
    return t_Expected<int, t_ErrorInfo>(0); // Success represented by 0
}

bool t_Interpreter::IsTruthy(const std::string &value)
{
    return value != "false" && value != "nil";
}

t_Expected<std::string, t_ErrorInfo> t_Interpreter::Evaluate(t_Expr *expr)
{
    if (t_LiteralExpr *literal = dynamic_cast<t_LiteralExpr *>(expr))
    {
        // Check if this is a format string based on token type
        if (literal->token_type == e_TokenType::FORMAT_STRING)
        {
            // Process format string
            std::string format_str = literal->value;
            std::string result = format_str;

            // Improved string replacement approach with better error handling
            size_t pos = 0;
            while ((pos = result.find('{')) != std::string::npos)
            {
                size_t end_pos = result.find('}', pos);
                if (end_pos != std::string::npos)
                {
                    std::string expression = 
                    result.substr(pos + 1, end_pos - pos - 1);

                    // Trim whitespace from expression
                    expression.erase(0, expression.find_first_not_of(" \t"));
                    expression.erase(expression.find_last_not_of(" \t") + 1);

                    // Evaluate the expression instead of just looking it up as a variable
                    t_Expected<std::string, t_ErrorInfo> expr_result = 
                    EvaluateFormatExpression(expression);
                    if (!expr_result.HasValue())
                    {
                        return t_Expected<std::string, t_ErrorInfo>
                        (
                            expr_result.Error()
                        );
                    }

                    result.replace
                    (
                        pos, end_pos - pos + 1, expr_result.Value()
                    );
                }
                else
                {
                    // No closing brace found, treat as literal
                    break;
                }
            }

            return t_Expected<std::string, t_ErrorInfo>(result);
        }

        return t_Expected<std::string, t_ErrorInfo>(literal->value);
    }

    if (t_GroupingExpr *grouping = dynamic_cast<t_GroupingExpr *>(expr))
    {
        return Evaluate(grouping->expression.get());
    }

    if (t_CallExpr *call_expr = dynamic_cast<t_CallExpr *>(expr))
    {
        // User-defined functions
        auto fun_it = functions.find(call_expr->callee);
        if (fun_it != functions.end())
        {
            t_FunStmt *fun_stmt = fun_it->second;

            if (call_expr->arguments.size() != fun_stmt->parameters.size())
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR,
                        "Function '" + fun_stmt->name +
                        "' called with wrong number of arguments"
                    )
                );
            }

            std::vector<t_TypedValue> argument_values;
            argument_values.reserve(call_expr->arguments.size());

            for (size_t i = 0; i < call_expr->arguments.size(); ++i)
            {
                t_Expected<std::string, t_ErrorInfo> arg_result =
                Evaluate(call_expr->arguments[i].get());
                if (!arg_result.HasValue())
                {
                    return arg_result;
                }

                std::string arg_value = arg_result.Value();
                e_ValueType arg_type = DetectType(arg_value);
                argument_values.emplace_back(arg_value, arg_type);
            }

            // Preserve modifications to pre-existing variables as in for-loops
            std::unordered_set<std::string> pre_call_variables;
            for (const auto &pair : environment)
            {
                pre_call_variables.insert(pair.first);
            }

            PushScope();

            try
            {
                // Bind parameters in the new scope
                for (size_t i = 0; i < fun_stmt->parameters.size(); ++i)
                {
                    const std::string &param_name = fun_stmt->parameters[i];
                    environment[param_name] = argument_values[i];
                }

                if (fun_stmt->body)
                {
                    // Reset the return flag before executing the function body
                    is_returning = false;
                    
                    t_Expected<int, t_ErrorInfo> body_result =
                    Execute(fun_stmt->body.get());

                    if (!body_result.HasValue())
                    {
                        PopScope();
                        return t_Expected<std::string, t_ErrorInfo>
                        (
                            body_result.Error()
                        );
                    }
                    
                    // If we were returning from the function, use the return value
                    if (is_returning)
                    {
                        is_returning = false; // Reset the flag
                        std::string result = return_value;
                        
                        // Preserve modifications to variables that existed before the call
                        std::unordered_map<std::string, t_TypedValue> modified_pre_existing;
                        for (const auto &pair : environment)
                        {
                            if (pre_call_variables.count(pair.first) > 0)
                            {
                                modified_pre_existing[pair.first] = pair.second;
                            }
                        }

                        PopScope();

                        for (const auto &pair : modified_pre_existing)
                        {
                            environment[pair.first] = pair.second;
                        }
                        
                        return t_Expected<std::string, t_ErrorInfo>(result);
                    }
                }

                // Preserve modifications to variables that existed before the call
                std::unordered_map<std::string, t_TypedValue> modified_pre_existing;
                for (const auto &pair : environment)
                {
                    if (pre_call_variables.count(pair.first) > 0)
                    {
                        modified_pre_existing[pair.first] = pair.second;
                    }
                }

                PopScope();

                for (const auto &pair : modified_pre_existing)
                {
                    environment[pair.first] = pair.second;
                }

                // User-defined functions currently return nil
                return t_Expected<std::string, t_ErrorInfo>("nil");
            }
            catch (...)
            {
                PopScope();
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR,
                        "Unhandled exception in function '" +
                        fun_stmt->name + "'"
                    )
                );
            }
        }

        return t_Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR,
                "Undefined function '" + call_expr->callee + "'",
                call_expr->line
            )
        );
    }

    if (t_UnaryExpr *unary = dynamic_cast<t_UnaryExpr *>(expr))
    {
        t_Expected<std::string, t_ErrorInfo> right_result = 
        Evaluate(unary->right.get());
        if (!right_result.HasValue())
        {
            return right_result;
        }
        
        std::string right = right_result.Value();
        switch (unary->op.type)
        {
        case e_TokenType::MINUS:
            // Optimize unary minus for numeric literals
            if 
            (
                t_LiteralExpr* right_literal = 
                dynamic_cast<t_LiteralExpr*>(unary->right.get())
            )
            {
                e_ValueType type = DetectType(right_literal->value);
                if (type == e_ValueType::NUMBER)
                {
                    try
                    {
                        double value = std::stod(right_literal->value);
                        return t_Expected<std::string, t_ErrorInfo>
                        (
                            FormatNumber(-value)
                        );
                    }
                    catch (...) {}
                }
            }
            return t_Expected<std::string, t_ErrorInfo>("-" + right);

        case e_TokenType::BANG:
            return t_Expected<std::string, t_ErrorInfo>
            (
                (right == "false" || right == "0") ? "true" : "false"
            );

        default:
            return t_Expected<std::string, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "Unsupported unary operator"
                )
            );
        }
    }

    if (t_PrefixExpr *prefix = dynamic_cast<t_PrefixExpr *>(expr))
    {
        if
        (
            t_VariableExpr *var_expr = 
            dynamic_cast<t_VariableExpr *>(prefix->operand.get())
        )
        {
            std::string var_name = var_expr->name;
            auto it = environment.find(var_name);
            
            if (it == environment.end())
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Undefined variable '" + var_name + "'"
                    )
                );
            }
            
            t_TypedValue& current_value = it->second;
            
            // Use direct numeric operations when possible
            if (current_value.has_numeric_value)
            {
                if (prefix->op.type == e_TokenType::PLUS_PLUS)
                {
                    double new_value = current_value.numeric_value + 1.0;
                    environment[var_name] = t_TypedValue(new_value);
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        std::to_string(new_value)
                    );
                }
                else if (prefix->op.type == e_TokenType::MINUS_MINUS)
                {
                    double new_value = current_value.numeric_value - 1.0;
                    environment[var_name] = t_TypedValue(new_value);
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        std::to_string(new_value)
                    );
                }
            }
            else
            {
                // Fallback to string-based operations
                try
                {
                    double num_value = std::stod(current_value.value);
                    
                    if (prefix->op.type == e_TokenType::PLUS_PLUS)
                    {
                        num_value += 1.0;
                        std::string new_value = std::to_string(num_value);
                        // Remove trailing zeros and decimal point if not needed
                        new_value.erase
                        (
                            new_value.find_last_not_of('0') + 1, std::string::npos
                        );
                        new_value.erase
                        (
                            new_value.find_last_not_of('.') + 1, std::string::npos
                        );
                        environment[var_name] =
                        t_TypedValue
                        (
                            new_value, e_ValueType::NUMBER
                        );
                        return t_Expected<std::string, t_ErrorInfo>(new_value);
                    }
                    else if (prefix->op.type == e_TokenType::MINUS_MINUS)
                    {
                        num_value -= 1.0;
                        std::string new_value = std::to_string(num_value);

                        // Remove trailing zeros and decimal point if not needed
                        new_value.erase
                        (
                            new_value.find_last_not_of('0') + 1, std::string::npos
                        );
                        new_value.erase
                        (
                            new_value.find_last_not_of('.') + 1, std::string::npos
                        );
                        environment[var_name] = 
                        t_TypedValue
                        (
                            new_value, e_ValueType::NUMBER
                        );
                        return t_Expected<std::string, t_ErrorInfo>(new_value);
                    }
                }
                catch (...)
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Cannot perform increment/decrement on non-numeric value"
                        )
                    );
                }
            }
        }
        return t_Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Prefix increment/decrement can only be applied to variables"
            )
        );
    }

    if (t_PostfixExpr *postfix = dynamic_cast<t_PostfixExpr *>(expr))
    {
        if 
        (
            t_VariableExpr *var_expr = 
            dynamic_cast<t_VariableExpr *>(postfix->operand.get())
        )
        {
            std::string var_name = var_expr->name;
            auto it = environment.find(var_name);
            
            if (it == environment.end())
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Undefined variable '" + var_name + "'"
                    )
                );
            }
            
            t_TypedValue& current_value = it->second;

            // Return the value BEFORE increment/decrement
            return_value = current_value.value;
            
            // Use direct numeric operations when possible
            if (current_value.has_numeric_value)
            {
                if (postfix->op.type == e_TokenType::PLUS_PLUS)
                {
                    double new_value = current_value.numeric_value + 1.0;
                    environment[var_name] = t_TypedValue(new_value);
                    
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        current_value.value
                    ); 
                }
                else if (postfix->op.type == e_TokenType::MINUS_MINUS)
                {
                    double new_value = current_value.numeric_value - 1.0;
                    environment[var_name] = t_TypedValue(new_value);
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        current_value.value
                    ); 
                }
            }
            else
            {
                // Fallback to string-based operations
                try
                {
                    double num_value = std::stod(current_value.value);
                    
                    if (postfix->op.type == e_TokenType::PLUS_PLUS)
                    {
                        num_value += 1.0;
                        std::string new_value = std::to_string(num_value);

                        // Remove trailing zeros and decimal point if not needed
                        new_value.erase
                        (
                            new_value.find_last_not_of('0') + 1, std::string::npos
                        );
                        new_value.erase
                        (
                            new_value.find_last_not_of('.') + 1, std::string::npos
                        );
                        environment[var_name] = 
                        t_TypedValue(new_value, e_ValueType::NUMBER);
                    }
                    else if (postfix->op.type == e_TokenType::MINUS_MINUS)
                    {
                        num_value -= 1.0;
                        std::string new_value = std::to_string(num_value);
                        // Remove trailing zeros and decimal point if not needed
                        new_value.erase
                        (
                            new_value.find_last_not_of('0') + 1, std::string::npos
                        );
                        new_value.erase
                        (
                            new_value.find_last_not_of('.') + 1, std::string::npos
                        );
                        environment[var_name] = 
                        t_TypedValue(new_value, e_ValueType::NUMBER);
                    }
                }
                catch (...)
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Cannot perform increment/decrement on non-numeric value"
                        )
                    );
                }
            }
            
            return t_Expected<std::string, t_ErrorInfo>(return_value);
        }
        return t_Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Postfix increment/decrement can only be applied to variables"
            )
        );
    }

    if (t_BinaryExpr *binary = dynamic_cast<t_BinaryExpr *>(expr))
    {
        // Handle assignment expressions
        if 
        (
            binary->op.type == e_TokenType::EQUAL       || 
            binary->op.type == e_TokenType::PLUS_EQUAL  ||
            binary->op.type == e_TokenType::MINUS_EQUAL ||
            binary->op.type == e_TokenType::STAR_EQUAL  ||
            binary->op.type == e_TokenType::SLASH_EQUAL ||
            binary->op.type == e_TokenType::MODULUS_EQUAL
        )
        {
            // Left side must be a variable
            if 
            (
                t_VariableExpr *var_expr = 
                dynamic_cast<t_VariableExpr *>(binary->left.get())
            )
            {
                std::string var_name = var_expr->name;
                
                // Check if variable was properly declared with 'auto' keyword
                // All variables must be declared before use
                if (environment.find(var_name) == environment.end())
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Variable '" + var_name + "' must be declared with 'auto' keyword before use"
                        )
                    );
                }
                
                // For compound assignments, we need to get the current value first
                t_Expected<std::string, t_ErrorInfo> left_result = 
                Evaluate(binary->left.get());

                if (!left_result.HasValue())
                {
                    return left_result;
                }
                std::string left_value = left_result.Value();
                
                t_Expected<std::string, t_ErrorInfo> right_result = 
                Evaluate(binary->right.get());

                if (!right_result.HasValue())
                {
                    return right_result;
                }
                std::string right_value = right_result.Value();
                
                t_Expected<std::string, t_ErrorInfo> final_value_result(right_value);
                
                // Handle compound assignments by converting them to regular operations
                switch (binary->op.type)
                {
                case e_TokenType::EQUAL:
                    final_value_result = 
                    t_Expected<std::string, t_ErrorInfo>(right_value);
                    break;
                    
                case e_TokenType::PLUS_EQUAL:
                    {
                        try 
                        {
                            double left_val = std::stod(left_value);
                            double right_val = std::stod(right_value);
                            final_value_result = 
                            t_Expected<std::string, t_ErrorInfo>
                            (
                                FormatNumber(left_val + right_val)
                            );
                        }
                        catch (...)
                        {
                            final_value_result = t_Expected<std::string, t_ErrorInfo>
                            (
                                t_ErrorInfo
                                (
                                    e_ErrorType::RUNTIME_ERROR, 
                                    "String concatenation with '+' is not allowed. Use comma-separated values in display statements instead."
                                )
                            );
                        }
                    }
                    break;
                    
                case e_TokenType::MINUS_EQUAL:
                    {
                        try 
                        {
                            double left_val = std::stod(left_value);
                            double right_val = std::stod(right_value);
                            final_value_result = t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val - right_val));
                        }
                        catch (...)
                        {
                            final_value_result = t_Expected<std::string, t_ErrorInfo>
                            (
                                t_ErrorInfo
                                (
                                    e_ErrorType::RUNTIME_ERROR, 
                                    "Cannot perform arithmetic operation"
                                )
                            );
                        }
                    }
                    break;
                    
                case e_TokenType::STAR_EQUAL:
                    {
                        try 
                        {
                            double left_val = std::stod(left_value);
                            double right_val = std::stod(right_value);
                            final_value_result = t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val * right_val));
                        }
                        catch (...)
                        {
                            final_value_result = t_Expected<std::string, t_ErrorInfo>
                            (
                                t_ErrorInfo
                                (
                                    e_ErrorType::RUNTIME_ERROR, 
                                    "Cannot perform arithmetic operation"
                                )
                            );
                        }
                    }
                    break;
                    
                case e_TokenType::MODULUS_EQUAL:
                    {
                        try 
                        {
                            double left_val = std::stod(left_value);
                            double right_val = std::stod(right_value);
                            
                            if (right_val == 0)
                            {
                                final_value_result = t_Expected<std::string, t_ErrorInfo>
                                (
                                    t_ErrorInfo
                                    (
                                        e_ErrorType::RUNTIME_ERROR, 
                                        "Modulus by zero"
                                    )
                                );
                            }
                            else
                            {
                                final_value_result = t_Expected<std::string, t_ErrorInfo>(FormatNumber(std::fmod(left_val, right_val)));
                            }
                        }
                        catch (...)
                        {
                            final_value_result = t_Expected<std::string, t_ErrorInfo>
                            (
                                t_ErrorInfo
                                (
                                    e_ErrorType::RUNTIME_ERROR, 
                                    "Cannot perform arithmetic operation"
                                )
                            );
                        }
                    }
                    break;
                    
                case e_TokenType::SLASH_EQUAL:
                    {
                        try 
                        {
                            double left_val = std::stod(left_value);
                            double right_val = std::stod(right_value);
                            
                            if (right_val == 0)
                            {
                                final_value_result = t_Expected<std::string, t_ErrorInfo>
                                (
                                    t_ErrorInfo
                                    (
                                        e_ErrorType::RUNTIME_ERROR, 
                                        "Division by zero"
                                    )
                                );
                            }
                            else
                            {
                                final_value_result = t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val / right_val));
                            }
                        }
                        catch (...)
                        {
                            final_value_result = t_Expected<std::string, t_ErrorInfo>
                            (
                                t_ErrorInfo
                                (
                                    e_ErrorType::RUNTIME_ERROR, 
                                    "Cannot perform arithmetic operation"
                                )
                            );
                        }
                    }
                    break;
                    
                default:
                    final_value_result = t_Expected<std::string, t_ErrorInfo>(right_value);
                    break;
                }
                
                if (!final_value_result.HasValue())
                {
                    return final_value_result;
                }
                
                std::string final_value = final_value_result.Value();
                e_ValueType type = DetectType(final_value);
                
                // Enforce static typing - check if the new value type matches the declared variable type
                auto it = environment.find(var_name);
                if (it != environment.end())
                {
                    e_ValueType declared_type = it->second.type;
                    // Allow assignment only if types match or if assigning to a NIL typed variable (initial assignment)
                    if 
                    (
                        declared_type != e_ValueType::NIL && 
                        declared_type != type
                    )
                    {
                        std::string type_name;
                        switch (declared_type)
                        {
                        case e_ValueType::NUMBER:
                            type_name = "number";
                            break;
                        case e_ValueType::STRING:
                            type_name = "string";
                            break;
                        case e_ValueType::BOOLEAN:
                            type_name = "boolean";
                            break;
                        default:
                            type_name = "unknown";
                            break;
                        }
                        
                        std::string new_type_name;
                        switch (type)
                        {
                        case e_ValueType::NUMBER:
                            new_type_name = "number";
                            break;
                        case e_ValueType::STRING:
                            new_type_name = "string";
                            break;
                        case e_ValueType::BOOLEAN:
                            new_type_name = "boolean";
                            break;
                        default:
                            new_type_name = "unknown";
                            break;
                        }
                        
                        return t_Expected<std::string, t_ErrorInfo>
                        (
                            t_ErrorInfo
                            (
                                e_ErrorType::TYPE_ERROR, 
                                "Type mismatch: variable '" + 
                                var_name                    + 
                                "' is "                     + 
                                type_name                   + 
                                ", cannot assign "          + 
                                new_type_name
                            )
                        );
                    }
                }
                
                // Update the variable in the environment
                AssignToVisibleVariable
                (
                    var_name, 
                    t_TypedValue(final_value, type), 
                    environment, 
                    scope_stack
                );
                
                // Return the assigned value
                return t_Expected<std::string, t_ErrorInfo>(final_value);
            }
            else
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Left side of assignment must be a variable"
                    )
                );
            }
        }
        
        // Evaluate operands
        t_Expected<std::string, t_ErrorInfo> left_result = 
        Evaluate(binary->left.get());
        if (!left_result.HasValue())
        {
            return left_result;
        }
        std::string left_str = left_result.Value();

        if (binary->op.type == e_TokenType::AND)
        {
            if (!IsTruthy(left_str))
            {
                return t_Expected<std::string, t_ErrorInfo>("false");
            }

            t_Expected<std::string, t_ErrorInfo> right_result =
            Evaluate(binary->right.get());
            if (!right_result.HasValue())
            {
                return right_result;
            }

            return t_Expected<std::string, t_ErrorInfo>
            (
                IsTruthy(right_result.Value()) ? "true" : "false"
            );
        }

        if (binary->op.type == e_TokenType::OR)
        {
            if (IsTruthy(left_str))
            {
                return t_Expected<std::string, t_ErrorInfo>("true");
            }

            t_Expected<std::string, t_ErrorInfo> right_result =
            Evaluate(binary->right.get());
            if (!right_result.HasValue())
            {
                return right_result;
            }

            return t_Expected<std::string, t_ErrorInfo>
            (
                IsTruthy(right_result.Value()) ? "true" : "false"
            );
        }
        
        t_Expected<std::string, t_ErrorInfo> right_result = 
        Evaluate(binary->right.get());
        if (!right_result.HasValue())
        {
            return right_result;
        }
        std::string right_str = right_result.Value();
        
        // Get typed values for optimized operations
        t_TypedValue left_typed(left_str, DetectType(left_str));
        t_TypedValue right_typed(right_str, DetectType(right_str));

        switch (binary->op.type)
        {
        case e_TokenType::PLUS:
            {
                // Simple addition implementation
                try 
                {
                    double left_val = std::stod(left_str);
                    double right_val = std::stod(right_str);
                    return t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val + right_val));
                }
                catch (...)
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "String concatenation with '+' is not allowed. Use comma-separated values in display statements instead."
                        )
                    );
                }
            }

        case e_TokenType::MINUS:
            {
                // Simple subtraction implementation
                try 
                {
                    double left_val = std::stod(left_str);
                    double right_val = std::stod(right_str);
                    return t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val - right_val));
                }
                catch (...)
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Cannot perform arithmetic operation"
                        )
                    );
                }
            }

        case e_TokenType::STAR:
            {
                // Simple multiplication implementation
                try 
                {
                    double left_val = std::stod(left_str);
                    double right_val = std::stod(right_str);
                    return t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val * right_val));
                }
                catch (...)
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Cannot perform arithmetic operation"
                        )
                    );
                }
            }
            
        case e_TokenType::MODULUS:
            {
                // Simple modulus implementation
                try 
                {
                    double left_val = std::stod(left_str);
                    double right_val = std::stod(right_str);
                    
                    if (right_val == 0)
                    {
                        return t_Expected<std::string, t_ErrorInfo>
                        (
                            t_ErrorInfo
                            (
                                e_ErrorType::RUNTIME_ERROR, 
                                "Modulus by zero"
                            )
                        );
                    }
                    
                    return t_Expected<std::string, t_ErrorInfo>(FormatNumber(std::fmod(left_val, right_val)));
                }
                catch (...)
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Cannot perform arithmetic operation"
                        )
                    );
                }
            }
            
        case e_TokenType::SLASH:
            {
                // Simple division implementation
                try 
                {
                    double left_val = std::stod(left_str);
                    double right_val = std::stod(right_str);
                    
                    if (right_val == 0)
                    {
                        return t_Expected<std::string, t_ErrorInfo>
                        (
                            t_ErrorInfo
                            (
                                e_ErrorType::RUNTIME_ERROR, 
                                "Division by zero"
                            )
                        );
                    }
                    
                    return t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val / right_val));
                }
                catch (...)
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Cannot perform arithmetic operation"
                        )
                    );
                }
            }
            
        case e_TokenType::BANG_EQUAL:
        case e_TokenType::EQUAL_EQUAL:
        case e_TokenType::GREATER:
        case e_TokenType::GREATER_EQUAL:
        case e_TokenType::LESS:
        case e_TokenType::LESS_EQUAL:
            {
                t_Expected<bool, t_ErrorInfo> comparison_result = PerformComparison(left_typed, binary->op.type, right_typed);
                if (!comparison_result.HasValue())
                {
                    return t_Expected<std::string, t_ErrorInfo>(comparison_result.Error());
                }
                return t_Expected<std::string, t_ErrorInfo>
                (
                    comparison_result.Value() ? "true" : "false"
                );
            }
        
        default:
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Unsupported binary operator"
                    )
                );
            }
        }
    }

    if (t_VariableExpr *variable = dynamic_cast<t_VariableExpr *>(expr))
    {
        auto it = environment.find(variable->name);
        if (it != environment.end())
        {
            return t_Expected<std::string, t_ErrorInfo>(it->second.value);
        }
        // Variables must be declared with 'auto' keyword before use
        return t_Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Variable '"   + 
                variable->name + 
                "' must be declared with 'auto' keyword before use"
            )
        );
    }

    return t_Expected<std::string, t_ErrorInfo>("");
}

std::string t_Interpreter::Stringify(const std::string &value)
{
    // In a real implementation, we would handle different types
    return value;
}

t_Expected<std::string, t_ErrorInfo> t_Interpreter::EvaluateFormatExpression
(
    const std::string &expr_str
)
{
    try
    {
        // Handle empty expressions
        if (expr_str.empty())
        {
            return t_Expected<std::string, t_ErrorInfo>("");
        }

        // For simple variable names, look them up directly in the environment
        auto it = environment.find(expr_str);
        if (it != environment.end())
        {
            return t_Expected<std::string, t_ErrorInfo>(it->second.value);
        }
        
        // For numeric literals, return them directly
        if (DetectType(expr_str) == e_ValueType::NUMBER)
        {
            return t_Expected<std::string, t_ErrorInfo>(expr_str);
        }
        
        // Handle simple arithmetic expressions
        // Look for operators in order of precedence (multiplication and division first)
        size_t mul_pos = expr_str.find('*');
        size_t div_pos = expr_str.find('/');
        size_t plus_pos = expr_str.find('+');
        size_t minus_pos = expr_str.find('-');
        
        // Handle multiplication
        if 
        (
            mul_pos != std::string::npos && 
            mul_pos > 0                  && 
            mul_pos < expr_str.length() - 1
        )
        {
            std::string left_str = expr_str.substr(0, mul_pos);
            std::string right_str = expr_str.substr(mul_pos + 1);
            
            // Trim whitespace
            left_str.erase(0, left_str.find_first_not_of(" \t"));
            left_str.erase(left_str.find_last_not_of(" \t") + 1);
            right_str.erase(0, right_str.find_first_not_of(" \t"));
            right_str.erase(right_str.find_last_not_of(" \t") + 1);
            
            // Evaluate left and right operands
            t_Expected<std::string, t_ErrorInfo> left_result = EvaluateFormatExpression(left_str);
            if (!left_result.HasValue())
            {
                return left_result;
            }
            t_Expected<std::string, t_ErrorInfo> right_result = EvaluateFormatExpression(right_str);
            if (!right_result.HasValue())
            {
                return right_result;
            }
            
            // Try to perform multiplication
            std::string left_value = left_result.Value();
            std::string right_value = right_result.Value();
            
            try 
            {
                double left_val = std::stod(left_value);
                double right_val = std::stod(right_value);
                return t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val * right_val));
            }
            catch (...)
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Cannot perform arithmetic operation"
                    )
                );
            }

        }
        
        // Handle division
        if 
        (
            div_pos != std::string::npos && 
            div_pos > 0                  && 
            div_pos < expr_str.length() - 1
        )
        {
            std::string left_str = expr_str.substr(0, div_pos);
            std::string right_str = expr_str.substr(div_pos + 1);
            
            // Trim whitespace
            left_str.erase(0, left_str.find_first_not_of(" \t"));
            left_str.erase(left_str.find_last_not_of(" \t") + 1);
            right_str.erase(0, right_str.find_first_not_of(" \t"));
            right_str.erase(right_str.find_last_not_of(" \t") + 1);
            
            // Evaluate left and right operands
            t_Expected<std::string, t_ErrorInfo> left_result = EvaluateFormatExpression(left_str);
            if (!left_result.HasValue())
            {
                return left_result;
            }
            t_Expected<std::string, t_ErrorInfo> right_result = EvaluateFormatExpression(right_str);
            if (!right_result.HasValue())
            {
                return right_result;
            }
            
            // Try to perform division
            std::string left_value = left_result.Value();
            std::string right_value = right_result.Value();
            
            try 
            {
                double left_val = std::stod(left_value);
                double right_val = std::stod(right_value);
                
                if (right_val == 0)
                {
                    return t_Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Division by zero"
                        )
                    );
                }
                
                return t_Expected<std::string, t_ErrorInfo>
                (
                    FormatNumber(left_val / right_val)
                );
            }
            catch (...)
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Cannot perform arithmetic operation"
                    )
                );
            }

        }
        
        // Handle addition
        if 
        (
            plus_pos != std::string::npos && 
            plus_pos > 0                  && 
            plus_pos < expr_str.length() - 1
        )
        {
            std::string left_str = expr_str.substr(0, plus_pos);
            std::string right_str = expr_str.substr(plus_pos + 1);
            
            // Trim whitespace
            left_str.erase(0, left_str.find_first_not_of(" \t"));
            left_str.erase(left_str.find_last_not_of(" \t") + 1);
            right_str.erase(0, right_str.find_first_not_of(" \t"));
            right_str.erase(right_str.find_last_not_of(" \t") + 1);
            
            // Evaluate left and right operands
            t_Expected<std::string, t_ErrorInfo> left_result = EvaluateFormatExpression(left_str);
            if (!left_result.HasValue())
            {
                return left_result;
            }
            t_Expected<std::string, t_ErrorInfo> right_result = EvaluateFormatExpression(right_str);
            if (!right_result.HasValue())
            {
                return right_result;
            }
            
            // Try to perform addition
            std::string left_value = left_result.Value();
            std::string right_value = right_result.Value();
            
            try 
            {
                double left_val = std::stod(left_value);
                double right_val = std::stod(right_value);
                return t_Expected<std::string, t_ErrorInfo>
                (
                    FormatNumber(left_val + right_val)
                );
            }
            catch (...)
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "String concatenation with '+' is not allowed. Use comma-separated values in display statements instead."
                    )
                );
            }

        }
        
        // Handle subtraction
        if 
        (
            minus_pos != std::string::npos && 
            minus_pos > 0                  && 
            minus_pos < expr_str.length() - 1
        )
        {
            std::string left_str = expr_str.substr(0, minus_pos);
            std::string right_str = expr_str.substr(minus_pos + 1);
            
            // Trim whitespace
            left_str.erase(0, left_str.find_first_not_of(" \t"));
            left_str.erase(left_str.find_last_not_of(" \t") + 1);
            right_str.erase(0, right_str.find_first_not_of(" \t"));
            right_str.erase(right_str.find_last_not_of(" \t") + 1);
            
            // Evaluate left and right operands
            t_Expected<std::string, t_ErrorInfo> left_result = EvaluateFormatExpression(left_str);
            if (!left_result.HasValue())
            {
                return left_result;
            }
            t_Expected<std::string, t_ErrorInfo> right_result = EvaluateFormatExpression(right_str);
            if (!right_result.HasValue())
            {
                return right_result;
            }
            
            // Try to perform subtraction
            std::string left_value = left_result.Value();
            std::string right_value = right_result.Value();
            
            try 
            {
                double left_val = std::stod(left_value);
                double right_val = std::stod(right_value);
                return t_Expected<std::string, t_ErrorInfo>(FormatNumber(left_val - right_val));
            }
            catch (...)
            {
                return t_Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Cannot perform arithmetic operation"
                    )
                );
            }

        }
        
        // For other complex expressions, return the expression as a string
        return t_Expected<std::string, t_ErrorInfo>(expr_str);
    }
    catch (const std::exception& e)
    {
        // If parsing fails, treat as literal text
        return t_Expected<std::string, t_ErrorInfo>(expr_str);
    }
    catch (...)
    {
        // If parsing fails, treat as literal text
        return t_Expected<std::string, t_ErrorInfo>(expr_str);
    }
}

// Helper function to check if a for loop is a simple numeric loop pattern: for (auto i = 0; i < N; i++)
bool t_Interpreter::IsSimpleNumericLoop(t_ForStmt* for_stmt)
{
    // Check if initializer is: auto var = 0
    if (!for_stmt->initializer) return false;
    
    t_VarStmt* init_var = dynamic_cast<t_VarStmt*>(for_stmt->initializer.get());
    if (!init_var || !init_var->initializer) return false;
    
    t_LiteralExpr* init_literal = 
    dynamic_cast<t_LiteralExpr*>(init_var->initializer.get());
    if (!init_literal || init_literal->token_type != e_TokenType::NUMBER) return false;

    int start_value = 0;
    try
    {
        double raw_value = std::stod(init_literal->value);
        start_value = static_cast<int>(raw_value);
        if (raw_value != static_cast<double>(start_value))
        {
            return false;
        }
    }
    catch (...)
    {
        return false;
    }
    
    // Check if condition is: var < number
    if (!for_stmt->condition) return false;
    
    t_BinaryExpr* condition_binary = 
    dynamic_cast<t_BinaryExpr*>(for_stmt->condition.get());
    if (!condition_binary) return false;

    e_TokenType condition_op = condition_binary->op.type;
    if
    (
        condition_op != e_TokenType::LESS          &&
        condition_op != e_TokenType::LESS_EQUAL    &&
        condition_op != e_TokenType::GREATER       &&
        condition_op != e_TokenType::GREATER_EQUAL
    )
    {
        return false;
    }
    
    t_VariableExpr* condition_var = 
    dynamic_cast<t_VariableExpr*>(condition_binary->left.get());
    if (!condition_var || condition_var->name != init_var->name) return false;
    
    t_LiteralExpr* condition_literal = 
    dynamic_cast<t_LiteralExpr*>(condition_binary->right.get());
    if (!condition_literal || condition_literal->token_type != e_TokenType::NUMBER) return false;

    int limit_value = 0;
    try
    {
        double raw_value = std::stod(condition_literal->value);
        limit_value = static_cast<int>(raw_value);
        if (raw_value != static_cast<double>(limit_value))
        {
            return false;
        }
    }
    catch (...)
    {
        return false;
    }
    
    // Try to convert condition literal to number
    try 
    {
        (void)std::stod(condition_literal->value);
    } 
    catch (...) 
    {
        return false;
    }
    
    // Check if increment is: var++ or ++var
    if (!for_stmt->increment) return false;

    int step = 0;
    
    // Check for postfix increment: var++
    t_PostfixExpr* postfix_inc = 
    dynamic_cast<t_PostfixExpr*>(for_stmt->increment.get());
    if (postfix_inc) 
    {
        t_VariableExpr* inc_var = 
        dynamic_cast<t_VariableExpr*>(postfix_inc->operand.get());
        if 
        (
            inc_var                         && 
            inc_var->name == init_var->name && 
            postfix_inc->op.type == e_TokenType::PLUS_PLUS
        ) 
        {
            step = 1;
        }
        else if
        (
            inc_var                         &&
            inc_var->name == init_var->name &&
            postfix_inc->op.type == e_TokenType::MINUS_MINUS
        )
        {
            step = -1;
        }
        else
        {
            return false;
        }
    }
    
    // Check for prefix increment: ++var
    t_PrefixExpr* prefix_inc = 
    dynamic_cast<t_PrefixExpr*>(for_stmt->increment.get());

    if (prefix_inc) 
    {
        t_VariableExpr* inc_var = 
        dynamic_cast<t_VariableExpr*>(prefix_inc->operand.get());
        if 
        (
            inc_var                         && 
            inc_var->name == init_var->name && 
            prefix_inc->op.type == e_TokenType::PLUS_PLUS
        )
        {
            step = 1;
        }
        else if
        (
            inc_var                         &&
            inc_var->name == init_var->name &&
            prefix_inc->op.type == e_TokenType::MINUS_MINUS
        )
        {
            step = -1;
        }
        else
        {
            return false;
        }
    }

    if (step == 0)
    {
        if (t_BinaryExpr* assign = dynamic_cast<t_BinaryExpr*>(for_stmt->increment.get()))
        {
            if
            (
                assign->op.type != e_TokenType::PLUS_EQUAL &&
                assign->op.type != e_TokenType::MINUS_EQUAL
            )
            {
                return false;
            }

            t_VariableExpr* lhs = dynamic_cast<t_VariableExpr*>(assign->left.get());
            t_LiteralExpr* rhs = dynamic_cast<t_LiteralExpr*>(assign->right.get());
            if
            (
                !lhs ||
                !rhs ||
                lhs->name != init_var->name ||
                rhs->token_type != e_TokenType::NUMBER
            )
            {
                return false;
            }

            int rhs_int = 0;
            try
            {
                double raw_value = std::stod(rhs->value);
                rhs_int = static_cast<int>(raw_value);
                if (raw_value != static_cast<double>(rhs_int))
                {
                    return false;
                }
            }
            catch (...)
            {
                return false;
            }

            if (rhs_int == 0)
            {
                return false;
            }

            step = (assign->op.type == e_TokenType::PLUS_EQUAL) ? rhs_int : -rhs_int;
        }
        else
        {
            return false;
        }
    }

    if (condition_op == e_TokenType::LESS || condition_op == e_TokenType::LESS_EQUAL)
    {
        if (step <= 0)
        {
            return false;
        }
    }
    else
    {
        if (step >= 0)
        {
            return false;
        }
    }

    (void)start_value;
    (void)limit_value;
    return true;
}

// Ultra-fast optimization for simple accumulation loops: for (auto i = 0; i < N; i++) { var += i; }
bool t_Interpreter::IsSimpleAccumulationLoop(t_ForStmt* for_stmt)
{
    // First check if it's a simple numeric loop
    if (!IsSimpleNumericLoop(for_stmt)) return false;

    t_VarStmt* init_var = dynamic_cast<t_VarStmt*>
    (
        for_stmt->initializer.get()
    );
    t_LiteralExpr* init_literal =
    dynamic_cast<t_LiteralExpr*>(init_var->initializer.get());

    t_BinaryExpr* condition_binary =
    dynamic_cast<t_BinaryExpr*>(for_stmt->condition.get());

    if
    (
        !init_literal ||
        init_literal->value != "0" ||
        !condition_binary ||
        condition_binary->op.type != e_TokenType::LESS
    )
    {
        return false;
    }

    bool increment_is_plus_plus = false;
    if 
    (
        t_PostfixExpr* postfix_inc = 
        dynamic_cast<t_PostfixExpr*>(for_stmt->increment.get())
    )
    {
        increment_is_plus_plus = 
        (
            postfix_inc->op.type == e_TokenType::PLUS_PLUS
        );
    }
    else if 
    (
        t_PrefixExpr* prefix_inc = 
        dynamic_cast<t_PrefixExpr*>(for_stmt->increment.get())
    )
    {
        increment_is_plus_plus = 
        (
            prefix_inc->op.type == e_TokenType::PLUS_PLUS
        );
    }

    if (!increment_is_plus_plus)
    {
        return false;
    }
    
    // Check if body is a block with exactly one statement
    t_BlockStmt* body_block = dynamic_cast<t_BlockStmt*>(for_stmt->body.get());
    if (!body_block || body_block->statements.size() != 1) return false;
    
    t_ExpressionStmt* expr_stmt = 
    dynamic_cast<t_ExpressionStmt*>(body_block->statements[0].get());
    if (!expr_stmt) return false;
    
    // Check if the expression is a binary assignment (+=)
    t_BinaryExpr* binary_expr = 
    dynamic_cast<t_BinaryExpr*>(expr_stmt->expression.get());
    if 
    (
        !binary_expr || 
        binary_expr->op.type != e_TokenType::PLUS_EQUAL
    ) return false;
    
    // Check if left side is a variable
    t_VariableExpr* left_var = 
    dynamic_cast<t_VariableExpr*>(binary_expr->left.get());
    if (!left_var) return false;
    
    // Check if right side is the loop variable
    t_VariableExpr* right_var = 
    dynamic_cast<t_VariableExpr*>(binary_expr->right.get());
    if (!right_var) return false;
    
    // Check if right side matches loop variable
    if (right_var->name != init_var->name) return false;
    
    return true;
}

// Ultra-fast native execution for simple accumulation loops
t_Expected<int, t_ErrorInfo> t_Interpreter::ExecuteAccumulationLoop
(
    t_ForStmt* for_stmt
)
{
    // Extract loop parameters
    t_VarStmt* init_var = dynamic_cast<t_VarStmt*>
    (
        for_stmt->initializer.get()
    );
    t_BinaryExpr* condition_binary = 
    dynamic_cast<t_BinaryExpr*>(for_stmt->condition.get());
    
    t_LiteralExpr* condition_literal = 
    dynamic_cast<t_LiteralExpr*>(condition_binary->right.get());
    
    // Get the loop limit
    int limit = static_cast<int>(std::stod(condition_literal->value));
    
    // Get accumulation variable name
    t_BlockStmt* body_block = dynamic_cast<t_BlockStmt*>(for_stmt->body.get());
    t_ExpressionStmt* expr_stmt = 
    dynamic_cast<t_ExpressionStmt*>(body_block->statements[0].get());

    t_BinaryExpr* binary_expr = 
    dynamic_cast<t_BinaryExpr*>(expr_stmt->expression.get());
    
    t_VariableExpr* acc_var = 
    dynamic_cast<t_VariableExpr*>(binary_expr->left.get());
    
    std::string acc_var_name = acc_var->name;
    std::string loop_var_name = init_var->name;
    
    // Check if accumulation variable exists and get its initial value
    auto acc_it = environment.find(acc_var_name);
    if (acc_it == environment.end())
    {
        return t_Expected<int, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Variable '" + acc_var_name + "' must be declared with 'auto' keyword before use"
            )
        );
    }
    
    // Get initial value (should be 0 based on test.rd)
    double accumulator = 0.0;
    if (acc_it->second.has_numeric_value)
    {
        accumulator = acc_it->second.numeric_value;
    }
    else
    {
        try
        {
            accumulator = std::stod(acc_it->second.value);
        }
        catch (...)
        {
            return t_Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Variable '" + acc_var_name + "' must be numeric for accumulation"
                )
            );
        }
    }
    
    // NATIVE C++ LOOP: Direct arithmetic without interpretation overhead
    for (int i = 0; i < limit; i++)
    {
        accumulator += i;
    }
    
    // Update the accumulation variable with the result
    environment[acc_var_name] = t_TypedValue(accumulator);

    return t_Expected<int, t_ErrorInfo>(0);
}

t_Expected<int, t_ErrorInfo> t_Interpreter::ExecuteSimpleNumericLoop
(
    t_ForStmt* for_stmt
)
{
    // Store variables that existed before the loop
    std::unordered_set<std::string> pre_loop_variables;
    for (const auto& pair : environment) 
    {
        pre_loop_variables.insert(pair.first);
    }
    
    // Push a new scope for the loop
    PushScope();
    loop_depth++;
    
    // Extract loop parameters
    t_VarStmt* init_var = dynamic_cast<t_VarStmt*>
    (
        for_stmt->initializer.get()
    );
    t_LiteralExpr* init_literal =
    dynamic_cast<t_LiteralExpr*>(init_var->initializer.get());

    t_BinaryExpr* condition_binary = 
    dynamic_cast<t_BinaryExpr*>(for_stmt->condition.get());

    t_LiteralExpr* condition_literal = 
    dynamic_cast<t_LiteralExpr*>(condition_binary->right.get());

    int start = static_cast<int>(std::stod(init_literal->value));
    int limit = static_cast<int>(std::stod(condition_literal->value));
    e_TokenType condition_op = condition_binary->op.type;

    int step = 1;
    if (t_PostfixExpr* postfix_inc = dynamic_cast<t_PostfixExpr*>(for_stmt->increment.get()))
    {
        step = (postfix_inc->op.type == e_TokenType::PLUS_PLUS) ? 1 : -1;
    }
    else if (t_PrefixExpr* prefix_inc = dynamic_cast<t_PrefixExpr*>(for_stmt->increment.get()))
    {
        step = (prefix_inc->op.type == e_TokenType::PLUS_PLUS) ? 1 : -1;
    }
    else if (t_BinaryExpr* assign = dynamic_cast<t_BinaryExpr*>(for_stmt->increment.get()))
    {
        t_LiteralExpr* rhs = dynamic_cast<t_LiteralExpr*>(assign->right.get());
        int rhs_int = static_cast<int>(std::stod(rhs->value));
        step = 
        (
            assign->op.type == e_TokenType::PLUS_EQUAL) ? 
                rhs_int : -rhs_int;
    }

    std::string loop_var_name = init_var->name;

    bool use_fast_break_optimization = false;
    int break_at_value = -1;

    bool allow_break_optimization =
    (
        start == 0                  &&
        step == 1                   &&
        condition_op == e_TokenType::LESS
    );

    t_BlockStmt* body_block = dynamic_cast<t_BlockStmt*>(for_stmt->body.get());
    if 
    (
        allow_break_optimization && 
        body_block               &&
        body_block->statements.size() == 1
    )
    {
        t_IfStmt* if_stmt = dynamic_cast<t_IfStmt*>
        (
            body_block->statements[0].get()
        );
        if (if_stmt && !if_stmt->else_branch)
        {
            bool then_is_break = false;

            if (dynamic_cast<t_BreakStmt*>(if_stmt->then_branch.get()))
            {
                then_is_break = true;
            }
            else if 
            (
                t_BlockStmt* then_block = 
                dynamic_cast<t_BlockStmt*>(if_stmt->then_branch.get())
            )
            {
                if (then_block->statements.size() == 1)
                {
                    if 
                    (
                        dynamic_cast<t_BreakStmt*>
                        (
                            then_block->statements[0].get()
                        )
                    )
                    {
                        then_is_break = true;
                    }
                }
            }

            if (then_is_break)
            {
                t_BinaryExpr* cond_binary = 
                dynamic_cast<t_BinaryExpr*>(if_stmt->condition.get());

                if 
                (
                    cond_binary && 
                    cond_binary->op.type == e_TokenType::EQUAL_EQUAL
                )
                {
                    t_VariableExpr* cond_var = 
                    dynamic_cast<t_VariableExpr*>(cond_binary->left.get());
                    t_LiteralExpr* cond_lit = 
                    dynamic_cast<t_LiteralExpr*>(cond_binary->right.get());

                    if (!cond_var || !cond_lit)
                    {
                        cond_var = dynamic_cast<t_VariableExpr*>(cond_binary->right.get());

                        cond_lit = dynamic_cast<t_LiteralExpr*>(cond_binary->left.get());
                    }

                    if 
                    (
                        cond_var && 
                        cond_lit && 
                        cond_var->name == loop_var_name
                    )
                    {
                        try
                        {
                            double raw_value = std::stod(cond_lit->value);
                            int int_value = static_cast<int>(raw_value);
                            if (raw_value == static_cast<double>(int_value))
                            {
                                break_at_value = int_value;
                                use_fast_break_optimization = true;
                            }
                        }
                        catch (...) {}
                    }
                }
            }
        }
    }

    // Execute the loop with direct numeric operations
    // Manually control the loop to properly handle continue statements
    if (!use_fast_break_optimization)
    {
        auto ConditionHolds =
        [condition_op, limit](int i) -> bool
        {
            switch (condition_op)
            {
            case e_TokenType::LESS:
                return i < limit;

            case e_TokenType::LESS_EQUAL:
                return i <= limit;

            case e_TokenType::GREATER:
                return i > limit;

            case e_TokenType::GREATER_EQUAL:
                return i >= limit;

            default:
                return false;
            }
        };

        for (int i = start; ConditionHolds(i);)
        {
            // Set loop variable directly as integer
            environment[loop_var_name] = t_TypedValue(static_cast<double>(i));
            
            // Execute body
            control_signal.clear();
            t_Expected<int, t_ErrorInfo> body_result = 
            Execute(for_stmt->body.get());

            if (!body_result.HasValue())
            {
                PopScope(); // Clean up scope before returning
                loop_depth--;
                return body_result;
            }
            if (control_signal == "break")
            {
                control_signal.clear();
                break;
            }
            if (control_signal == "continue")
            {
                control_signal.clear();
                // Skip increment and go to next iteration when continue is encountered
                i += step;
                continue;
            }
            
            // If we're returning from a function, stop the loop and propagate the return
            if (is_returning)
            {
                PopScope();
                loop_depth--;
                return t_Expected<int, t_ErrorInfo>(0);
            }
            
            // Only increment if no control signal was encountered
            i += step;
        }
    }
    else
    {
        int final_i = 0;
        if (break_at_value >= 0 && break_at_value < limit)
        {
            final_i = break_at_value;
        }
        else if (limit > 0)
        {
            final_i = limit - 1;
        }

        control_signal.clear();
        environment[loop_var_name] = t_TypedValue
        (
            static_cast<double>(final_i)
        );
    }

    // Before popping scope, preserve modifications to pre-existing variables
    std::unordered_map<std::string, t_TypedValue> modified_pre_existing;
    for (const auto& pair : environment)
    {
        if (pre_loop_variables.count(pair.first) > 0)
        {
            modified_pre_existing[pair.first] = pair.second;
        }
    }

    // Pop the loop scope
    PopScope();
    
    // Restore the modified values of pre-existing variables
    for (const auto& pair : modified_pre_existing)
    {
        environment[pair.first] = pair.second;
    }
    loop_depth--;
    
    return t_Expected<int, t_ErrorInfo>(0); 
}
#include <rubberduck/Interpreter.h>
#include <iostream>
#include <sstream>
#include <regex>
#include <stack>
#include <cctype>
#include <iomanip>
#include <unordered_set>
#include <limits>
#include <cmath>
#include <rubberduck/Lexer.h>
#include <rubberduck/Parser.h>
#include <rubberduck/ErrorHandling.h>

// Helper: Assigns variable to all visible scopes where it exists.
static void AssignToVisibleVariable
(
    const std::string &name, 
    const t_TypedValue &value, 
    std::unordered_map<std::string, 
    t_TypedValue> &m_Environment, 
    std::vector<std::unordered_map<std::string, 
    t_TypedValue>> &m_ScopeStack
)
{
    auto& env_entry = m_Environment[name];
    env_entry = value;
    
    const auto& key_ref = name;
    for(auto& scope : m_ScopeStack) 
    {
        auto it = scope.find(key_ref);
        if(it != scope.end())
        {
            it->second = value;
        }
    }
}

Interpreter::Interpreter()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    PushScope();
    m_BufferOutput = false;
    m_OutputBuffer.clear();

    m_Functions.reserve(64);
    m_Environment.reserve(256);
    m_ScopeStack.reserve(32);
}

void Interpreter::WriteOutput(const std::string& text)
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

void Interpreter::FlushOutput()
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

InterpretationResult Interpreter::Interpret
(
    const std::vector<PoolPtr<t_Stmt>> &statements
)
{
    m_Functions.clear();

    for (const auto &statement : statements)
    {
        if (t_FunStmt *fun_stmt = As<t_FunStmt>(statement.get()))
        {
            m_Functions[fun_stmt->name] = fun_stmt;
        }
    }

    for (const auto &statement : statements)
    {
        try
        {
            Expected<int, t_ErrorInfo> result = Execute(statement.get());
            if (!result)
            {
                // Report the error and stop execution
                ReportError(result.Error());
                return InterpretationResult(result.Error());
            }
        }
        catch (const std::exception& ex)
        {
            t_ErrorInfo err
            (
                e_ErrorType::RUNTIME_ERROR,
                std::string("Unhandled std::exception: ") + ex.what()
            );
            ReportError(err);
            return InterpretationResult(err);
        }
        catch (...)
        {
            t_ErrorInfo err
            (
                e_ErrorType::RUNTIME_ERROR,
                "Unhandled unknown exception during execution"
            );
            ReportError(err);
            return InterpretationResult(err);
        }
    }
    
    return InterpretationResult(0); // Success represented by 0
}

// Scope management m_Functions
void Interpreter::PushScope()
{
    // Save the current state of the m_Environment
    m_ScopeStack.emplace_back(m_Environment);
}

void Interpreter::PopScope()
{
    if (!m_ScopeStack.empty()) 
    {
        // Restore the m_Environment to its previous state
        m_Environment = std::move(m_ScopeStack.back());
        m_ScopeStack.pop_back();
    }
}

Expected<int, t_ErrorInfo> Interpreter::DeclareVariable
(
    const std::string& name, int line
)
{
    // Check if variable is already declared in current scope
    // We do this by checking if it exists in the current m_Environment
    // and comparing with the previous scope's m_Environment if it exists
    if (!m_Environment.count(name)) 
    {
        // Variable not declared yet, this is fine
        return Expected<int, t_ErrorInfo>(0); // Success
    }
    
    // Variable already exists, check if it was declared in current scope
    if (!m_ScopeStack.empty()) 
    {
        const auto& previous_scope = m_ScopeStack.back();
        if (previous_scope.find(name) == previous_scope.end()) 
        {
            // Variable exists but was declared in current scope - this is an error
            return Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Variable '" 
                    + name + 
                    "' has already been declared in this scope", 
                    line
                )
            );
        }
    }
    
    // Variable exists but was declared in a previous scope - this is shadowing, which is allowed
    return Expected<int, t_ErrorInfo>(0); // Success
}

// Helper function to format numbers (removes trailing zeros)
std::string Interpreter::FormatNumber(double value)
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
e_ValueType Interpreter::DetectType(const std::string& value)
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
bool Interpreter::IsInteger(const std::string& value)
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
bool Interpreter::IsFloat(const std::string& value)
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
    
    return has_decimal && 
    (
        static_cast<size_t>(value.length()) > static_cast<size_t>(start + 1)
    );
}

// Comparison operations are kept as they are more complex
Expected<bool, t_ErrorInfo> Interpreter::PerformComparison
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
            return Expected<bool, t_ErrorInfo>
            (
                left.numeric_value > right.numeric_value
            );
        
        case e_TokenType::GREATER_EQUAL:
            return Expected<bool, t_ErrorInfo>
            (
                left.numeric_value >= right.numeric_value
            );
        
        case e_TokenType::LESS:
            return Expected<bool, t_ErrorInfo>
            (
                left.numeric_value < right.numeric_value
            );
        
        case e_TokenType::LESS_EQUAL:
            return Expected<bool, t_ErrorInfo>
            (
                left.numeric_value <= right.numeric_value
            );
        
        case e_TokenType::EQUAL_EQUAL:
            return Expected<bool, t_ErrorInfo>
            (
                left.numeric_value == right.numeric_value
            );
        
        case e_TokenType::BANG_EQUAL:
            return Expected<bool, t_ErrorInfo>
            (
                left.numeric_value != right.numeric_value
            );
        
        default:
            return Expected<bool, t_ErrorInfo>
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
            return Expected<bool, t_ErrorInfo>(left_num > right_num);
        case e_TokenType::GREATER_EQUAL:
            return Expected<bool, t_ErrorInfo>(left_num >= right_num);
        case e_TokenType::LESS:
            return Expected<bool, t_ErrorInfo>(left_num < right_num);
        case e_TokenType::LESS_EQUAL:
            return Expected<bool, t_ErrorInfo>(left_num <= right_num);
        case e_TokenType::EQUAL_EQUAL:
            return Expected<bool, t_ErrorInfo>(left_num == right_num);
        case e_TokenType::BANG_EQUAL:
            return Expected<bool, t_ErrorInfo>(left_num != right_num);
        default:
            return Expected<bool, t_ErrorInfo>(false);
        }
    }
    catch (...)
    {
        // For non-numeric comparisons, fallback to string comparison
        switch (op)
        {
        case e_TokenType::EQUAL_EQUAL:
            return Expected<bool, t_ErrorInfo>(left.value == right.value);
        case e_TokenType::BANG_EQUAL:
            return Expected<bool, t_ErrorInfo>(left.value != right.value);
        default:
            return Expected<bool, t_ErrorInfo>
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

Expected<int, t_ErrorInfo> Interpreter::Execute(t_Stmt *stmt)
{
    if (t_BlockStmt* block_stmt = As<t_BlockStmt>(stmt))
    {
        // Push a new scope for the block
        PushScope();
		try
		{
			for (const auto &statement : block_stmt->statements)
			{
				Expected<int, t_ErrorInfo> result = 
                Execute(statement.get());
				if (!result)
				{
					PopScope(); // Clean up scope before returning
					return result.Error();
				}

                // If a control signal was raised inside this block (break/continue),
                // stop executing further statements in this block and propagate upward.
                if (!m_ControlSignal.empty())
                {
                    // Pop the scope to clean up variables declared in this block
                    PopScope();
                    return Expected<int, t_ErrorInfo>(0);
                }
                
                // If we're returning from a function, stop executing further statements
                // and propagate the return signal upward.
                if (m_IsReturning)
                {
                    PopScope();
                    return Expected<int, t_ErrorInfo>(0);
                }
			}

			// Pop the block scope, which will automatically clean up variables declared in this scope
			PopScope();
		}
		catch (...)
		{
			// Ensure scope is popped and convert to runtime error
			PopScope();
			return Expected<int, t_ErrorInfo>
			(
				t_ErrorInfo
				(
					e_ErrorType::RUNTIME_ERROR,
					"Unhandled exception in block"
				)
			);
		}
    }
    else if (As<t_BreakStmt>(stmt))
    {
        // Validate that we're inside a loop
        if (m_LoopDepth <= 0)
        {
            return Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "'break' used outside of a loop"
                )
            );
        }
        // Signal break without throwing
        m_ControlSignal = "break";
        return Expected<int, t_ErrorInfo>(0);
    }
    else if (As<t_ContinueStmt>(stmt))
    {
        // Validate that we're inside a loop
        if (m_LoopDepth <= 0)
        {
            return Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "'continue' used outside of a loop"
                )
            );
        }
        // Signal continue without throwing
        m_ControlSignal = "continue";
        return Expected<int, t_ErrorInfo>(0);
    }
    else if (t_IfStmt *if_stmt = As<t_IfStmt>(stmt))
    {
        Expected<std::string, t_ErrorInfo> condition_result = Evaluate
        (
            if_stmt->condition.get()
        );
        if (!condition_result)
        {
            return condition_result.Error();
        }
        
        if (IsTruthy(condition_result.Value()))
        {
            Expected<int, t_ErrorInfo> then_result = Execute
            (
                if_stmt->then_branch.get()
            );
            if (!then_result)
            {
                return then_result;
            }
            
            // If we're returning from a function, propagate the return
            if (m_IsReturning)
            {
                return Expected<int, t_ErrorInfo>(0);
            }
        }
        else if (if_stmt->else_branch)
        {
            Expected<int, t_ErrorInfo> else_result = 
            Execute(if_stmt->else_branch.get());

            if (!else_result)
            {
                return else_result;
            }
            
            // If we're returning from a function, propagate the return
            if (m_IsReturning)
            {
                return Expected<int, t_ErrorInfo>(0);
            }
        }
    }
    else if (t_ForStmt *for_stmt = As<t_ForStmt>(stmt))
    {
        // Check for ultra-fast nested arithmetic loop pattern: nested loops with arithmetic
        if (IsNestedArithmeticLoop(for_stmt))
        {
            Expected<int, t_ErrorInfo> result = 
            ExecuteNestedArithmeticLoop(for_stmt);

            if (!result)
            {
                return result;
            }
        }
        // Check for ultra-fast accumulation loop pattern: for (auto i = 0; i < N; i++) { var += i; }
        else if (IsSimpleAccumulationLoop(for_stmt))
        {
            Expected<int, t_ErrorInfo> result = 
            ExecuteAccumulationLoop(for_stmt);

            if (!result)
            {
                return result;
            }
        }
        // Check for specialized numeric loop pattern: for (auto i = 0; i < N; i++)
        else if (IsSimpleNumericLoop(for_stmt))
        {
            Expected<int, t_ErrorInfo> result = 
            ExecuteSimpleNumericLoop(for_stmt);

            if (!result)
            {
                return result;
            }
        }
        else
        {
            // Store variables that existed before the loop
            std::unordered_set<std::string_view> pre_loop_variables;
            pre_loop_variables.reserve(m_Environment.size());
            for (auto&& [key, _] : m_Environment) 
            {
                pre_loop_variables.insert(key);
            }

            // Push a new scope for the loop
            PushScope();
            m_LoopDepth++;
            try
            {
                // Execute initializer (if any)
                if (for_stmt->initializer)
                {
                    Expected<int, t_ErrorInfo> init_result = 
                    Execute(for_stmt->initializer.get());

                    if (!init_result)
                    {
                        PopScope(); // Clean up scope before returning
                        m_LoopDepth--;
                        return init_result;
                    }
                }

                // Loop while condition is true (or forever if no condition)
                while (true)
                {
                    // Check condition (if any)
                    if (for_stmt->condition)
                    {
                        Expected<std::string, t_ErrorInfo> condition_result = 
                        Evaluate(for_stmt->condition.get());
                        if (!condition_result)
                        {
                            PopScope(); // Clean up scope before returning
                            m_LoopDepth--;
                            return condition_result.Error();
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
                        m_ControlSignal.clear();
                        Expected<int, t_ErrorInfo> body_result = 
                        Execute(for_stmt->body.get());

                        if (!body_result)
                        {
                            PopScope(); // Clean up scope before returning
                            m_LoopDepth--;
                            return body_result;
                        }
                        if (m_ControlSignal == "break")
                        {
                            m_ControlSignal.clear();
                            break;
                        }
                        if (m_ControlSignal == "continue")
                        {
                            // Skip increment and start next iteration
                            m_ControlSignal.clear();
                            continue; // Continue to the next loop iteration without incrementing
                        }
                        
                        // If we're returning from a function, stop the loop and propagate the return
                        if (m_IsReturning)
                        {
                            PopScope();
                            m_LoopDepth--;
                            return Expected<int, t_ErrorInfo>(0);
                        }
                    }

                    // Execute increment (if any)
                    if (for_stmt->increment && m_ControlSignal.empty())
                    {
                        Expected<std::string, t_ErrorInfo> increment_result= 
                        Evaluate(for_stmt->increment.get());

                        if (!increment_result)
                        {
                            PopScope(); // Clean up scope before returning
                            m_LoopDepth--;
                            return increment_result.Error();
                        }
                    }
                }

                // Before popping scope, preserve modifications to pre-existing variables
                std::unordered_map<std::string, t_TypedValue> modified_pre_existing;
                modified_pre_existing.reserve(m_Environment.size());
                for (auto&& [key, value] : m_Environment)
                {
                    if (pre_loop_variables.contains(key))
                    {
                        modified_pre_existing.try_emplace(key, value);
                    }
                }

                // Pop the loop scope
                PopScope();
                
                // Restore the modified values of pre-existing variables
                for (auto&& [key, value] : modified_pre_existing)
                {
                    m_Environment.try_emplace(key, value);
                }
                
                m_LoopDepth--;
            }
			catch (...)
			{
				// Ensure scope is cleaned and convert to runtime error
				PopScope();
				m_LoopDepth--;
				return Expected<int, t_ErrorInfo>
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
        t_VarStmt *var_stmt = As<t_VarStmt>(stmt)
    )
    {
        // Check if variable is already declared in current scope
        Expected<int, t_ErrorInfo> declare_result =
        DeclareVariable(var_stmt->name, 0); 

        if (!declare_result)
        {
            return declare_result.Error();
        }

        t_TypedValue typed_value;
        if (var_stmt->initializer)
        {
            Expected<std::string, t_ErrorInfo> value_result = 
            Evaluate(var_stmt->initializer.get());

            if (!value_result)
            {
                return value_result.Error();
            }
            
            std::string value = value_result.Value();
            e_ValueType type = DetectType(value);
            typed_value = t_TypedValue(value, type);
        }
        
        // Add to constants set if this is a constant declaration
        if (var_stmt->is_const)
        {
            m_Constants.insert(var_stmt->name);
        }
        
        m_Environment[var_stmt->name] = typed_value;
    }
    else if (t_DisplayStmt *display_stmt = As<t_DisplayStmt>(stmt))
    {
        bool first = true;
        for (const auto &EXPR : display_stmt->expressions)
        {
            if (!first)
            {
                WriteOutput(" ");
            }
            first = false;

            Expected<std::string, t_ErrorInfo> value_result = 
            Evaluate(EXPR.get());

            if (!value_result)
            {
                return value_result.Error();
            }
            
            std::string value = value_result.Value();
            WriteOutput(value);
        }
        WriteOutput("\n");
    }
    else if (t_GetinStmt *getin_stmt = As<t_GetinStmt>(stmt))
    {
        const std::string &var_name = getin_stmt->variable_name;

        auto it = m_Environment.find(var_name);
        if (it == m_Environment.end())
        {
            return Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "Variable '" + var_name +
                    "' must be declared with 'auto' keyword before use"
                )
            );
        }

        if(m_Constants.count(var_name))
        {
            return Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "Cannot modify constant '" + var_name + "' with getin"
                )
            );
        }

        t_TypedValue &current_value = it->second;
        std::string input_line;

        // Read the entire line for all types first
        if (!std::getline(std::cin, input_line))
        {
            // If getline fails, clear the error state
            std::cin.clear();
            return Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "Failed to read input for variable '" + var_name + "'"
                )
            );
        }

        // Trim trailing carriage return if present (for cross-platform compatibility)
        if (!input_line.empty() && input_line.back() == '\r')
        {
            input_line.pop_back();
        }

        switch (current_value.type)
        {
        case e_ValueType::NUMBER:
            {
                try
                {
                    double number_value = std::stod(input_line);
                    AssignToVisibleVariable
                    (
                        var_name,
                        t_TypedValue(number_value),
                        m_Environment,
                        m_ScopeStack
                    );
                }
                catch (...)
                {
                    return Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR,
                            "Invalid type for variable " +
                            var_name
                        )
                    );
                }
                break;
            }

        case e_ValueType::BOOLEAN:
            {
                // Convert string to lowercase for case-insensitive comparison
                std::string lower_input = input_line;
                std::transform
                (
                    lower_input.begin(), 
                    lower_input.end(), 
                    lower_input.begin(), 
                    ::tolower
                );

                // Try parsing as boolean
                bool bool_value = false;
                if (lower_input == "true" || lower_input == "1")
                {
                    bool_value = true;
                }
                else if (lower_input == "false" || lower_input == "0")
                {
                    bool_value = false;
                }
                else
                {
                    // Try to parse as number
                    try
                    {
                        double num = std::stod(input_line);
                        bool_value = (num != 0.0);
                    }
                    catch (...)
                    {
                        return Expected<int, t_ErrorInfo>
                        (
                            t_ErrorInfo
                            (
                                e_ErrorType::RUNTIME_ERROR,
                                "Failed to convert input to boolean for variable '" +
                                var_name    + 
                                "'. Valid values: true, false, 1, 0"
                            )
                        );
                    }
                }

                std::string stored_value = bool_value ? "true" : "false";
                AssignToVisibleVariable
                (
                    var_name,
                    t_TypedValue(stored_value, e_ValueType::BOOLEAN),
                    m_Environment,
                    m_ScopeStack
                );
                break;
            }

        case e_ValueType::STRING:
            {
                // For strings, we already have the input line
                AssignToVisibleVariable
                (
                    var_name,
                    t_TypedValue(input_line, e_ValueType::STRING),
                    m_Environment,
                    m_ScopeStack
                );
                break;
            }

        case e_ValueType::NIL:
        default:
            {
                // For untyped variables, infer type from input
                e_ValueType inferred_type = DetectType(input_line);
                AssignToVisibleVariable
                (
                    var_name,
                    t_TypedValue(input_line, inferred_type),
                    m_Environment,
                    m_ScopeStack
                );
                break;
            }
        }
    }
    else if 
    (
        t_BenchmarkStmt *benchmark_stmt = As<t_BenchmarkStmt>(stmt)
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
        Expected<int, t_ErrorInfo> body_result = 
        Execute(benchmark_stmt->body.get());

        FlushOutput();
        m_BufferOutput = previous_buffering;
        
        if (!body_result)
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
    else if (As<t_EmptyStmt>(stmt))
    {
        // Do nothing for empty statements
    }
    else if 
    (
        t_FunStmt *fun_stmt =
        As<t_FunStmt>(stmt)
    )
    {
        // Native m_Functions are implemented in C++.
        // The fun declaration is currently a no-op placeholder.
        (void)fun_stmt;
    }
    else if 
    (
        t_ExpressionStmt *expr_stmt = 
        As<t_ExpressionStmt>(stmt)
    )
    {
        Expected<std::string, t_ErrorInfo> result =
        Evaluate(expr_stmt->expression.get());

        if (!result)
        {
            return result.Error();
        }
    }
    else if 
    (
        t_ReturnStmt *return_stmt = 
        As<t_ReturnStmt>(stmt)
    )
    {
        // Evaluate the return value expression (if any)
        if (return_stmt->value)
        {
            Expected<std::string, t_ErrorInfo> result =
            Evaluate(return_stmt->value.get());

            if (!result)
            {
                return result.Error();
            }
            
            m_ReturnValue = result.Value();
        }
        else
        {
            // No return value specified, return nil
            m_ReturnValue = "nil";
        }
        
        // Set the returning flag to indicate we should stop execution
        m_IsReturning = true;
        return Expected<int, t_ErrorInfo>(0);
    }
    
    return Expected<int, t_ErrorInfo>(0); // Success represented by 0
}

bool Interpreter::IsTruthy(const std::string &value)
{
    return value != "false" && value != "nil";
}

Expected<std::string, t_ErrorInfo> Interpreter::Evaluate(t_Expr *expr)
{
    if (t_LiteralExpr *literal = As<t_LiteralExpr>(expr))
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
                    Expected<std::string, t_ErrorInfo> expr_result = 
                    EvaluateFormatExpression(expression);
                    if (!expr_result)
                    {
                        return expr_result.Error();
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

            return Expected<std::string, t_ErrorInfo>(result);
        }

        return Expected<std::string, t_ErrorInfo>(literal->value);
    }

    if (t_GroupingExpr *grouping = As<t_GroupingExpr>(expr))
    {
        return Evaluate(grouping->expression.get());
    }

    if (t_CallExpr *call_expr = As<t_CallExpr>(expr))
    {
        // User-defined m_Functions
        if 
        (
            auto fun_it = m_Functions.find(call_expr->callee);
            fun_it != m_Functions.end()
        )
        {
            t_FunStmt *fun_stmt = fun_it->second;
            if (call_expr->arguments.size() != fun_stmt->parameters.size())
            {
                return Expected<std::string, t_ErrorInfo>
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
                Expected<std::string, t_ErrorInfo> arg_result =
                Evaluate(call_expr->arguments[i].get());
                if (!arg_result)
                {
                    return arg_result;
                }

                std::string arg_value = arg_result.Value();
                e_ValueType arg_type = DetectType(arg_value);
                argument_values.emplace_back(arg_value, arg_type);
            }

            // Preserve modifications to pre-existing variables as in for-loops
            std::unordered_set<std::string_view> pre_call_variables; 
            pre_call_variables.reserve(m_Environment.size());
            for (auto&& [key, _]: m_Environment)
            {
                pre_call_variables.insert(key);
            }

            PushScope();
            try
            {
                // Bind parameters in the new scope
                for (size_t i = 0; i < fun_stmt->parameters.size(); ++i)
                {
                    const std::string& param_name = fun_stmt->parameters[i];
                    m_Environment[param_name] = argument_values[i];
                }

                std::string return_value = "nil";
                if (fun_stmt->body)
                {
                    m_IsReturning = false;
                
                    Expected<int, t_ErrorInfo> body_result =
                        Execute(fun_stmt->body.get());
                
                    if (!body_result)
                    {
                        PopScope();
                        return body_result.Error();
                    }
                
                    if (m_IsReturning)
                    {
                        return_value = m_ReturnValue;
                        m_IsReturning = false;
                    }
                }
            
                // Preserve modifications to variables that existed before the call
                std::unordered_map<std::string, t_TypedValue> modified_pre_existing;
                modified_pre_existing.reserve(pre_call_variables.size());
            
                for (auto&& [key, value] : m_Environment)
                {
                    if (pre_call_variables.contains(key))
                    {
                        modified_pre_existing.emplace(key, value);
                    }
                }
            
                // Exit function scope
                PopScope();
            
                // Restore preserved variables
                for (auto&& [key, value] : modified_pre_existing)
                {
                    m_Environment.find(key)->second = value;
                }
            
                return Expected<std::string, t_ErrorInfo>(return_value);
            }

            catch (...)
            {
                PopScope();
                return Expected<std::string, t_ErrorInfo>
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

        return Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR,
                "Undefined function '" + call_expr->callee + "'",
                call_expr->line
            )
        );
    }

    if (t_UnaryExpr *unary = As<t_UnaryExpr>(expr))
    {
        Expected<std::string, t_ErrorInfo> right_result = 
        Evaluate(unary->right.get());
        if (!right_result)
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
                As<t_LiteralExpr>(unary->right.get())
            )
            {
                e_ValueType type = DetectType(right_literal->value);
                if (type == e_ValueType::NUMBER)
                {
                    try
                    {
                        double value = std::stod(right_literal->value);
                        return Expected<std::string, t_ErrorInfo>
                        (
                            FormatNumber(-value)
                        );
                    }
                    catch (...) {}
                }
            }
            return Expected<std::string, t_ErrorInfo>("-" + right);

        case e_TokenType::BANG:
            return Expected<std::string, t_ErrorInfo>
            (
                (right == "false" || right == "0") ? "true" : "false"
            );

        default:
            return Expected<std::string, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR,
                    "Unsupported unary operator"
                )
            );
        }
    }

    if (t_PrefixExpr *prefix = As<t_PrefixExpr>(expr))
    {
        if
        (
            t_VariableExpr *var_expr = 
            As<t_VariableExpr>(prefix->operand.get())
        )
        {
            std::string var_name = var_expr->name;
            auto it = m_Environment.find(var_name);
            
            if (it == m_Environment.end())
            {
                return Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Undefined variable '" + var_name + "'"
                    )
                );
            }
            
            if (m_Constants.count(var_name))
            {
                return Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR,
                        "Cannot modify constant '" + var_name + "'"
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
                    m_Environment[var_name] = t_TypedValue(new_value);
                    return Expected<std::string, t_ErrorInfo>
                    (
                        std::to_string(new_value)
                    );
                }
                else if (prefix->op.type == e_TokenType::MINUS_MINUS)
                {
                    double new_value = current_value.numeric_value - 1.0;
                    m_Environment[var_name] = t_TypedValue(new_value);
                    return Expected<std::string, t_ErrorInfo>
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
                        m_Environment[var_name] =
                        t_TypedValue
                        (
                            new_value, e_ValueType::NUMBER
                        );
                        return Expected<std::string, t_ErrorInfo>(new_value);
                    }
                    else if (prefix->op.type == e_TokenType::MINUS_MINUS)
                    {
                        num_value -= 1.0;
                        std::string new_value = std::to_string(num_value);

                        // Remove trailing zeros and decimal point if not needed
                        new_value.erase
                        (
                            new_value.find_last_not_of('0') + 1,
                            std::string::npos
                        );
                        new_value.erase
                        (
                            new_value.find_last_not_of('.') + 1,
                            std::string::npos
                        );
                        m_Environment[var_name] = 
                        t_TypedValue
                        (
                            new_value, e_ValueType::NUMBER
                        );
                        return Expected<std::string, t_ErrorInfo>(new_value);
                    }
                }
                catch (...)
                {
                    return Expected<std::string, t_ErrorInfo>
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
        return Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Prefix increment/decrement can only be applied to variables"
            )
        );
    }

    if (t_PostfixExpr *postfix = As<t_PostfixExpr>(expr))
    {
        if 
        (
            t_VariableExpr *var_expr = 
            As<t_VariableExpr>(postfix->operand.get())
        )
        {
            std::string var_name = var_expr->name;
            auto it = m_Environment.find(var_name);
            if (it == m_Environment.end())
            {
                return Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Undefined variable '" + var_name + "'"
                    )
                );
            }

            if (m_Constants.count(var_name))
            {
                return Expected<std::string, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR,
                        "Cannot modify constant '" + var_name + "'"
                    )
                );
            }

            t_TypedValue& current_value = it->second;
            std::string old_value = current_value.value;
            if (postfix->op.type == e_TokenType::PLUS_PLUS)
            {
                if (current_value.has_numeric_value)
                {
                    current_value.numeric_value += 1.0;
                    current_value.value = FormatNumber
                    (
                        current_value.numeric_value
                    );
                }
                else
                {
                    try
                    {
                        double num_value = std::stod(current_value.value);
                        num_value += 1.0;
                        current_value.value = FormatNumber(num_value);
                        current_value.has_numeric_value = true;
                        current_value.numeric_value = num_value;
                        current_value.type = e_ValueType::NUMBER;
                    }
                    catch (...)
                    {
                        return Expected<std::string, t_ErrorInfo>
                        (
                            t_ErrorInfo
                            (
                                e_ErrorType::RUNTIME_ERROR, 
                                "Cannot perform increment on non-numeric value"
                            )
                        );
                    }
                }
            }
            else if (postfix->op.type == e_TokenType::MINUS_MINUS)
            {
                if (current_value.has_numeric_value)
                {
                    current_value.numeric_value -= 1.0;
                    current_value.value = FormatNumber
                    (
                        current_value.numeric_value
                    );
                }
                else
                {
                    try
                    {
                        double num_value = std::stod(current_value.value);
                        num_value -= 1.0;
                        current_value.value = FormatNumber(num_value);
                        current_value.has_numeric_value = true;
                        current_value.numeric_value = num_value;
                        current_value.type = e_ValueType::NUMBER;
                    }
                    catch (...)
                    {
                        return Expected<std::string, t_ErrorInfo>
                        (
                            t_ErrorInfo
                            (
                                e_ErrorType::RUNTIME_ERROR, 
                                "Cannot perform decrement on non-numeric value"
                            )
                        );
                    }
                }
            }

            // Return the OLD value (postfix behavior)
            // Also ensure the variable is updated in all scopes
            AssignToVisibleVariable
            (
                var_name, 
                current_value, 
                m_Environment, 
                m_ScopeStack
            );

            return Expected<std::string, t_ErrorInfo>(old_value);
        }
        return Expected<std::string, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Postfix increment/decrement can only be applied to variables"
            )
        );
    }

    if (t_BinaryExpr *binary = As<t_BinaryExpr>(expr))
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
                As<t_VariableExpr>(binary->left.get())
            )
            {
                std::string var_name = var_expr->name;
                
                // Check if variable was properly declared with 'auto' keyword
                // All variables must be declared before use
                if (!m_Environment.contains(var_name))
                {
                    return Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Variable '" + var_name + "' must be declared with 'auto' keyword before use"
                        )
                    );
                }
                
                if (m_Constants.count(var_name))
                {
                    return Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR,
                            "Cannot assign to constant '" + var_name + "'"
                        )
                    );
                }

                // For compound assignments, we need to get the current value first
                Expected<std::string, t_ErrorInfo> left_result = 
                Evaluate(binary->left.get());

                if (!left_result)
                {
                    return left_result;
                }
                std::string left_value = left_result.Value();
                
                Expected<std::string, t_ErrorInfo> right_result = 
                Evaluate(binary->right.get());

                if (!right_result)
                {
                    return right_result;
                }
                std::string right_value = right_result.Value();
                
                Expected<std::string, t_ErrorInfo> final_value_result
                (
                    right_value
                );
                
                // Handle compound assignments by converting them to regular operations
                switch (binary->op.type)
                {
                case e_TokenType::EQUAL:
                    final_value_result = 
                    Expected<std::string, t_ErrorInfo>(right_value);
                    break;
                    
                case e_TokenType::PLUS_EQUAL:
                    {
                        try 
                        {
                            double left_val = std::stod(left_value);
                            double right_val = std::stod(right_value);
                            final_value_result = 
                            Expected<std::string, t_ErrorInfo>
                            (
                                FormatNumber(left_val + right_val)
                            );
                        }
                        catch (...)
                        {
                            final_value_result = Expected<std::string, t_ErrorInfo>
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
                            final_value_result = 
                            Expected<std::string, t_ErrorInfo>
                            (
                                FormatNumber(left_val - right_val)
                            );
                        }
                        catch (...)
                        {
                            final_value_result = 
                            Expected<std::string, t_ErrorInfo>
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
                            final_value_result = 
                            Expected<std::string, t_ErrorInfo>
                            (
                                FormatNumber(left_val * right_val)
                            );
                        }
                        catch (...)
                        {
                            final_value_result = Expected<std::string, t_ErrorInfo>
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
                                final_value_result = Expected<std::string, t_ErrorInfo>
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
                                final_value_result = 
                                Expected<std::string, t_ErrorInfo>
                                (
                                    FormatNumber(std::fmod(left_val, right_val))
                                );
                            }
                        }
                        catch (...)
                        {
                            final_value_result = Expected<std::string, t_ErrorInfo>
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
                                final_value_result = Expected<std::string, t_ErrorInfo>
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
                                final_value_result = 
                                Expected<std::string, t_ErrorInfo>
                                (
                                    FormatNumber(left_val / right_val)
                                );
                            }
                        }
                        catch (...)
                        {
                            final_value_result = Expected<std::string, t_ErrorInfo>
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
                    final_value_result = 
                    Expected<std::string, t_ErrorInfo>(right_value);
                    break;
                }
                
                if (!final_value_result)
                {
                    return final_value_result;
                }
                
                std::string final_value = final_value_result.Value();
                e_ValueType type = DetectType(final_value);
                
                // Enforce static typing - check if the new value type matches the declared variable type
                if 
                (
                    auto it = m_Environment.find(var_name); 
                    it != m_Environment.end()
                )
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
                        
                        return Expected<std::string, t_ErrorInfo>
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
                
                // Update the variable in the m_Environment
                AssignToVisibleVariable
                (
                    var_name, 
                    t_TypedValue(final_value, type), 
                    m_Environment, 
                    m_ScopeStack
                );
                
                // Return the assigned value
                return Expected<std::string, t_ErrorInfo>(final_value);
            }
            else
            {
                return Expected<std::string, t_ErrorInfo>
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
        Expected<std::string, t_ErrorInfo> left_result = 
        Evaluate(binary->left.get());
        if (!left_result)
        {
            return left_result;
        }
        std::string left_str = left_result.Value();

        if (binary->op.type == e_TokenType::AND)
        {
            if (!IsTruthy(left_str))
            {
                return Expected<std::string, t_ErrorInfo>("false");
            }

            Expected<std::string, t_ErrorInfo> right_result =
            Evaluate(binary->right.get());
            if (!right_result)
            {
                return right_result;
            }

            return Expected<std::string, t_ErrorInfo>
            (
                IsTruthy(right_result.Value()) ? "true" : "false"
            );
        }

        if (binary->op.type == e_TokenType::OR)
        {
            if (IsTruthy(left_str))
            {
                return Expected<std::string, t_ErrorInfo>("true");
            }

            Expected<std::string, t_ErrorInfo> right_result =
            Evaluate(binary->right.get());
            if (!right_result)
            {
                return right_result;
            }

            return Expected<std::string, t_ErrorInfo>
            (
                IsTruthy(right_result.Value()) ? "true" : "false"
            );
        }
        
        Expected<std::string, t_ErrorInfo> right_result = 
        Evaluate(binary->right.get());
        if (!right_result)
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
                    return Expected<std::string, t_ErrorInfo>
                    (
                        FormatNumber(left_val + right_val)
                    );
                }
                catch (...)
                {
                    return Expected<std::string, t_ErrorInfo>
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
                    return Expected<std::string, t_ErrorInfo>
                    (
                        FormatNumber(left_val - right_val)
                    );
                }
                catch (...)
                {
                    return Expected<std::string, t_ErrorInfo>
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
                    return Expected<std::string, t_ErrorInfo>
                    (
                        FormatNumber(left_val * right_val)
                    );
                }
                catch (...)
                {
                    return Expected<std::string, t_ErrorInfo>
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
                        return Expected<std::string, t_ErrorInfo>
                        (
                            t_ErrorInfo
                            (
                                e_ErrorType::RUNTIME_ERROR, 
                                "Modulus by zero"
                            )
                        );
                    }
                    
                    return Expected<std::string, t_ErrorInfo>
                    (
                        FormatNumber(std::fmod(left_val, right_val))
                    );
                }
                catch (...)
                {
                    return Expected<std::string, t_ErrorInfo>
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
                        return Expected<std::string, t_ErrorInfo>
                        (
                            t_ErrorInfo
                            (
                                e_ErrorType::RUNTIME_ERROR, 
                                "Division by zero"
                            )
                        );
                    }
                    
                    return Expected<std::string, t_ErrorInfo>
                    (
                        FormatNumber(left_val / right_val)
                    );
                }
                catch (...)
                {
                    return Expected<std::string, t_ErrorInfo>
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
                Expected<bool, t_ErrorInfo> comparison_result = 
                PerformComparison(left_typed, binary->op.type, right_typed);
                if (!comparison_result)
                {
                    return comparison_result.Error();
                }
                return Expected<std::string, t_ErrorInfo>
                (
                    comparison_result.Value() ? "true" : "false"
                );
            }
        
        default:
            {
                return Expected<std::string, t_ErrorInfo>
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

    if (t_VariableExpr *variable = As<t_VariableExpr>(expr))
    {
        if 
        (
            auto it = m_Environment.find(variable->name);
            it != m_Environment.end()
        )
        {
            return Expected<std::string, t_ErrorInfo>(it->second.value);
        }
        // Variables must be declared with 'auto' keyword before use
        return Expected<std::string, t_ErrorInfo>
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

    return Expected<std::string, t_ErrorInfo>("");
}

std::string Interpreter::Stringify(const std::string &value)
{
    // In a real implementation, we would handle different types
    return value;
}

Expected<std::string, t_ErrorInfo> Interpreter::EvaluateFormatExpression
(
    const std::string &expr_str
)
{
    try
    {
        // Handle empty expressions
        if (expr_str.empty())
        {
            return Expected<std::string, t_ErrorInfo>("");
        }

        // For simple variable names, look them up directly in the m_Environment
        if 
        (
            auto it = m_Environment.find(expr_str);
            it != m_Environment.end()
        )
        {
            return Expected<std::string, t_ErrorInfo>(it->second.value);
        }
        
        // For numeric literals, return them directly
        if (DetectType(expr_str) == e_ValueType::NUMBER)
        {
            return Expected<std::string, t_ErrorInfo>(expr_str);
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
            Expected<std::string, t_ErrorInfo> left_result = 
            EvaluateFormatExpression(left_str);
            if (!left_result)
            {
                return left_result;
            }
            Expected<std::string, t_ErrorInfo> right_result = 
            EvaluateFormatExpression(right_str);
            if (!right_result)
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
                return Expected<std::string, t_ErrorInfo>
                (
                    FormatNumber(left_val * right_val)
                );
            }
            catch (...)
            {
                return Expected<std::string, t_ErrorInfo>
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
            Expected<std::string, t_ErrorInfo> left_result = 
            EvaluateFormatExpression(left_str);
            if (!left_result)
            {
                return left_result;
            }
            Expected<std::string, t_ErrorInfo> right_result = 
            EvaluateFormatExpression(right_str);
            if (!right_result)
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
                    return Expected<std::string, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Division by zero"
                        )
                    );
                }
                
                return Expected<std::string, t_ErrorInfo>
                (
                    FormatNumber(left_val / right_val)
                );
            }
            catch (...)
            {
                return Expected<std::string, t_ErrorInfo>
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
            Expected<std::string, t_ErrorInfo> left_result = 
            EvaluateFormatExpression(left_str);
            if (!left_result)
            {
                return left_result;
            }
            Expected<std::string, t_ErrorInfo> right_result = 
            EvaluateFormatExpression(right_str);
            if (!right_result)
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
                return Expected<std::string, t_ErrorInfo>
                (
                    FormatNumber(left_val + right_val)
                );
            }
            catch (...)
            {
                return Expected<std::string, t_ErrorInfo>
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
            Expected<std::string, t_ErrorInfo> left_result = 
            EvaluateFormatExpression(left_str);
            if (!left_result)
            {
                return left_result;
            }
            Expected<std::string, t_ErrorInfo> right_result = 
            EvaluateFormatExpression(right_str);
            if (!right_result)
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
                return Expected<std::string, t_ErrorInfo>
                (
                    FormatNumber(left_val - right_val)
                );
            }
            catch (...)
            {
                return Expected<std::string, t_ErrorInfo>
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
        return Expected<std::string, t_ErrorInfo>(expr_str);
    }
    catch (...)
    {
        // If parsing fails, treat as literal text
        return Expected<std::string, t_ErrorInfo>(expr_str);
    }
}

// Helper function to check if a for loop is a simple numeric loop pattern: for (auto i = 0; i < N; i++)
bool Interpreter::IsSimpleNumericLoop(t_ForStmt* for_stmt)
{
    // Check if initializer is: auto var = 0
    if (!for_stmt->initializer) return false;
    
    t_VarStmt* init_var = As<t_VarStmt>(for_stmt->initializer.get());
    if (!init_var || !init_var->initializer) return false;
    
    t_LiteralExpr* init_literal = 
    As<t_LiteralExpr>(init_var->initializer.get());
    if (!init_literal || init_literal->token_type != e_TokenType::NUMBER)
    {
        return false;
    }

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
    As<t_BinaryExpr>(for_stmt->condition.get());
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
    As<t_VariableExpr>(condition_binary->left.get());
    if 
    (
        !condition_var || 
        condition_var->name != init_var->name
    ) return false;
    
    t_LiteralExpr* condition_literal = 
    As<t_LiteralExpr>(condition_binary->right.get());
    if 
    (
        !condition_literal || 
        condition_literal->token_type != e_TokenType::NUMBER
    ) return false;

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
    As<t_PostfixExpr>(for_stmt->increment.get());
    if (postfix_inc) 
    {
        t_VariableExpr* inc_var = 
        As<t_VariableExpr>(postfix_inc->operand.get());
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
    As<t_PrefixExpr>(for_stmt->increment.get());

    if (prefix_inc) 
    {
        t_VariableExpr* inc_var = 
        As<t_VariableExpr>(prefix_inc->operand.get());
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
        if 
        (
            t_BinaryExpr* assign = 
            As<t_BinaryExpr>(for_stmt->increment.get())
        )
        {
            if
            (
                assign->op.type != e_TokenType::PLUS_EQUAL &&
                assign->op.type != e_TokenType::MINUS_EQUAL
            )
            {
                return false;
            }

            t_VariableExpr* lhs = As<t_VariableExpr>
            (
                assign->left.get()
            );
            t_LiteralExpr* rhs = As<t_LiteralExpr>
            (
                assign->right.get()
            );
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

            step = 
            (
                assign->op.type == e_TokenType::PLUS_EQUAL
            ) ? rhs_int : -rhs_int;
        }
        else
        {
            return false;
        }
    }

    if
    (
        condition_op == e_TokenType::LESS || 
        condition_op == e_TokenType::LESS_EQUAL
    )
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
    return true;
}

// Ultra-fast optimization for simple accumulation loops: for (auto i = 0; i < N; i++) { var += i; }
bool Interpreter::IsSimpleAccumulationLoop(t_ForStmt* for_stmt)
{
    // First check if it's a simple numeric loop
    if (!IsSimpleNumericLoop(for_stmt)) return false;

    t_VarStmt* init_var = As<t_VarStmt>
    (
        for_stmt->initializer.get()
    );
    t_LiteralExpr* init_literal =
    As<t_LiteralExpr>(init_var->initializer.get());

    t_BinaryExpr* condition_binary =
    As<t_BinaryExpr>(for_stmt->condition.get());

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
        As<t_PostfixExpr>(for_stmt->increment.get())
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
        As<t_PrefixExpr>(for_stmt->increment.get())
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
    t_BlockStmt* body_block = As<t_BlockStmt>(for_stmt->body.get());
    if (!body_block || body_block->statements.size() != 1) return false;
    
    t_ExpressionStmt* expr_stmt = 
    As<t_ExpressionStmt>(body_block->statements[0].get());
    if (!expr_stmt) return false;
    
    // Check if the expression is a binary assignment (+=)
    t_BinaryExpr* binary_expr = 
    As<t_BinaryExpr>(expr_stmt->expression.get());
    if 
    (
        !binary_expr || 
        binary_expr->op.type != e_TokenType::PLUS_EQUAL
    ) return false;
    
    // Check if left side is a variable
    t_VariableExpr* left_var = 
    As<t_VariableExpr>(binary_expr->left.get());
    if (!left_var) return false;
    
    // Check if right side is the loop variable
    t_VariableExpr* right_var = 
    As<t_VariableExpr>(binary_expr->right.get());
    if (!right_var) return false;
    
    // Check if right side matches loop variable
    if (right_var->name != init_var->name) return false;
    
    return true;
}

// Ultra-fast native execution for simple accumulation loops
Expected<int, t_ErrorInfo> Interpreter::ExecuteAccumulationLoop
(
    t_ForStmt* for_stmt
)
{
    // Extract loop parameters
    t_VarStmt* init_var = As<t_VarStmt>
    (
        for_stmt->initializer.get()
    );
    t_BinaryExpr* condition_binary = 
    As<t_BinaryExpr>(for_stmt->condition.get());
    
    t_LiteralExpr* condition_literal = 
    As<t_LiteralExpr>(condition_binary->right.get());
    
    // Get the loop limit
    int limit = static_cast<int>(std::stod(condition_literal->value));
    
    // Get accumulation variable name
    t_BlockStmt* body_block = As<t_BlockStmt>(for_stmt->body.get());
    t_ExpressionStmt* expr_stmt = 
    As<t_ExpressionStmt>(body_block->statements[0].get());

    t_BinaryExpr* binary_expr = 
    As<t_BinaryExpr>(expr_stmt->expression.get());
    
    t_VariableExpr* acc_var = 
    As<t_VariableExpr>(binary_expr->left.get());
    
    std::string acc_var_name = acc_var->name;
    std::string loop_var_name = init_var->name;
    
    // Check if accumulation variable exists and get its initial value
    auto acc_it = m_Environment.find(acc_var_name);
    if (acc_it == m_Environment.end())
    {
        return Expected<int, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Variable '" + 
                acc_var_name + 
                "' must be declared with 'auto' keyword before use"
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
            return Expected<int, t_ErrorInfo>
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
    m_Environment[acc_var_name] = t_TypedValue(accumulator);

    return Expected<int, t_ErrorInfo>(0);
}

// Check if a for loop contains a nested loop with arithmetic accumulation
bool Interpreter::IsNestedArithmeticLoop(t_ForStmt* for_stmt)
{
    // First check if outer loop is a simple numeric loop
    if (!IsSimpleNumericLoop(for_stmt)) return false;
    
    // Check if body is a block with exactly one statement (the inner loop)
    t_BlockStmt* body_block = 
    As<t_BlockStmt>(for_stmt->body.get());

    if (!body_block || body_block->statements.size() != 1) return false;
    
    // Check if the statement is a nested for loop
    t_ForStmt* inner_for = 
    As<t_ForStmt>(body_block->statements[0].get());

    if (!inner_for) return false;
    
    // Check if inner loop is also a simple numeric loop
    if (!IsSimpleNumericLoop(inner_for)) return false;
    
    // Check if inner loop body contains arithmetic accumulation
    t_BlockStmt* inner_body = As<t_BlockStmt>(inner_for->body.get());
    if (!inner_body || inner_body->statements.size() != 1) return false;
    
    t_ExpressionStmt* expr_stmt = 
    As<t_ExpressionStmt>(inner_body->statements[0].get());
    if (!expr_stmt) return false;
    
    // Check if the expression is a binary assignment (+=, -=, *=, /=, %=)
    t_BinaryExpr* binary_expr = 
    As<t_BinaryExpr>(expr_stmt->expression.get());
    if 
    (
        !binary_expr || 
        (
            binary_expr->op.type != e_TokenType::PLUS_EQUAL &&
            binary_expr->op.type != e_TokenType::MINUS_EQUAL &&
            binary_expr->op.type != e_TokenType::STAR_EQUAL &&
            binary_expr->op.type != e_TokenType::SLASH_EQUAL &&
            binary_expr->op.type != e_TokenType::MODULUS_EQUAL
        )
    ) return false;
    
    // Check if left side is a variable
    t_VariableExpr* left_var = 
    As<t_VariableExpr>(binary_expr->left.get());
    if (!left_var) return false;
    
    // Check if right side is a binary expression with loop variables
    t_BinaryExpr* right_binary = 
    As<t_BinaryExpr>(binary_expr->right.get());
    if (!right_binary) return false;
    
    // Get loop variable names
    t_VarStmt* outer_init = 
    As<t_VarStmt>(for_stmt->initializer.get());
    
    t_VarStmt* inner_init = 
    As<t_VarStmt>(inner_for->initializer.get());
    if (!outer_init || !inner_init) return false;
    
    std::string outer_var = outer_init->name;
    std::string inner_var = inner_init->name;
    
    // Check if right side is arithmetic with loop variables (i + j, i - j, etc.)
    t_VariableExpr* right_left_var = 
    As<t_VariableExpr>(right_binary->left.get());

    t_VariableExpr* right_right_var = 
    As<t_VariableExpr>(right_binary->right.get());
    
    if (!right_left_var || !right_right_var) return false;
    
    // Check if the binary expression uses loop variables
    bool uses_outer = 
    (
        right_left_var->name == outer_var || 
        right_right_var->name == outer_var
    );
    bool uses_inner = 
    (
        right_left_var->name == inner_var || 
        right_right_var->name == inner_var
    );
    
    // Must use at least one loop variable
    if (!uses_outer && !uses_inner) return false;
    
    // Check if the operator is arithmetic (+, -, *, /, %)
    e_TokenType op = right_binary->op.type;
    if 
    (
        op != e_TokenType::PLUS &&
        op != e_TokenType::MINUS &&
        op != e_TokenType::STAR &&
        op != e_TokenType::SLASH &&
        op != e_TokenType::MODULUS
    ) return false;
    
    return true;
}

// Ultra-fast native execution for nested loops with arithmetic expressions
Expected<int, t_ErrorInfo> Interpreter::ExecuteNestedArithmeticLoop
(
    t_ForStmt* for_stmt
)
{
    // Extract outer loop parameters
    t_VarStmt* outer_init = 
    As<t_VarStmt>(for_stmt->initializer.get());
    t_BinaryExpr* outer_condition = 
    As<t_BinaryExpr>(for_stmt->condition.get());
    t_LiteralExpr* outer_limit_lit = 
    As<t_LiteralExpr>(outer_condition->right.get());
    
    int outer_limit = static_cast<int>(std::stod(outer_limit_lit->value));
    std::string outer_var = outer_init->name;
    
    // Extract inner loop parameters
    t_BlockStmt* body_block = As<t_BlockStmt>(for_stmt->body.get());
    t_ForStmt* inner_for = 
    As<t_ForStmt>(body_block->statements[0].get());
    
    t_VarStmt* inner_init = 
    As<t_VarStmt>(inner_for->initializer.get());
    t_BinaryExpr* inner_condition = 
    As<t_BinaryExpr>(inner_for->condition.get());
    t_LiteralExpr* inner_limit_lit = 
    As<t_LiteralExpr>(inner_condition->right.get());
    
    int inner_limit = static_cast<int>(std::stod(inner_limit_lit->value));
    std::string inner_var = inner_init->name;
    
    // Extract accumulation variable and expression
    t_BlockStmt* inner_body = As<t_BlockStmt>(inner_for->body.get());
    t_ExpressionStmt* expr_stmt = 
    As<t_ExpressionStmt>(inner_body->statements[0].get());
    t_BinaryExpr* assign_expr = 
    As<t_BinaryExpr>(expr_stmt->expression.get());
    
    t_VariableExpr* acc_var_expr = 
    As<t_VariableExpr>(assign_expr->left.get());
    std::string acc_var_name = acc_var_expr->name;
    
    t_BinaryExpr* arithmetic_expr = 
    As<t_BinaryExpr>(assign_expr->right.get());
    e_TokenType arithmetic_op = arithmetic_expr->op.type;
    
    t_VariableExpr* arith_left = 
    As<t_VariableExpr>(arithmetic_expr->left.get());
    t_VariableExpr* arith_right = 
    As<t_VariableExpr>(arithmetic_expr->right.get());
    
    // Check if accumulation variable exists and get its initial value
    auto acc_it = m_Environment.find(acc_var_name);
    if (acc_it == m_Environment.end())
    {
        return Expected<int, t_ErrorInfo>
        (
            t_ErrorInfo
            (
                e_ErrorType::RUNTIME_ERROR, 
                "Variable '" + 
                acc_var_name + 
                "' must be declared with 'auto' keyword before use"
            )
        );
    }
    
    // Get initial accumulator value
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
            return Expected<int, t_ErrorInfo>
            (
                t_ErrorInfo
                (
                    e_ErrorType::RUNTIME_ERROR, 
                    "Variable '" + acc_var_name + "' must be numeric for accumulation"
                )
            );
        }
    }
    
    // NATIVE C++ NESTED LOOP: Direct arithmetic without interpretation overhead
    e_TokenType assign_op = assign_expr->op.type;
    
    for (int i = 0; i < outer_limit; i++)
    {
        for (int j = 0; j < inner_limit; j++)
        {
            // Compute the arithmetic expression value
            double left_val = 0.0;
            double right_val = 0.0;
            
            if (arith_left->name == outer_var)
            {
                left_val = static_cast<double>(i);
            }
            else if (arith_left->name == inner_var)
            {
                left_val = static_cast<double>(j);
            }
            else
            {
                // Should not happen if IsNestedArithmeticLoop passed
                left_val = 0.0;
            }
            
            if (arith_right->name == outer_var)
            {
                right_val = static_cast<double>(i);
            }
            else if (arith_right->name == inner_var)
            {
                right_val = static_cast<double>(j);
            }
            else
            {
                // Should not happen if IsNestedArithmeticLoop passed
                right_val = 0.0;
            }
            
            // Compute arithmetic result
            double arith_result = 0.0;
            switch (arithmetic_op)
            {
            case e_TokenType::PLUS:
                arith_result = left_val + right_val;
                break;
            case e_TokenType::MINUS:
                arith_result = left_val - right_val;
                break;
            case e_TokenType::STAR:
                arith_result = left_val * right_val;
                break;
            case e_TokenType::SLASH:
                if (right_val == 0.0)
                {
                    return Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Division by zero"
                        )
                    );
                }
                arith_result = left_val / right_val;
                break;
            case e_TokenType::MODULUS:
                if (right_val == 0.0)
                {
                    return Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Modulus by zero"
                        )
                    );
                }
                arith_result = std::fmod(left_val, right_val);
                break;
            default:
                return Expected<int, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Unsupported arithmetic operation in optimized loop"
                    )
                );
            }
            
            // Apply assignment operation
            switch (assign_op)
            {
            case e_TokenType::PLUS_EQUAL:
                accumulator += arith_result;
                break;
            case e_TokenType::MINUS_EQUAL:
                accumulator -= arith_result;
                break;
            case e_TokenType::STAR_EQUAL:
                accumulator *= arith_result;
                break;
            case e_TokenType::SLASH_EQUAL:
                if (arith_result == 0.0)
                {
                    return Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Division by zero"
                        )
                    );
                }
                accumulator /= arith_result;
                break;
            case e_TokenType::MODULUS_EQUAL:
                if (arith_result == 0.0)
                {
                    return Expected<int, t_ErrorInfo>
                    (
                        t_ErrorInfo
                        (
                            e_ErrorType::RUNTIME_ERROR, 
                            "Modulus by zero"
                        )
                    );
                }
                accumulator = std::fmod(accumulator, arith_result);
                break;
            default:
                return Expected<int, t_ErrorInfo>
                (
                    t_ErrorInfo
                    (
                        e_ErrorType::RUNTIME_ERROR, 
                        "Unsupported assignment operation in optimized loop"
                    )
                );
            }
        }
    }
    
    // Update the accumulation variable with the result
    m_Environment[acc_var_name] = t_TypedValue(accumulator);
    
    return Expected<int, t_ErrorInfo>(0);
}

Expected<int, t_ErrorInfo> Interpreter::ExecuteSimpleNumericLoop
(
    t_ForStmt* for_stmt
)
{
    // Store variables that existed before the loop
    std::unordered_set<std::string_view> pre_loop_variables;
    pre_loop_variables.reserve(m_Environment.size());

    for(auto&& [key, _] : m_Environment)
    {
        pre_loop_variables.insert(key);
    }
    
    // Push a new scope for the loop
    PushScope();
    m_LoopDepth++;
    
    // Extract loop parameters
    t_VarStmt* init_var = As<t_VarStmt>
    (
        for_stmt->initializer.get()
    );
    t_LiteralExpr* init_literal =
    As<t_LiteralExpr>(init_var->initializer.get());

    t_BinaryExpr* condition_binary = 
    As<t_BinaryExpr>(for_stmt->condition.get());

    t_LiteralExpr* condition_literal = 
    As<t_LiteralExpr>(condition_binary->right.get());

    int start = static_cast<int>(std::stod(init_literal->value));
    int limit = static_cast<int>(std::stod(condition_literal->value));
    e_TokenType condition_op = condition_binary->op.type;

    int step = 1;
    if 
    (
        t_PostfixExpr* postfix_inc = 
        As<t_PostfixExpr>(for_stmt->increment.get())
    )
    {
        step = (postfix_inc->op.type == e_TokenType::PLUS_PLUS) ? 1 : -1;
    }
    else if 
    (
        t_PrefixExpr* prefix_inc = 
        As<t_PrefixExpr>(for_stmt->increment.get())
    )
    {
        step = (prefix_inc->op.type == e_TokenType::PLUS_PLUS) ? 1 : -1;
    }
    else if 
    (
        t_BinaryExpr* assign = 
        As<t_BinaryExpr>(for_stmt->increment.get())
    )
    {
        t_LiteralExpr* rhs = As<t_LiteralExpr>(assign->right.get());
        int rhs_int = static_cast<int>(std::stod(rhs->value));
        step = 
        (
            assign->op.type == e_TokenType::PLUS_EQUAL
        ) ? rhs_int : -rhs_int;
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

    t_BlockStmt* body_block = As<t_BlockStmt>(for_stmt->body.get());
    if 
    (
        allow_break_optimization && 
        body_block               &&
        body_block->statements.size() == 1
    )
    {
        t_IfStmt* if_stmt = As<t_IfStmt>
        (
            body_block->statements[0].get()
        );
        if (if_stmt && !if_stmt->else_branch)
        {
            bool then_is_break = false;

            if (As<t_BreakStmt>(if_stmt->then_branch.get()))
            {
                then_is_break = true;
            }
            else if 
            (
                t_BlockStmt* then_block = 
                As<t_BlockStmt>(if_stmt->then_branch.get())
            )
            {
                if (then_block->statements.size() == 1)
                {
                    if 
                    (
                        As<t_BreakStmt>
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
                As<t_BinaryExpr>(if_stmt->condition.get());

                if 
                (
                    cond_binary && 
                    cond_binary->op.type == e_TokenType::EQUAL_EQUAL
                )
                {
                    t_VariableExpr* cond_var = 
                    As<t_VariableExpr>(cond_binary->left.get());
                    t_LiteralExpr* cond_lit = 
                    As<t_LiteralExpr>(cond_binary->right.get());

                    if (!cond_var || !cond_lit)
                    {
                        cond_var = 
                        As<t_VariableExpr>(cond_binary->right.get());

                        cond_lit = 
                        As<t_LiteralExpr>(cond_binary->left.get());
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
            m_Environment[loop_var_name] = t_TypedValue(static_cast<double>(i));
            
            // Execute body
            m_ControlSignal.clear();
            Expected<int, t_ErrorInfo> body_result = 
            Execute(for_stmt->body.get());

            if (!body_result)
            {
                PopScope(); // Clean up scope before returning
                m_LoopDepth--;
                return body_result;
            }
            if (m_ControlSignal == "break")
            {
                m_ControlSignal.clear();
                break;
            }
            if (m_ControlSignal == "continue")
            {
                m_ControlSignal.clear();
                // Skip increment and go to next iteration when continue is encountered
                i += step;
                continue;
            }
            
            // If we're returning from a function, stop the loop and propagate the return
            if (m_IsReturning)
            {
                PopScope();
                m_LoopDepth--;
                return Expected<int, t_ErrorInfo>(0);
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

        m_ControlSignal.clear();
        m_Environment[loop_var_name] = t_TypedValue
        (
            static_cast<double>(final_i)
        );
    }

    // Before popping scope, preserve modifications to pre-existing variables
    std::unordered_map<std::string, t_TypedValue> modified_pre_existing;
    modified_pre_existing.reserve(pre_loop_variables.size());
    for (auto&& [key, value] : m_Environment)
    {
        if (pre_loop_variables.contains(key))
        {
            modified_pre_existing.try_emplace(key, value);
        }
    }

    // Pop the loop scope
    PopScope();
    
    // Restore the modified values of pre-existing variables
    for (auto&& [key, value] : modified_pre_existing)
    {
        m_Environment.try_emplace(key, value);
    }
    m_LoopDepth--;
    
    return Expected<int, t_ErrorInfo>(0); 
}
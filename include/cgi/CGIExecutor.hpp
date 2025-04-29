#ifndef CGI_EXECUTOR_HPP
#define CGI_EXECUTOR_HPP

#include <string>
#include <map>
#include <vector>

/**
 * @brief CGI exit status enum
 */
enum class CGIExitStatus {
    Success = 0,
    Timeout = -2,
    Error = -1,
    KilledBySignal = -3
};

/**
 * @brief Handles CGI script execution and I/O management
 * 
 * This class is responsible for:
 * - Executing CGI scripts using fork and execve
 * - Managing pipes for script I/O
 * - Setting up environment variables
 */
class CGIExecutor {
public:
    CGIExecutor() = default;
    ~CGIExecutor() = default;

    /**
     * @brief Execute a CGI script
     * @param interpreter Path to the script interpreter (e.g., /usr/bin/python3)
     * @param script_path Path to the CGI script
     * @param request_body Data to pass to script
     * @param env_vars Environment variables for the script
     * @return Pair of {exit_code, output}
     * @throw std::runtime_error on execution failure
     */
    std::pair<int, std::string> execute(
        const std::string& interpreter,
        const std::string& script_path,
        const std::string& request_body,
        const std::map<std::string, std::string>& env_vars);

private:
    // Pipe management
    int input_pipe_[2];   // For writing to script
    int output_pipe_[2];  // For reading from script
    int error_pipe_[2];   // For errors from script

    /**
     * @brief Set up pipes for communication with CGI script
     * @throw std::runtime_error on pipe creation failure
     */
    void setupPipes();

    /**
     * @brief Close all open pipes
     */
    void closePipes();

    /**
     * @brief Convert environment map to array format for execve
     * @param env_vars Map of environment variables
     * @return Vector of strings in KEY=VALUE format
     */
    std::vector<std::string> prepareEnvironment(
        const std::map<std::string, std::string>& env_vars) const;

    /**
     * @brief Read output from CGI script
     * @return Script's output as string
     */
    std::string readOutput();

    /**
     * @brief Wait for child process to finish with timeout handling
     * 
     * @param pid PID of CGI child process
     * @return int exit_code
     */
    int wait_for_child_with_timeout(pid_t pid);

    /**
     * @brief Read the error messeges from the CGI script
     * 
     * @return Script's error as string
     */
    std::string readError();
};

#endif // CGI_EXECUTOR_HPP
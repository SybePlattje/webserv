#include "cgi/CGIExecutor.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <fcntl.h>

std::pair<int, std::string> CGIExecutor::execute(
    const std::string& interpreter,
    const std::string& script_path,
    const std::string& request_body,
    const std::map<std::string, std::string>& env_vars)
{
    setupPipes();

    pid_t pid = fork();
    if (pid == -1) {
        closePipes();
        throw std::runtime_error("Fork failed: " + std::string(strerror(errno)));
    }

    if (pid == 0) {  // Child process
        // Close unused pipe ends
        close(input_pipe_[1]);   // Close write end of input
        close(output_pipe_[0]);  // Close read end of output

        // Redirect stdin to input pipe
        if (dup2(input_pipe_[0], STDIN_FILENO) == -1) {
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to output pipe
        if (dup2(output_pipe_[1], STDOUT_FILENO) == -1) {
            exit(EXIT_FAILURE);
        }

        // Prepare environment
        std::vector<std::string> env_strings = prepareEnvironment(env_vars);
        std::vector<const char*> env_array;
        for (const auto& str : env_strings) {
            env_array.push_back(str.c_str());
        }
        env_array.push_back(nullptr);

        // Prepare arguments
        const char* args[] = {
            interpreter.c_str(),
            script_path.c_str(),
            nullptr
        };

        execve(interpreter.c_str(), const_cast<char* const*>(args), const_cast<char* const*>(env_array.data()));
        exit(EXIT_FAILURE);  // Only reached if execve fails
    }

    int status;
    waitpid(pid, &status, 0);
    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    // Parent process
    close(input_pipe_[0]);   // Close read end of input
    close(output_pipe_[1]);  // Close write end of output

    // Write request body to script if present
    if (!request_body.empty()) {
        write(input_pipe_[1], request_body.c_str(), request_body.length());
    }
    close(input_pipe_[1]);  // Close after writing

    // Read script output
    std::string output = readOutput();

    // Wait for child and get status

    return {exit_code, output};
}

void CGIExecutor::setupPipes()
{
    if (pipe(input_pipe_) == -1 || pipe(output_pipe_) == -1) {
        throw std::runtime_error("Pipe creation failed: " + std::string(strerror(errno)));
    }

    // Set pipes to non-blocking mode
    fcntl(output_pipe_[0], F_SETFL, O_NONBLOCK);
}

void CGIExecutor::closePipes()
{
    close(input_pipe_[0]);
    close(input_pipe_[1]);
    close(output_pipe_[0]);
    close(output_pipe_[1]);
}

std::vector<std::string> CGIExecutor::prepareEnvironment(
    const std::map<std::string, std::string>& env_vars) const
{
    std::vector<std::string> result;
    result.reserve(env_vars.size());

    for (const auto& [key, value] : env_vars) {
        result.push_back(key + "=" + value);
    }

    return result;
}

std::string CGIExecutor::readOutput()
{
    std::stringstream output;
    char buffer[4096];
    ssize_t bytes_read;

    while ((bytes_read = read(output_pipe_[0], buffer, sizeof(buffer))) > 0) {
        output.write(buffer, bytes_read);
    }

    close(output_pipe_[0]);
    return output.str();
}
/* Executes one tool without shell parsing or command-string interpolation. */

#include "process.h"
#include <cerrno>
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <thread>

namespace emul::formal {
ProcessResult run_process(
    const std::filesystem::path& executable,
    const std::vector<std::string>& arguments,
    const ProcessOptions& options) {
    const auto started = std::chrono::steady_clock::now();
    int pipes[2];
    if (pipe(pipes) != 0)
        throw std::runtime_error(std::strerror(errno));
    auto child = fork();
    if (child < 0)
        throw std::runtime_error(std::strerror(errno));
    if (child == 0) {
        setpgid(0, 0);
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (getppid() == 1)
            _exit(128 + SIGTERM);
        close(pipes[0]);
        dup2(pipes[1], STDOUT_FILENO);
        dup2(pipes[1], STDERR_FILENO);
        close(pipes[1]);
        if (!options.working_directory.empty() &&
            chdir(options.working_directory.c_str()) != 0)
            _exit(126);
        std::vector<char*> argv;
        auto executable_text = executable.string();
        argv.push_back(executable_text.data());
        for (const auto& argument : arguments)
            argv.push_back(const_cast<char*>(argument.c_str()));
        argv.push_back(nullptr);
        execv(executable.c_str(), argv.data());
        _exit(127);
    }
    close(pipes[1]);
    setpgid(child, child);
    auto flags = fcntl(pipes[0], F_GETFL, 0);
    fcntl(pipes[0], F_SETFL, flags | O_NONBLOCK);
    ProcessResult result;
    char buffer[4096];
    int status = 0;
    bool exited = false;
    while (!exited) {
        while (true) {
            auto size = read(pipes[0], buffer, sizeof(buffer));
            if (size > 0) {
                result.output.append(buffer, static_cast<size_t>(size));
                continue;
            }
            if (size < 0 && errno != EAGAIN && errno != EWOULDBLOCK &&
                errno != EINTR) {
                close(pipes[0]);
                throw std::runtime_error(std::strerror(errno));
            }
            break;
        }
        auto waited = waitpid(child, &status, WNOHANG);
        if (waited == child) {
            exited = true;
            break;
        }
        if (waited < 0 && errno != EINTR) {
            close(pipes[0]);
            throw std::runtime_error(std::strerror(errno));
        }
        const auto elapsed = std::chrono::steady_clock::now() - started;
        if (options.timeout.count() > 0 && elapsed >= options.timeout) {
            result.timed_out = true;
            kill(-child, SIGTERM);
            const auto deadline = std::chrono::steady_clock::now() +
                                  options.terminate_grace;
            while (std::chrono::steady_clock::now() < deadline) {
                if (waitpid(child, &status, WNOHANG) == child) {
                    exited = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (!exited) {
                kill(-child, SIGKILL);
                while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
                exited = true;
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    while (true) {
        auto size = read(pipes[0], buffer, sizeof(buffer));
        if (size > 0) result.output.append(buffer, static_cast<size_t>(size));
        else break;
    }
    close(pipes[0]);
    result.status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    return result;
}
}

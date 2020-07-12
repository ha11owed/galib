#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ga {

/// @brief Provides an simple crossplatform way to work with processes.
/// @note All the methods that return an status code (int) will return 0 on success
///       and a negative status code in case of error.
class Process {
  public:
    /// @brief Callback for when the process outputs a line of text.
    using OnReadLine = std::function<void(const std::string &)>;

  public:
    Process();
    Process(const std::vector<std::string> &cmd);
    Process(const std::vector<std::string> &cmd, OnReadLine onReadLine);
    Process(const std::vector<std::string> &cmd, const std::vector<std::string> &env, OnReadLine onReadLine);
    virtual ~Process();

    /// @brief Each call to write line will clear the buffer with the read lines.
    int writeLine(const std::string &line);

    /// @brief Read all the lines that are in the output stream.
    /// calling this method will also clear the buffer,
    /// meaning that the next call will not contain lines that have already been read.
    std::vector<std::string> readStdoutLines(int timeoutMs = 0);

    /// @brief Read all the lines that are in the output stream.
    /// calling this method will also clear the buffer,
    /// meaning that the next call will not contain lines that have already been read.
    std::vector<std::string> readStderrLines(int timeoutMs = 0);

    /// @brief Returns true if the process is still running.
    bool isRunning();

    /// @brief Terminate the process immediately without waiting for it to finish.
    int kill();

    /// @brief Wait for the process to finish.
    int join();

  private:
    struct Impl;
    std::shared_ptr<Impl> _impl;
};

} // namespace ga

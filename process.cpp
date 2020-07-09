#include "process.h"

#include <chrono>
#include <deque>
#include <mutex>
#include <thread>

// Anonymous namespace to avoid poluting the global namespace
// and cause any linking issues, should this implementation be used in a larger project.
namespace {
#include "subprocess.h"
}

namespace ga {

struct Process::Impl {
    subprocess_s _sp;

    std::vector<std::string> _cmdLines;
    std::vector<const char *> _cmdLinesRaw;

    std::vector<std::string> _env;
    std::vector<const char *> _envRaw;

    std::deque<std::string> _writeLines;
    std::deque<std::string> _readLines;

    std::thread _writeThread;
    std::thread _readThread;

    std::mutex _mutex;
    bool _isRunning = false;

    OnReadLine _onReadLine;

    Impl(const std::vector<std::string> &cmdLines, const std::vector<std::string> &env, OnReadLine onReadLine) {
        if (onReadLine) {
            setOnReadLine(onReadLine);
        }
        create(cmdLines, env);
    }

    Impl() {}
    ~Impl() { kill(); }

    bool create(const std::vector<std::string> &cmdLines, const std::vector<std::string> &env) {
        std::lock_guard<std::mutex> g(_mutex);
        if (_isRunning) {
            return false;
        }

        vecToRaw(cmdLines, _cmdLines, _cmdLinesRaw);
        vecToRaw(env, _env, _envRaw);

        int options = subprocess_option_combined_stdout_stderr | subprocess_option_enable_async;
        if (env.empty()) {
            options |= subprocess_option_inherit_environment;
        }
        int r = subprocess_create_ex(_cmdLinesRaw.data(), _envRaw.data(), options, &_sp);
        _isRunning = (r == 0);
        if (!_isRunning) {
            return false;
        }

        _writeThread = std::thread([this]() {
            std::string _nl = "\n";

            while (isRunning()) {
                std::string line;
                if (!popWriteLine(line)) {
                    // Do not hog the CPU
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

                line += _nl;
                if (!writeToStdin(line)) {
                    break;
                }
            }
        });

        _readThread = std::thread([this]() {
            std::string line;

            int prevCh = 0;
            while (true) {
                int ch = readFromStdout();
                if (ch == EOF) {
                    break;
                }

                if (ch == '\r') {
                    // do nothing \r\n will be treated as new line
                } else if (ch == '\n') {
                    pushReadLine(line);
                    line.clear();
                } else {
                    if (prevCh == '\r') {
                        line += '\r';
                    }
                    line += static_cast<char>(ch);
                }

                prevCh = ch;
            }

            // flush any partially read line
            if (!line.empty()) {
                pushReadLine(line);
                line.clear();
            }
        });

        return _isRunning;
    }

    static void vecToRaw(const std::vector<std::string> &in, std::vector<std::string> &out,
                         std::vector<const char *> &outRaw) {
        out = in;
        outRaw.resize(out.size() + 1);
        for (size_t i = 0; i < out.size(); i++) {
            outRaw[i] = out[i].c_str();
        }
        outRaw[out.size()] = nullptr;
    }

    bool writeToStdin(const std::string &line) {
        FILE *fin = nullptr;

        {
            std::lock_guard<std::mutex> g(_mutex);
            if (_isRunning) {
                fin = subprocess_stdin(&_sp);
                if (!fin) {
                    _isRunning = false;
                }
            }
        }

        if (!fin) {
            return false;
        }

        size_t n = fwrite(line.data(), sizeof(char), line.size(), fin);
        fflush(fin);
        if (n == line.size()) {
            return true;
        }

        setRunningFalse();
        return false;
    }

    int readFromStdout() {
        FILE *fout = nullptr;
        {
            std::lock_guard<std::mutex> g(_mutex);
            if (_isRunning) {
                fout = subprocess_stdout(&_sp);
                if (!fout) {
                    _isRunning = false;
                }
            }
        }

        if (!fout) {
            return EOF;
        }

        char buffer[1];
        size_t n = fread(buffer, sizeof(char), 1, fout);
        if (n == 1) {
            return buffer[0];
        }

        setRunningFalse();
        return EOF;
    }

    bool pushWriteLine(const std::string &in) {
        std::lock_guard<std::mutex> g(_mutex);
        if (!_isRunning) {
            return false;
        }
        _writeLines.push_back(in);
        return true;
    }

    bool popWriteLine(std::string &out) {
        std::lock_guard<std::mutex> g(_mutex);
        if (_writeLines.empty()) {
            return false;
        }

        out = _writeLines.front();
        _writeLines.pop_front();
        return true;
    }

    void popReadLines(std::vector<std::string> &out) {
        std::lock_guard<std::mutex> g(_mutex);

        out.clear();
        out.assign(_readLines.begin(), _readLines.end());
        _readLines.clear();
    }

    void pushReadLine(const std::string &line) {
        std::lock_guard<std::mutex> g(_mutex);
        _readLines.push_back(line);

        if (_onReadLine) {
            _onReadLine(line);
        }
    }

    void setOnReadLine(OnReadLine onReadLine) {
        std::lock_guard<std::mutex> g(_mutex);
        _onReadLine = onReadLine;
    }

    bool isRunning() {
        std::lock_guard<std::mutex> g(_mutex);
        return _isRunning;
    }

    bool setRunningFalse() {
        std::lock_guard<std::mutex> g(_mutex);
        if (_isRunning) {
            _isRunning = false;
            return true;
        }

        return false;
    }

    bool join(int &retCode) {
        if (!isRunning()) {
            return false;
        }

        int r = subprocess_join(&_sp, &retCode);
        dispose();

        return (r == 0);
    }

    void kill() { dispose(); }

    void dispose() {
        {
            std::lock_guard<std::mutex> g(_mutex);
            _isRunning = false;

            // Prevent double deletion of the process.
            if (_sp.stdin_file) {
                subprocess_terminate(&_sp);
                subprocess_destroy(&_sp);
                _sp.stdin_file = nullptr;
                _sp.stdout_file = nullptr;
                _sp.stderr_file = nullptr;
            }
        }

        if (_readThread.joinable()) {
            _readThread.join();
        }
        if (_writeThread.joinable()) {
            _writeThread.join();
        }
    }
};

Process::Process()
    : _impl(std::make_shared<Impl>()) {}

Process::Process(const std::vector<std::string> &cmd)
    : _impl(std::make_shared<Impl>(cmd, std::vector<std::string>(), nullptr)) {}

Process::Process(const std::vector<std::string> &cmd, OnReadLine onReadLine)
    : _impl(std::make_shared<Impl>(cmd, std::vector<std::string>(), onReadLine)) {}

Process::Process(const std::vector<std::string> &cmd, const std::vector<std::string> &env, OnReadLine onReadLine)
    : _impl(std::make_shared<Impl>(cmd, env, onReadLine)) {}

Process::~Process() {}

int Process::writeLine(const std::string &line) {
    int result = -1;
    if (_impl) {
        bool ok = _impl->pushWriteLine(line);
        if (ok) {
            result = 0;
        }
    }
    return result;
}

std::vector<std::string> Process::readLines(int timeoutMs) {
    std::vector<std::string> lines;
    if (_impl) {
        if (timeoutMs > 0 && _impl->isRunning()) {
            auto end = std::chrono::steady_clock::now();
            end += std::chrono::milliseconds(timeoutMs);

            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if (!_impl->isRunning() || std::chrono::steady_clock::now() >= end) {
                    break;
                }
            }
        }
        _impl->popReadLines(lines);
    }
    return lines;
}

bool Process::isRunning() { return _impl && _impl->isRunning(); }

int Process::kill() {
    if (!_impl) {
        return -1;
    }

    _impl->kill();
    return 0;
}

int Process::join() {
    if (!_impl) {
        return -1;
    }
    int retCode;
    _impl->join(retCode);
    return retCode;
}

} // namespace ga

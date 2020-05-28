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

    std::deque<std::string> _writeLines;
    std::deque<std::string> _readLines;

    std::thread _writeThread;
    std::thread _readThread;

    std::mutex _mutex;
    bool _isRunning = false;

    OnReadLine _onReadLine;

    Impl(const std::vector<std::string> &cmdLines, OnReadLine onReadLine) {
        setOnReadLine(onReadLine);
        create(cmdLines);
    }
    Impl(const std::vector<std::string> &cmdLines) { create(cmdLines); }
    Impl() {}
    ~Impl() { kill(); }

    bool create(const std::vector<std::string> &cmdLines) {
        std::lock_guard<std::mutex> g(_mutex);
        if (_isRunning) {
            return false;
        }

        _cmdLines = cmdLines;
        _cmdLinesRaw.resize(_cmdLines.size());
        for (size_t i = 0; i < _cmdLines.size(); i++) {
            _cmdLinesRaw[i] = _cmdLines[i].c_str();
        }

        int options = subprocess_option_inherit_environment | subprocess_option_combined_stdout_stderr;
        int r = subprocess_create(_cmdLinesRaw.data(), options, &_sp);
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
            subprocess_terminate(&_sp);
            subprocess_destroy(&_sp);
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
    : _impl(std::make_shared<Impl>(cmd)) {}

Process::Process(const std::vector<std::string> &cmd, OnReadLine onReadLine)
    : _impl(std::make_shared<Impl>(cmd, onReadLine)) {}

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
    if (_impl) {
        return -1;
    }
    int retCode;
    _impl->join(retCode);
    return 0;
}

} // namespace ga

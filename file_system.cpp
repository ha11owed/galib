#include "file_system.h"

#include <deque>
#include <fstream>
#include <memory>

namespace ga {

namespace detail {
#include "tinydir.h"

class Directory {
  public:
    Directory(const std::string &path)
        : _path(path)
        , _recursiveLevel(0)
        , _isOpen(false) {
        resetEntry(_current);
    }

    ~Directory() { close(); }

    void close() {
        resetEntry(_current);

        for (tinydir_dir &dir : _dirs) {
            tinydir_close(&dir);
        }
        _dirs.clear();
        _recursiveLevel = 0;

        _isOpen = false;
    }

    bool open() {
        if (_isOpen) {
            return false;
        }

        _dirs.emplace_back(tinydir_dir());
        _recursiveLevel = 0;
        _isOpen = (tinydir_open(&_dirs.back(), _path.c_str()) == 0);
        return _isOpen;
    }

    static void resetEntry(ChildEntry &entry) {
        entry.name = nullptr;
        entry.path = nullptr;
        entry.extension = nullptr;
        entry.type = ChildType::None;
        entry.recursiveLevel = 0;
    }

    void setFilter(const DirectorySearch &filter) {
        close();
        _filter = filter;
        open();
    }

    void setFilterRecursiveFiles() {
        DirectorySearch ds;
        ds.maxRecursionLevel = 999999;
        ds.includeFiles = true;
        ds.includeDirectories = false;
        setFilter(ds);
    }

    void setFilterRecursiveFilesAndDirectories() {
        DirectorySearch ds;
        ds.maxRecursionLevel = 999999;
        ds.includeFiles = true;
        ds.includeDirectories = true;
        setFilter(ds);
    }

    void setFilterRecursiveDirectories() {
        DirectorySearch ds;
        ds.maxRecursionLevel = 999999;
        ds.includeFiles = false;
        ds.includeDirectories = true;
        setFilter(ds);
    }

    const ChildEntry *nextChild() {
        if (!_isOpen) {
            open();
        }

        while (!_dirs.empty()) {
            resetEntry(_current);

            tinydir_dir *dir = &_dirs.back();

            if (tinydir_readfile(dir, &_currentFile) != 0) {
                tinydir_close(dir);
                _dirs.pop_back();
                _recursiveLevel--;
                continue;
            }

            tinydir_next(dir);

            if (_currentFile.is_dir) {
                _current.type = ChildType::Directory;
            } else if (_currentFile.is_reg) {
                _current.type = ChildType::File;
            } else {
                continue;
            }

            if (strcmp(_currentFile.name, ".") == 0 || strcmp(_currentFile.name, "..") == 0) {
                continue;
            }

            _current.name = _currentFile.name;
            _current.path = _currentFile.path;
            _current.extension = _currentFile.extension;

            // Add directories to the queue.
            if (_current.type == ChildType::Directory && _recursiveLevel < _filter.maxRecursionLevel) {
                _dirs.emplace_front(tinydir_dir());
                bool isChildOpen = (tinydir_open(&_dirs.front(), _current.path) == 0);
                if (isChildOpen) {
                    _recursiveLevel++;
                } else {
                    _dirs.pop_front();
                }
            }

            // Continue to the next entry if the current one does not match the filters.
            if (_current.type == ChildType::File) {
                if (!_filter.includeFiles) {
                    continue;
                }
                if (!_filter.allowedExtensions.empty()) {
                    const char *ext = getFileExtension(_current.name);
                    if (ext == nullptr) {
                        continue;
                    }
                    auto it = _filter.allowedExtensions.find(ext);
                    if (it == _filter.allowedExtensions.end()) {
                        continue;
                    }
                }
            } else if (_current.type == ChildType::Directory) {
                if (!_filter.includeDirectories) {
                    continue;
                }
            }

            // Stop and return the current result.
            break;
        }

        if (_dirs.empty()) {
            _isOpen = false;
        }

        // Result
        if (_current.type == ChildType::None) {
            return nullptr;
        }
        return &_current;
    }

  private:
    std::string _path;
    DirectorySearch _filter;
    std::deque<tinydir_dir> _dirs;
    int _recursiveLevel;

    tinydir_file _currentFile;
    ChildEntry _current;
    bool _isOpen;
};

} // namespace detail

void findInDirectory(const std::string &rootPath, OnChildEntry onChildEntry, const DirectorySearch &filter) {
    detail::Directory d(rootPath);
    d.setFilter(filter);
    while (true) {
        const ChildEntry *ce = d.nextChild();
        if (ce == nullptr) {
            break;
        }

        onChildEntry(*ce);
    }
}

bool readFile(const std::string &inFile, std::string &outBytes) {
    std::ifstream file(inFile, std::ifstream::in | std::ifstream::binary);
    if (!file) {
        return false;
    }

    file.seekg(0, std::ios::end);
    auto length = file.tellg();
    file.seekg(0, std::ios::beg);
    outBytes.resize(static_cast<size_t>(length));
    file.read(static_cast<char *>(outBytes.data()), length);
    return true;
}

const char *getFileExtension(const char *filePath) {
    if (filePath == nullptr) {
        return nullptr;
    }

    const char *ext = nullptr;
    const char *p = filePath;
    while (true) {
        char ch = *p;
        if (ch == '\0') {
            break;
        }
        if (ch == '.') {
            ext = p + 1;
        }

        p++;
    }

    if (ext != p) {
        return ext;
    }
    return nullptr;
}

std::string getFileExtension(const std::string &filePath) {
    const char *ext = getFileExtension(filePath.c_str());
    if (ext != nullptr) {
        return ext;
    }
    return "";
}

std::string getFilename(const std::string &filePath) {
    size_t n = filePath.size();
    if (n > 0) {
        for (size_t i = n - 1; i != 0; i--) {
            char ch = filePath[i];
            if (isPathSeparator(ch)) {
                return filePath.substr(i);
            }
        }
    }
    return filePath;
}

bool isStrictFilename(const std::string &value) {
    size_t n = value.size();
    if (n == 0 || n > 1024) {
        return false;
    }

    bool isDot = false;
    for (char c : value) {
        if (std::isalnum(c)) {
            isDot = false;
            continue;
        }
        if (c == '-' || c == '_') {
            isDot = false;
            continue;
        }
        if (c == '.') {
            if (isDot) {
                // consecutive dots are not allowed
                return false;
            }
            isDot = true;
            continue;
        }
        return false;
    }
    return true;
}

bool isStrictNormalizedUrl(const std::string &value) {
    size_t n = value.size();
    if (n == 0 || n > 1024) {
        return false;
    }

    bool isSpecial = false;
    for (char c : value) {
        if (std::isalnum(c)) {
            isSpecial = false;
            continue;
        }
        if (c == '-' || c == '_') {
            isSpecial = false;
            continue;
        }
        if (c == '/' || c == '\\' || c == '.') {
            if (isSpecial) {
                // Relative paths are dissallowed
                return false;
            }
            isSpecial = true;
            continue;
        }
        return false;
    }
    return true;
}

bool isPathSeparator(char value) { return (value == '/') || (value == '\\'); }

} // namespace ga

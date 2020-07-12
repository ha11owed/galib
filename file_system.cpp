#include "file_system.h"

#include <deque>
#include <fstream>
#include <memory>

#ifdef _WIN32
#include <io.h>
#define access _access_s
#ifndef F_OK
#define F_OK 0
#endif
#else
#include <unistd.h>
#endif

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

// ==== helpers ====

inline void sumBSandS(const std::string &path, int &nBS, int &nS) {
    for (char c : path) {
        if (c == '\\') {
            nBS++;
        } else if (c == '/') {
            nS++;
        }
    }
}

// ==== public ====

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
    // maybe add some compiler check, starting with c++20 the cast is not needed anymore
    file.read(const_cast<char *>(outBytes.data()), length);
    return true;
}

bool pathExists(const std::string &path) {
    bool exists = false;
    if (!path.empty()) {
        exists = (access(path.c_str(), F_OK) == 0);
    }
    return exists;
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
            if (isPathSeparator(filePath[i])) {
                return filePath.substr(i + 1);
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

std::string getParent(const std::string &path) {
    std::string parent;
    int n = path.size();
    int lastPS = n;
    for (int i = n - 1; i >= 0; i--) {
        if (isPathSeparator(path[i])) {
            int d = lastPS - i;
            if ((d < 2) || (d == 2 && path[i + 1] == '.')) {
                lastPS = i;
                continue;
            }

            parent = path.substr(0, i + 1);
            break;
        }
    }
    return parent;
}

std::string combine(const std::string &a, const std::string &b) {
    std::string r(a);

    size_t n = a.size();
    size_t m = b.size();

    if (n != 0 && m != 0) {
        bool aPS = isPathSeparator(a.back());
        bool bPS = isPathSeparator(b.front());
        if (aPS && bPS) {
            r += b.substr(1);
        } else if (!aPS && !bPS) {
            int nBS = 0, nS = 0;
            sumBSandS(a, nBS, nS);
            sumBSandS(b, nBS, nS);
            r += (nBS > 0) ? '\\' : '/';
            r += b;
        } else {
            r += b;
        }
    } else {
        r += b;
    }

    return r;
}

std::vector<std::string> splitPath(const std::string &path) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = 0;

    size_t i = 0;
    size_t n = path.size();

    for (; i < n; i++) {
        if (isPathSeparator(path[i])) {
            start = end = (i + 1);
        } else {
            break;
        }
    }

    for (; i < n; i++) {
        if (isPathSeparator(path[i])) {
            end = i;
            if (start < end) {
                parts.push_back(path.substr(start, end - start));

                size_t j = i + 1;
                for (; j < n; j++) {
                    if (!isPathSeparator(path[j])) {
                        break;
                    }
                }

                start = j;
                i = (j - 1);
            }
        }
    }

    if (start > end && start < n - 1) {
        parts.push_back(path.substr(start));
    }
    return parts;
}

bool getRelativePath(const std::string &fromDirPath, const std::string &toDirPath, std::string &out) {
    std::string result;
    bool ok = false;
    bool isFromAbsolute = isAbsolutePath(fromDirPath);
    bool isToAbsolute = isAbsolutePath(toDirPath);
    if (isFromAbsolute && isToAbsolute) {
        int nBS = 0;
        int nS = 0;
        sumBSandS(fromDirPath, nBS, nS);
        sumBSandS(toDirPath, nBS, nS);

        if ((nBS == 0 && nS != 0) || (nBS != 0 && nS == 0)) {
            const char separator = (nBS > 0) ? '\\' : '/';
            std::vector<std::string> fromParts = splitPath(fromDirPath);
            std::vector<std::string> toParts = splitPath(toDirPath);
            size_t commonLength = 0;
            size_t l = std::min(fromParts.size(), toParts.size());
            for (size_t i = 0; i < l; i++) {
                if (fromParts[i] == toParts[i]) {
                    commonLength++;
                } else {
                    break;
                }
            }

            for (size_t i = commonLength; i < fromParts.size(); i++) {
                result += "..";
                result += separator;
            }

            size_t n = toParts.size();
            if (n > 0) {
                n--;
                for (size_t i = commonLength; i < n; i++) {
                    result += toParts[i];
                    result += separator;
                }

                if (commonLength <= n && toDirPath.size() > 0) {
                    result += toParts[n];
                    if (toDirPath.back() == separator) {
                        result += separator;
                    }
                }
            }

            ok = true;
        }
    }

    out = result;
    return ok;
}

bool getSimplePath(const std::string &path, std::string &out) {
    bool ok = false;
    std::string result;

    int nBS = 0;
    int nS = 0;
    sumBSandS(path, nBS, nS);
    if ((nBS == 0 && nS != 0) || (nBS != 0 && nS == 0)) {
        const char separator = (nBS > 0) ? '\\' : '/';
        const bool isAbs = isAbsolutePath(path);
        const size_t nFixed = isAbs && (path[0] != separator) ? 1 : 0;
        std::vector<std::string> parts = splitPath(path);
        std::deque<size_t> normalIndexes;

        for (size_t i = 0; i < parts.size(); i++) {
            if (parts[i] == ".") {
                parts.erase(parts.begin() + i);
                i--;
            } else if (parts[i] == "..") {
                if (normalIndexes.size() > nFixed) {
                    parts.erase(parts.begin() + i);
                    parts.erase(parts.begin() + normalIndexes.back());
                    normalIndexes.pop_back();
                    i -= 2;
                } else if (isAbs) {
                    parts.erase(parts.begin() + i);
                    i--;
                }
            } else {
                normalIndexes.push_back(i);
            }
        }

        size_t n = parts.size();
        if (n > 0) {
            if (isAbs && nFixed == 0) {
                result += separator;
            }
            result += parts[0];

            for (size_t i = 1; i < n; i++) {
                result += separator;
                result += parts[i];
            }
        }

        out = result;
        ok = true;
    }
    return ok;
}

bool isAbsolutePath(const std::string &path) {
    size_t n = path.size();
    if (n == 0) {
        return false;
    }
    if (path[0] == '/') {
        return true;
    }
    if (n >= 3 && path[1] == ':' && path[2] == '\\') {
        return true;
    }
    return false;
}

bool isPathSeparator(char value) { return (value == '/') || (value == '\\'); }

} // namespace ga

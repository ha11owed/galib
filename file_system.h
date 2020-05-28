#pragma once

#include <functional>
#include <set>
#include <string>
#include <vector>

namespace ga {

enum ChildType {
    None,
    Directory,
    File,
};

struct ChildEntry {
    const char *path;
    const char *name;
    const char *extension;
    ChildType type;
    int recursiveLevel;
};

struct DirectorySearch {
    int maxRecursionLevel = 5;

    bool includeDirectories = false;
    bool includeFiles = true;

    /// @brief If the set is empty all extensions are allowed.
    /// Otherwise only the ones in the set (case sensitive).
    std::set<std::string> allowedExtensions;
};

using OnChildEntry = std::function<void(const ChildEntry &)>;

void findInDirectory(const std::string &rootPath, OnChildEntry onChildEntry,
                     const DirectorySearch &filter = DirectorySearch());

bool readFile(const std::string &inFile, std::string &outBytes);

const char *getFileExtension(const char *filePath);

std::string getFileExtension(const std::string &filePath);

std::string getFilename(const std::string &filePath);

bool isPathSeparator(char value);

} // namespace ga

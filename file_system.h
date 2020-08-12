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

/// @brief write the bytes to a file in an atomic way (by writing to a temp file and doing a rename).
bool writeFile(const std::string &filePath, const std::string &inBytes);

/// @brief returs true if the path exists (but does not check for read or write permissions on the file or dir).
bool pathExists(const std::string &path);

const char *getFileExtension(const char *filePath);

std::string getFileExtension(const std::string &filePath);

std::string getFilename(const std::string &filePath);

std::string getParent(const std::string &path);

std::string combine(const std::string &a, const std::string &b);

std::vector<std::string> splitPath(const std::string &path);

bool getRelativePath(const std::string &fromDirPath, const std::string &toDirPath, std::string &out);

bool getSimplePath(const std::string &path, std::string &out);

bool isAbsolutePath(const std::string &path);

bool isPathSeparator(char value);

} // namespace ga

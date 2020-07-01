#include "CMaker.h"

#include "file_system.h"
#include "json.hpp"
#include "process.h"
#include "tinyxml2.h"
#include "whereami.h"
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>

namespace ga {

using XmlElemPtr = tinyxml2::XMLElement *;
using XmlElemParentPair = std::pair<XmlElemPtr, XmlElemPtr>;

struct CMaker::Impl {
    struct CMakeInput {
        std::string configFilePath;
        std::set<std::string> configFileNames;

        std::string pwd;
        std::string projectDir;
        std::string buildDir;
        std::string sdkDir;
        std::string virtualFolderPrefix;
        std::string oldSdkPrefix;
        std::string oldVirtualFolderPrefix;
        std::vector<std::string> extraAddDirectory;
        std::set<std::string> gccClangFixes;
        bool overrideFiles = true;
    };

    struct CMakeOutput {
        std::map<std::string, std::string> originalFileContent;
        std::map<std::string, std::string> modifiedFileContent;
        std::set<std::string> errors;
    };

    CMakeInput in;
    CMakeOutput out;

    void split(const std::string &input, const std::string &separator, std::vector<std::string> &parts) {
        size_t start = 0;
        size_t end = input.find(separator);
        while (end != std::string::npos) {
            parts.push_back(input.substr(start, end - start));
            start = end + separator.length();
            end = input.find(separator, start);
        }
        if (start < input.size() - 1) {
            parts.push_back(input.substr(start));
        }
    }

    std::string join(const std::vector<std::string> &content, const std::string &separator) {
        std::stringstream ss;
        size_t n = content.size();
        if (n > 0) {
            ss << content[0];
            for (size_t i = 1; i < n; i++) {
                ss << separator;
                ss << content[i];
            }
        }
        return ss.str();
    }

    void cleanPathSeparators(std::string &path, char separator) {
        for (char &c : path) {
            if (ga::isPathSeparator(c)) {
                c = separator;
            }
        }
    }

    bool getAttribute(XmlElemPtr elem, const char *attrName, std::string &outValue) {
        bool ok = false;
        if (elem != nullptr) {
            const char *value = elem->Attribute(attrName);
            if (value && strlen(value) > 0) {
                outValue = value;
                ok = true;
            }
        }
        return ok;
    }

    void addPrefix(XmlElemPtr elem, const char *attrName, const std::string &prefix) {
        std::string value;
        if (!getAttribute(elem, attrName, value)) {
            return;
        }

        size_t idx = value.find("/usr/");
        if (idx == std::string::npos) {
            return;
        }

        value = prefix + value.substr(idx);
        ga::getSimplePath(value, value);
        elem->SetAttribute(attrName, value.c_str());
    }

    void addPrefixToVirtualFolder(XmlElemPtr elem, const char *attrName) {
        std::string value;
        if (!getAttribute(elem, attrName, value)) {
            return;
        }

        const std::string DELIMIT = ";";
        std::vector<std::string> parts;
        split(value, DELIMIT, parts);

        const std::string CMakeFiles_BS = "CMake Files\\";
        const std::string BS_USR = "\\usr";
        size_t n = parts.size();
        for (size_t i = 0; i < n; i++) {
            std::string part = parts[i];

            // part must begin with "CMake Files\" otherwise continue (maybe error?)
            if (part.find(CMakeFiles_BS) != 0) {
                continue;
            }

            std::string virtualPath(part.substr(CMakeFiles_BS.size()));
            if (virtualPath.find(in.oldVirtualFolderPrefix) == 0) {
                // replace the old virtual folder prefix with the new one
                virtualPath = virtualPath.substr(in.oldVirtualFolderPrefix.size());
                virtualPath = in.virtualFolderPrefix + virtualPath;
            } else {
                // check if the path is inside the source dir
                std::string simpleVirtualPath = combine(in.buildDir, virtualPath);
                cleanPathSeparators(simpleVirtualPath, '\\');
                ga::getSimplePath(simpleVirtualPath, simpleVirtualPath);
                if (simpleVirtualPath.find("..") == 0) {
                    // virtual must be put in the SDK
                    simpleVirtualPath = combine(in.virtualFolderPrefix, simpleVirtualPath);
                    virtualPath = simpleVirtualPath;
                }
            }

            parts[i] = CMakeFiles_BS + virtualPath;
        }

        if (n > 0) {
            value = parts[0];
            for (size_t i = 1; i < n; i++) {
                value += ";";
                value += parts[i];
            }
        }

        elem->SetAttribute(attrName, value.c_str());
    }

    bool readNote(XmlElemPtr elem) {
        std::string showNotes;
        bool ok = false;
        for (;;) {
            if (!getAttribute(elem, "show_notes", showNotes)) {
                break;
            }
            XmlElemPtr child = elem->FirstChildElement();
            if (!child) {
                break;
            }

            std::string data = child->GetText();
            auto itS = data.find("<![CDATA[");
            auto itE = data.find("]]>");
            // strip the <![CDATA[]]> to keep the content only
            if (itS == 0 && itE == data.size() - 4) {
                data = data.substr(9, data.size() - 12);
            }

            std::vector<std::string> content;
            split(data, "\n", content);
            if (content.size() >= 2) {
                in.oldSdkPrefix = content[0];
                in.oldVirtualFolderPrefix = content[1];
            }

            std::vector<std::string> newContent;
            newContent.push_back(in.sdkDir);
            newContent.push_back(in.virtualFolderPrefix);
            std::string sNewContent = join(newContent, "\n");
            child->SetText(sNewContent.c_str());

            ok = true;
            break;
        }
        return ok;
    }

    XmlElemPtr createNote(XmlElemPtr elem) {
        XmlElemPtr option = elem->InsertNewChildElement("Option");
        option->SetAttribute("show_notes", "0");
        XmlElemPtr notes = option->InsertNewChildElement("notes");

        tinyxml2::XMLText *text = notes->InsertNewText("");
        text->SetCData(true);

        std::vector<std::string> newContent;
        newContent.push_back(in.sdkDir);
        newContent.push_back(in.virtualFolderPrefix);
        std::string sContent = join(newContent, "\n");
        text->SetValue(sContent.c_str());

        elem->InsertFirstChild(option);
        return option;
    }

    void enqueueWithSiblings(XmlElemPtr elem, XmlElemPtr parent, std::deque<XmlElemParentPair> &q) {
        if (elem == nullptr) {
            return;
        }

        q.push_back(std::make_pair(elem, parent));
        while ((elem = elem->NextSiblingElement()) != nullptr) {
            q.push_back(std::make_pair(elem, parent));
        }
    }

    std::string getModuleDir() const {
        char buffer[1024 * 64];
        int dirLen;
        int n = wai_getModulePath(buffer, sizeof(buffer), &dirLen);

        std::string dir;
        if (n > 0 && dirLen > 0) {
            dir.append(buffer, dirLen);
        }
        return dir;
    }

    int exec(const std::vector<std::string> &args, const std::string &pwd) {
        bool patchCbp = preparePatchCBPs(args, pwd);
        if (patchCbp) {
            patchCBPs();
        }

        return execCMake(args);
    }

    /// @brief gather the parameters for patching the .cbp files to use a SDK.
    /// @return true if the CBPs should be patched and the parameters have been gathered.
    bool preparePatchCBPs(const std::vector<std::string> &args, const std::string &pwd) {
        bool patchCbp = false;

        if (args.size() >= 2) {
            patchCbp = pathExists(args[1]);
        }

        if (!patchCbp) {
            return false;
        }

        in = CMakeInput();
        in.configFileNames.insert("cmaker.json");
        in.projectDir = args[1];
        in.pwd = pwd;
        in.buildDir = pwd;
        if (pwd.empty()) {
            in.pwd = getModuleDir();
            in.buildDir = in.pwd;
        }

        DirectorySearch ds;
        ds.maxRecursionLevel = 0;
        ds.includeFiles = true;
        ds.includeDirectories = false;

        std::set<std::string> configFilePaths;

        std::string searchDir = in.projectDir;
        for (int i = 0; i < 3; i++) {
            findInDirectory(
                searchDir,
                [this, &configFilePaths](const ChildEntry &entry) {
                    if (in.configFileNames.find(entry.name) != in.configFileNames.end()) {
                        configFilePaths.insert(entry.path);
                    }
                },
                ds);

            if (!configFilePaths.empty()) {
                break;
            }
            searchDir = getParent(searchDir);
        }

        for (const std::string &configFilePath : configFilePaths) {
            std::ifstream fin(configFilePath);
            if (!fin) {
                out.errors.insert(configFilePath + " could not be read");
                continue;
            }

            nlohmann::json jObj;
            fin >> jObj;

            nlohmann::json jProjects = jObj["projects"];
            if (!jProjects.is_array()) {
                out.errors.insert(configFilePath + " does not contain the projects section");
                continue;
            }

            bool hasProject = false;
            nlohmann::json jProject;
            for (size_t i = 0; i < jProjects.size(); i++) {
                jProject = jProjects.at(i);
                if (jProject["path"] == in.projectDir) {
                    hasProject = true;
                    break;
                }
            }

            if (!hasProject) {
                out.errors.insert(configFilePath + " does not contain the project " + in.projectDir);
                break;
            }

            in.sdkDir = jProject["sdkPath"];
            for (const auto &kv : jProject["gccClangFixes"].items()) {
                in.gccClangFixes.insert(kv.value().get<std::string>());
            }
            for (const auto &kv : jProject["extraAddDirectory"].items()) {
                in.extraAddDirectory.push_back(kv.value().get<std::string>());
            }
            in.overrideFiles = jProject.value("overrideFiles", true);

            in.configFilePath = configFilePath;
            break;
        }

        return !in.sdkDir.empty();
    }

    /// @brief patch the .cbp files from the build directory.
    void patchCBPs() {
        std::vector<std::string> cbpFilePaths;

        // Gather all CBP files
        {
            ga::DirectorySearch ds;
            ds.includeFiles = true;
            ds.includeDirectories = false;
            ds.maxRecursionLevel = 0;
            ds.allowedExtensions.insert("cbp");
            ga::findInDirectory(
                in.buildDir, [&cbpFilePaths](const ga::ChildEntry &entry) { cbpFilePaths.push_back(entry.path); }, ds);
        }

        // Process all CBP files
        for (const std::string &filePath : cbpFilePaths) {
            tinyxml2::XMLDocument inXml;
            tinyxml2::XMLError error = inXml.LoadFile(filePath.c_str());
            if (error != tinyxml2::XML_SUCCESS) {
                out.errors.insert(filePath + " cannot be loaded");
                break;
            }

            // will contain the relative path from the directory of the filePath to the sdk folder
            std::string virtualFolderPrefix;
            std::string dir = ga::getParent(filePath);
            if (!ga::getRelativePath(dir, in.sdkDir, virtualFolderPrefix)) {
                out.errors.insert("cannot get relative path: " + dir + " => " + in.sdkDir);
                break;
            }
            for (char &c : virtualFolderPrefix) {
                if (c == '/') {
                    c = '\\';
                }
            }
            in.virtualFolderPrefix = virtualFolderPrefix;

            bool hasNotes = false;

            tinyxml2::XMLPrinter printerIn;
            inXml.Print(&printerIn);
            std::string original(printerIn.CStr(), printerIn.CStrSize());

            std::deque<XmlElemParentPair> q;
            enqueueWithSiblings(inXml.FirstChildElement(), nullptr, q);

            while (!q.empty()) {
                XmlElemParentPair currPair = q.front();
                XmlElemPtr curr = currPair.first;
                XmlElemPtr parentElem = currPair.second;
                q.pop_front();

                std::string parent;
                if (parentElem != nullptr && parentElem->Name() != nullptr) {
                    parent = parentElem->Name();
                }

                const char *name_cstr = curr->Name();
                if (name_cstr == nullptr) {
                    continue;
                }

                std::string name(name_cstr);

                if (parent == "Compiler" && name == "Add") {
                    addPrefix(curr, "directory", in.sdkDir);
                } else if (name == "Unit") {
                    addPrefix(curr, "filename", in.sdkDir);
                } else if (parent == "MakeCommands") {
                    static std::set<std::string> makeCommandChildren = {"Build", "CompileFile", "Clean", "DistClean"};
                    if (makeCommandChildren.find(name) != makeCommandChildren.end()) {
                        addPrefix(curr, "command", in.sdkDir);
                    }
                } else if (parent == "Unit" && name == "Option") {
                    addPrefixToVirtualFolder(curr, "virtualFolder");
                } else if (parent == "Project" && name == "Option") {
                    // In the Project section there will be multiple Option children.
                    if (readNote(curr)) {
                        hasNotes = true;
                    } else {
                        if (!hasNotes) {
                            createNote(parentElem);
                            hasNotes = true;
                        }
                        addPrefixToVirtualFolder(curr, "virtualFolders");
                    }
                }

                enqueueWithSiblings(curr->FirstChildElement(), curr, q);

                if (name == "Compiler") {
                    for (const std::string &addDir : in.extraAddDirectory) {
                        XmlElemPtr elem = curr->InsertNewChildElement("Add");
                        elem->SetAttribute("directory", addDir.c_str());
                        addPrefix(elem, "directory", in.sdkDir);
                    }

                    // Add the options at the beginning of the Compiler section
                    for (const std::string &addOption : in.extraAddDirectory) {
                        XmlElemPtr elem = curr->InsertNewChildElement("Add");
                        elem->SetAttribute("option", addOption.c_str());
                        curr->InsertFirstChild(elem);
                    }
                }
            }

            tinyxml2::XMLPrinter printerOut;
            inXml.Print(&printerOut);
            std::string modified(printerOut.CStr(), printerOut.CStrSize());

            out.originalFileContent[filePath] = original;
            if (original != modified) {
                out.modifiedFileContent[filePath] = modified;
            }

            if (in.overrideFiles) {
                std::string outFile = filePath + ".txt";
                inXml.SaveFile(outFile.c_str());
            }

            if (original != modified) {
                std::cout << filePath << " was patched" << std::endl;
            } else {
                std::cout << filePath << " was already patched" << std::endl;
            }
        }

        std::cout << "SDK:    " << in.sdkDir << std::endl;
        std::cout << "config: " << in.configFilePath << std::endl;
        std::cout << "Finished patching..." << std::endl;
    }

    int execCMake(const std::vector<std::string> &args) {
        if (args.empty()) {
            return -1;
        }

        std::vector<std::string> cmd;
        std::vector<std::string> lines;
        if (in.pwd.empty()) {
            cmd = args;
            cmd[0] = "cmake";
        } else {
            cmd = {"/bin/bash"};

            std::string cdCmd = "cd " + in.pwd;
            std::string cmakeCmd = "cmake";
            for (int i = 1; i < args.size(); i++) {
                cmakeCmd += " ";
                cmakeCmd += args[i];
            }

            lines.push_back(cdCmd);
            lines.push_back(cmakeCmd);
        }

        std::cout << "cmake..." << std::endl;

        ga::Process p(cmd);
        for (const std::string &line : lines) {
            std::cout << line << std::endl;
            p.writeLine(line);
            for (const std::string &line : p.readLines(5)) {
                std::cout << line << std::endl;
            }
        }

        int result = p.join();
        for (const std::string &line : p.readLines(0)) {
            std::cout << line << std::endl;
        }
        return result;
    }
};

CMaker::CMaker()
    : _impl(std::make_shared<Impl>()) {}

CMaker::~CMaker() {}

int CMaker::exec(const std::vector<std::string> &args, const std::string &pwd) {
    int r = -1;
    if (_impl) {
        r = _impl->exec(args, pwd);
    }
    return r;
}

} // namespace ga

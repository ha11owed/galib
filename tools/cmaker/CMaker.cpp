#include "CMaker.h"

#include "file_system.h"
#include "json.hpp"
#include "loguru.hpp"
#include "process.h"
#include "tinyxml2.h"
#include "whereami.h"
#include <deque>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef CMAKER_WITH_UNIT_TESTS
#include "gtest/gtest.h"
#endif

namespace gatools {

using XmlElemPtr = tinyxml2::XMLElement *;
using XmlElemParentPair = std::pair<XmlElemPtr, XmlElemPtr>;

struct CMaker::Impl {
    using WriteFileCb = std::function<void(const std::string &filePath, const std::string &content)>;

    struct CMakeInput {
        std::string configFilePath;

        std::string home;
        std::string projectDir;
        std::string buildDir;
        std::string sdkDir;
        std::string virtualFolderPrefix;
        std::string oldSdkPrefix;
        std::string oldVirtualFolderPrefix;
        std::vector<std::string> extraAddDirectory;
        std::set<std::string> gccClangFixes;
        bool overrideFiles = true;
        bool outputToStdout = true;
        std::set<std::string> cmdEnvironment;
        std::map<std::string, std::string> cmdReplacement;

        void log() const {
            LOG_F(INFO, "CMakeInput {");
            LOG_F(INFO, "  configFilePath: %s", configFilePath.c_str());
            LOG_F(INFO, "  projectDir: %s", projectDir.c_str());
            LOG_F(INFO, "  buildDir:   %s", buildDir.c_str());
            LOG_F(INFO, "  sdkDir:     %s", sdkDir.c_str());
            LOG_F(INFO, "  virtualFolderPrefix: %s", virtualFolderPrefix.c_str());
            LOG_IF_F(INFO, !oldSdkPrefix.empty(), "  oldSdkPrefix: %s", oldSdkPrefix.c_str());
            LOG_IF_F(INFO, !oldVirtualFolderPrefix.empty(), "  oldVirtualFolderPrefix: %s",
                     oldVirtualFolderPrefix.c_str());
            LOG_F(INFO, "}");
        }
    };

    enum class PatchResult { Changed, Unchanged, Error };

    const char *asString(PatchResult value) {
        switch (value) {
        case PatchResult::Changed:
            return "Changed";
        case PatchResult::Unchanged:
            return "Unchanged";
        case PatchResult::Error:
            return "Error";
        }
        return "Unknown PatchResult";
    }

    CMakeInput in;
    WriteFileCb writeFileCb;

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

    void replaceAll(const std::string &from, const std::string &to, std::string &inOutStr) {
        if (from.empty()) {
            return;
        }
        size_t start_pos = 0;
        while ((start_pos = inOutStr.find(from, start_pos)) != std::string::npos) {
            inOutStr.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
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

    void addPrefixToVirtualFolder(std::string &value) {
        const std::string DELIMIT = ";";
        std::vector<std::string> parts;
        split(value, DELIMIT, parts);

        const std::string CMakeFiles_BS = "CMake Files\\";
        size_t n = parts.size();
        for (size_t i = 0; i < n; i++) {
            std::string part = parts[i];

            // part must begin with "CMake Files\" otherwise continue (maybe error?)
            if (part.find(CMakeFiles_BS) != 0) {
                continue;
            }

            std::string virtualPath(part.substr(CMakeFiles_BS.size()));
            if (in.oldVirtualFolderPrefix.size() > 0 && virtualPath.find(in.oldVirtualFolderPrefix) == 0) {
                // replace the old virtual folder prefix with the new one
                virtualPath = virtualPath.substr(in.oldVirtualFolderPrefix.size());
                virtualPath = in.virtualFolderPrefix + virtualPath;
            } else {
                // check if the path is inside the source dir
                std::string simpleVirtualPath = ga::combine(in.buildDir, part);
                cleanPathSeparators(simpleVirtualPath, '/');
                ga::getSimplePath(simpleVirtualPath, simpleVirtualPath);
                if (simpleVirtualPath.find("/usr/") == 0) {
                    // virtual must be put in the SDK
                    simpleVirtualPath = ga::combine(in.virtualFolderPrefix, simpleVirtualPath);
                    cleanPathSeparators(simpleVirtualPath, '\\');
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
    }

    void addPrefixToVirtualFolder(XmlElemPtr elem, const char *attrName) {
        std::string value;
        if (!getAttribute(elem, attrName, value)) {
            return;
        }

        addPrefixToVirtualFolder(value);

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

    /// @brief get a vector of the config files found in the order of search priority
    std::vector<std::string> getConfigFilePaths() const {
        std::vector<std::string> configFilePaths;
        std::set<std::string> configFilePathSet;

        std::vector<std::pair<std::string, int>> searches;
        searches.emplace_back(std::make_pair(in.home, 1));
        searches.emplace_back(std::make_pair(in.projectDir, 3));
        searches.emplace_back(std::make_pair(in.buildDir, 3));

        for (const auto &kv : searches) {
            std::string searchDir = kv.first;
            if (searchDir.empty()) {
                continue;
            }

            for (int i = 0; i < kv.second; i++) {
                std::string filePath = ga::combine(searchDir, "cmaker.json");

                auto it = configFilePathSet.find(filePath);
                if (it == configFilePathSet.end()) {
                    if (ga::pathExists(filePath)) {
                        configFilePaths.push_back(filePath);
                    }
                }

                searchDir = ga::getParent(searchDir);
            }
        }

        return configFilePaths;
    }

    std::string getModuleDir() const {
        char buffer[1024 * 64];
        int dirLen;
        int n = wai_getModulePath(buffer, sizeof(buffer), &dirLen);

        std::string dir;
        if (n > 0 && dirLen > 0) {
            dir.append(buffer, static_cast<size_t>(dirLen));
        }
        return dir;
    }

    int exec(const std::vector<std::string> &args, const std::vector<std::string> &env, const std::string &home,
             const std::string &pwd) {
        // Log the input parameters
        LOG_F(INFO, "exec");
        for (size_t i = 0; i < args.size(); i++) {
            LOG_F(INFO, "exec arg[%lu]: %s", i, args[i].c_str());
        }
        for (size_t i = 0; i < env.size(); i++) {
            LOG_F(INFO, "exec env[%lu]: %s", i, env[i].c_str());
        }
        LOG_F(INFO, "exec pwd: %s", pwd.c_str());

        in = CMakeInput();
        bool patchCbp = canPatchCBP(args);
        LOG_F(INFO, "exec patchCbp: %d", patchCbp);
        bool hasConfig = readConfiguration(home, pwd);
        LOG_F(INFO, "exec hasConfig: %d", hasConfig);

        int retCode = -1;
        if (args.size() == 0) {
            LOG_F(ERROR, "exec empty args");
        } else {
            auto replIt = in.cmdReplacement.find(args[0]);
            if (replIt == in.cmdReplacement.end()) {
                replIt = in.cmdReplacement.find(ga::getFilename(args[0]));
            }
            if (replIt != in.cmdReplacement.end()) {
                LOG_F(INFO, "exec cmdReplacement: %s", replIt->second.c_str());
                std::vector<std::string> argsR(args);
                argsR[0] = replIt->second;

                std::vector<std::string> envR(env);
                for (const std::string &v : in.cmdEnvironment) {
                    LOG_F(INFO, "exec env: %s", v.c_str());
                    envR.push_back(v);
                }

                retCode = execOriginalCommand(argsR, envR);
            } else {
                LOG_F(ERROR, "exec cmdReplacement for: %s does not exist", args[0].c_str());
            }
        }
        if (patchCbp) {
            patchCBPs();
        }

        // Log the return code and return
        LOG_F(INFO, "exec retCode: %d", retCode);
        return retCode;
    }

    void writeCbp(WriteFileCb cb) { writeFileCb = cb; }

    /// @brief gather the parameters for patching the .cbp files to use a SDK.
    /// @return true if the CBPs should be patched and the parameters have been gathered.
    bool canPatchCBP(const std::vector<std::string> &args) {
        LOG_F(INFO, "canPatchCBP");

        bool patchCbp = false;

        if (args.size() >= 2) {
            patchCbp = (ga::getFilename(args[0]).find("make") != std::string::npos) && ga::pathExists(args[1]);
            if (patchCbp) {
                in.projectDir = args[1];
                LOG_F(INFO, "in.projectDir: %s. patchCbp: %d", in.projectDir.c_str(), patchCbp);
            } else {
                LOG_F(INFO, "args[1]: %s. patchCbp: %d", args[1].c_str(), patchCbp);
            }
        } else {
            LOG_F(INFO, "args.size() = %lu. patchCbp: %d", args.size(), patchCbp);
        }

        return patchCbp;
    }

    /// @brief gather the parameters for patching the .cbp files to use a SDK.
    /// @return true if the CBPs should be patched and the parameters have been gathered.
    bool readConfiguration(const std::string &home, const std::string &pwd) {
        LOG_F(INFO, "preparePatchCBPs");

        in.home = home;
        in.buildDir = pwd;

        ga::DirectorySearch ds;
        ds.maxRecursionLevel = 0;
        ds.includeFiles = true;
        ds.includeDirectories = false;

        std::vector<std::string> configFilePaths = getConfigFilePaths();
        for (const std::string &configFilePath : configFilePaths) {
            LOG_F(INFO, "configFilePath: %s", configFilePath.c_str());
        }

        for (const std::string &configFilePath : configFilePaths) {
            std::ifstream fin(configFilePath);
            if (!fin) {
                LOG_F(ERROR, "%s could not be read", configFilePath.c_str());
                continue;
            }

            nlohmann::json jObj;
            fin >> jObj;

            nlohmann::json jProjects = jObj["projects"];
            if (!jProjects.is_array()) {
                LOG_F(ERROR, "%s does not contain the projects section", configFilePath.c_str());
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
                for (size_t i = 0; i < jProjects.size(); i++) {
                    jProject = jProjects.at(i);
                    if (jProject["path"] == "*") {
                        hasProject = true;
                        break;
                    }
                }
            }

            if (!hasProject) {
                LOG_F(ERROR, "%s does not contain the project %s", configFilePath.c_str(), in.projectDir.c_str());
                break;
            }

            in.sdkDir = jProject["sdkPath"];

            in.gccClangFixes.clear();
            for (const auto &kv : jProject["gccClangFixes"].items()) {
                in.gccClangFixes.insert(kv.value().get<std::string>());
            }

            in.extraAddDirectory.clear();
            for (const auto &kv : jProject["extraAddDirectory"].items()) {
                in.extraAddDirectory.push_back(kv.value().get<std::string>());
            }

            // Output
            in.overrideFiles = jProject.value("overrideFiles", true);
            in.outputToStdout = jProject.value("outputToStdout", true);

            // CMD
            in.cmdEnvironment.clear();
            for (const auto &v : jProject["cmdEnvironment"]) {
                in.cmdEnvironment.insert(v.get<std::string>());
            }

            in.cmdReplacement.clear();
            std::string sdkDirWithS(in.sdkDir + "/");
            for (const auto &kv : jProject["cmdReplacement"].items()) {
                std::string value = kv.value();
                replaceAll("${sdkPath}", sdkDirWithS, value);
                ga::getSimplePath(value, value);

                in.cmdReplacement[kv.key()] = value;
                std::string smallKey = ga::getFilename(kv.key());
                if (in.cmdReplacement.find(smallKey) == in.cmdReplacement.end()) {
                    in.cmdReplacement[smallKey] = value;
                }
            }

            in.configFilePath = configFilePath;
            break;
        }

        return !in.sdkDir.empty();
    }

    /// @brief patch the .cbp at the filePath.
    PatchResult patchCBP(const std::string &filePath, tinyxml2::XMLDocument &inXml) {
        PatchResult patchResult = PatchResult::Error;

        // will contain the relative path from the directory of the filePath to the sdk folder
        std::string virtualFolderPrefix;
        std::string dir = ga::getParent(filePath);
        if (!ga::getRelativePath(dir, in.sdkDir, virtualFolderPrefix)) {
            LOG_F(ERROR, "cannot get relative path: %s => %s", dir.c_str(), in.sdkDir.c_str());
            return patchResult;
        }

        cleanPathSeparators(virtualFolderPrefix, '\\');
        in.virtualFolderPrefix = "..\\" + virtualFolderPrefix;
        in.log();

        bool hasNotes = false;

        tinyxml2::XMLPrinter printerIn;
        inXml.Print(&printerIn);
        std::string original(printerIn.CStr());

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
                    /// @todo maybe replace the commands with our own set?
                    // addPrefix(curr, "command", in.sdkDir);
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
        std::string modified(printerOut.CStr());

        bool isModified = (original != modified);
        if (isModified) {
            patchResult = PatchResult::Changed;
            if (writeFileCb) {
                writeFileCb(filePath, modified);
            }
        } else {
            patchResult = PatchResult::Unchanged;
        }

        return patchResult;
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
                LOG_F(ERROR, "%s cannot be loaded", filePath.c_str());
                continue;
            }

            PatchResult patchResult = patchCBP(filePath, inXml);

            LOG_F(INFO, "patchCBPs filePath: %s PatchResult: %s", in.sdkDir.c_str(), asString(patchResult));
            if (in.outputToStdout) {
                std::cout << filePath << " PatchResult: " << asString(patchResult) << std::endl;
            }

            switch (patchResult) {
            case PatchResult::Changed:
                if (in.overrideFiles && !writeFileCb) {
                    std::string outFile = filePath + ".tmp";
                    std::string bakFile = filePath + ".bak";
                    inXml.SaveFile(outFile.c_str());
                    if (!ga::pathExists(bakFile)) {
                        std::rename(filePath.c_str(), bakFile.c_str());
                    }
                    std::rename(outFile.c_str(), filePath.c_str());
                }
                break;
            case PatchResult::Unchanged:
                break;
            case PatchResult::Error:
                break;
            }
        }

        LOG_F(INFO, "patchCBPs SDK:    %s", in.sdkDir.c_str());
        LOG_F(INFO, "patchCBPs config: %s", in.configFilePath.c_str());
        if (in.outputToStdout) {
            std::cout << "SDK:    " << in.sdkDir << std::endl;
            std::cout << "config: " << in.configFilePath << std::endl;
            std::cout << "Finished patching..." << std::endl;
        }
    }

    int execOriginalCommand(const std::vector<std::string> &cmd, const std::vector<std::string> &env) {
        if (cmd.empty()) {
            LOG_F(ERROR, "cmd is empty");
            return -1;
        }

        LOG_F(INFO, "%s", cmd[0].c_str());
        ga::Process p(cmd, env, nullptr);

        int result = p.join();
        LOG_F(INFO, "join: %d", result);

        for (const std::string &line : p.readStderrLines(0)) {
            LOG_F(INFO, "err: %s", line.c_str());
            std::cerr << line << std::endl;
        }
        for (const std::string &line : p.readStdoutLines(0)) {
            LOG_F(INFO, "out: %s", line.c_str());
        }
        return result;
    }
};

CMaker::CMaker()
    : _impl(std::make_shared<Impl>()) {}

CMaker::~CMaker() {}

std::string CMaker::getModuleDir() const {
    std::string r;
    if (_impl) {
        r = _impl->getModuleDir();
    }
    return r;
}

int CMaker::exec(const std::vector<std::string> &args, const std::vector<std::string> &env, const std::string &home,
                 const std::string &pwd) {
    int r = -1;
    if (_impl) {
        r = _impl->exec(args, env, home, pwd);
    }
    return r;
}

void CMaker::writeCbp(WriteFileCb writeFileCb) {
    if (_impl) {
        _impl->writeCbp(writeFileCb);
    }
}

} // namespace gatools

#ifdef CMAKER_WITH_UNIT_TESTS

using namespace gatools;

class CMakerTest : public ::testing::Test {
  public:
    CMakerTest() {
        cmaker.writeCbp([this](const std::string &path, const std::string &content) {
            cbpPathAndContents.push_back(make_pair(path, content));
        });

        impl = cmaker._impl;
        impl->in.projectDir = "/home/testuser/project";
        impl->in.buildDir = "/home/testuser/build-proj";
        impl->in.sdkDir = "/home/testuser/sdks/v42";
        impl->in.outputToStdout = false;
    }

    CMaker cmaker;
    std::shared_ptr<CMaker::Impl> impl;
    std::vector<std::pair<std::string, std::string>> cbpPathAndContents;
};

TEST_F(CMakerTest, VirtualFoldersNoChange) {

    std::string value = "CMake Files\\;CMake Files\\..\\;CMake Files\\..\\..\\;CMake Files\\..\\..\\..\\";
    std::string expected = "CMake Files\\;CMake Files\\..\\;CMake Files\\..\\..\\;CMake Files\\..\\..\\..\\";
    impl->addPrefixToVirtualFolder(value);
    ASSERT_EQ(expected, value);
}

TEST_F(CMakerTest, VirtualFoldersChange) {

    impl->in.virtualFolderPrefix = "..\\..\\sdk\\v43";
    std::string value = "CMake Files\\..\\..\\..\\..\\usr\\include\\someotherlib";
    std::string expected = "CMake Files\\..\\..\\sdk\\v43\\usr\\include\\someotherlib";
    impl->addPrefixToVirtualFolder(value);
    ASSERT_EQ(expected, value);
}

TEST_F(CMakerTest, PatchCBPs) {

    std::string expectedTestprojectCbpOutput;
    ASSERT_TRUE(ga::readFile("testproject_output.cbp.xml", expectedTestprojectCbpOutput));

    // Transform the original xml
    tinyxml2::XMLDocument inXml;
    inXml.LoadFile("testproject_input.cbp");
    auto patchResult = impl->patchCBP("/home/testuser/build-proj/testproject_input.cbp", inXml);

    ASSERT_EQ(CMaker::Impl::PatchResult::Changed, patchResult);
    ASSERT_EQ(1, cbpPathAndContents.size());
    ASSERT_EQ(expectedTestprojectCbpOutput, cbpPathAndContents[0].second);

    // Already transformed. Nothing will be done
    patchResult = impl->patchCBP("/home/testuser/build-proj/testproject_input.cbp", inXml);

    ASSERT_EQ(CMaker::Impl::PatchResult::Unchanged, patchResult);
    ASSERT_EQ(1, cbpPathAndContents.size());
}

TEST_F(CMakerTest, ECHO) {
    int r = cmaker.exec({"xecho", "test"}, {}, cmaker.getModuleDir(), cmaker.getModuleDir());
    ASSERT_EQ(0, r);
}
#endif

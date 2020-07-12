// Defines the entry point for the console application (either run unit tests or the app)

#ifdef CMAKER_WITH_UNIT_TESTS
#include "gtest/gtest.h"

int RunAllTests(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

int main(int argc, char **argv) {
    // The program just runs all the tests.
    return RunAllTests(argc, argv);
}

#else

#include "CMaker.h"
#include "file_system.h"
#include "loguru.hpp"
#include <pwd.h>
#include <string>
#include <unistd.h>
#include <vector>

using namespace gatools;

inline void initlog(int argc, char **argv) {
    loguru::g_stderr_verbosity = loguru::Verbosity_OFF;

    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H%M%S", timeinfo);

    std::string logFilePath("/tmp/cmaker/cmaker_");
    logFilePath += buffer;
    if (argc >= 1) {
        std::string fileName = ga::getFilename(argv[0]);
        if (!fileName.empty()) {
            logFilePath += "_";
            logFilePath += fileName;
        }
    }
    logFilePath += "_";
    logFilePath += std::to_string(getpid());
    logFilePath += ".log";
    loguru::add_file(logFilePath.c_str(), loguru::Append, loguru::Verbosity_MAX);
}

int main(int argc, char **argv) {
    initlog(argc, argv);

    std::string pwd;
    std::string home;

    std::vector<std::string> cmd(argc);
    for (int i = 0; i < argc; i++) {
        cmd[i] = argv[i];
    }

    std::vector<std::string> env;
    for (int i = 0; environ[i]; i++) {
        env.push_back(environ[i]);
    }

    // Home dir
    {
        passwd *mypasswd = getpwuid(getuid());
        if (mypasswd && mypasswd->pw_dir) {
            home = mypasswd->pw_dir;
        }
    }

    // PWD
    {
        char buff[FILENAME_MAX];
        const char *p = getcwd(buff, FILENAME_MAX);
        if (p) {
            pwd = p;
        }
    }

    CMaker cmaker;
    // cmd = {"cmaker", "/home/alin/projects/cpp-httplib/", "'-GCodeBlocks - Unix Makefiles'"};
    // pwd = "/home/alin/projects/build-cpp-httplib-Desktop-Debug/";
    int result = cmaker.exec(cmd, env, home, pwd);
    return result;
}

#endif

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
#include "loguru.hpp"
#include <string>
#include <unistd.h>
#include <vector>

using namespace gatools;

inline void initlog() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H%M%S", timeinfo);

    std::string logFilePath("/tmp/cmaker/cmaker_");
    logFilePath += buffer;
    logFilePath += std::to_string(getpid());
    logFilePath += ".log";
    loguru::add_file(logFilePath.c_str(), loguru::Append, loguru::Verbosity_MAX);
}

int main(int argc, char **argv) {
    initlog();

    std::vector<std::string> cmd(argc);
    std::string pwd;
    for (int i = 0; i < argc; i++) {
        cmd[i] = argv[i];
    }

    std::vector<std::string> env;
    for (int i = 0; environ[i]; i++) {
        env.push_back(environ[i]);
    }

    CMaker cmaker;
    // cmd = {"cmaker", "/home/alin/projects/cpp-httplib/", "'-GCodeBlocks - Unix Makefiles'"};
    // pwd = "/home/alin/projects/build-cpp-httplib-Desktop-Debug/";
    int result = cmaker.exec(cmd, env, pwd);
    return result;
}

#endif

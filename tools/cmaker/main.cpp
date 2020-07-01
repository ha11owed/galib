// VmCtrl.cpp : Defines the entry point for the console application.
//

#include "CMaker.h"
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace ga;

int main(int argc, char **argv) {
    std::vector<std::string> cmd(argc);
    for (int i = 0; i < argc; i++) {
        cmd[i] = argv[i];
    }

    CMaker cmaker;
    cmd = {"cmaker", "/home/alin/projects/cpp-httplib/", "'-GCodeBlocks - Unix Makefiles'"};
    int result = cmaker.exec(cmd, "/home/alin/projects/build-cpp-httplib-Desktop-Debug/");
    return result;
}

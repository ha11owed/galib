#define private public
#include "CMaker.h"
#include "CMaker.cpp"
#undef private

#include "file_system.h"
#include "gtest/gtest.h"
#include <chrono>

using namespace gatools;

class CMakerTest : public ::testing::Test {
  public:
    CMakerTest() {
        cmaker.writeCbp([this](const std::string &path, const std::string &content) {
            if (!path.ends_with("testproject_input.cbp")) {
                return;
            }
            cbpPathAndContents.push_back(make_pair(path, content));
        });

        ga::readFile("testproject_output.cbp.xml", expectedTestprojectCbpOutput);
    }

    CMaker cmaker;
    std::string expectedTestprojectCbpOutput;
    std::vector<std::pair<std::string, std::string>> cbpPathAndContents;
};

TEST_F(CMakerTest, CMAKE_BUILD) {

    std::string buildDir = "/home/testuser/build-testproj-Desktop-Debug";
    std::string pwd = "/home/testuser/testproj";
    const std::vector<std::string> cmakeBuild{"cmake", buildDir, "'-GCodeBlocks - Unix Makefiles'", pwd};

    cmaker.exec(cmakeBuild, "");

    ASSERT_EQ(1, cbpPathAndContents.size());
    ASSERT_EQ(expectedTestprojectCbpOutput, cbpPathAndContents[0].second);
}

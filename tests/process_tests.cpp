#include "process.h"
#include "gtest/gtest.h"
#include <chrono>

using namespace galib;

class ProcessTest : public ::testing::Test {
  public:
    struct InputLine {
        std::string text;
        int timeoutMs = 5;
    };

    /// @brief Holds the output
    struct Output {
        std::string inputLine;
        std::vector<std::string> outputLines;
    };

    /// @brief returns true if the process ended before the timeout
    bool runProcessEx(std::vector<std::string> cmd, int timeoutMs, std::vector<InputLine> input,
                      std::vector<Output> *output) {
        auto start = std::chrono::steady_clock::now();
        // Use this line to help debugging the output of the process
        // Process p(cmd, [](const auto &_l) { std::cout << "| " << _l << std::endl; });
        Process p(cmd);
        Output out;
        out.outputLines = p.readStdoutLines(input.empty() ? timeoutMs : input[0].timeoutMs);
        if (output != nullptr) {
            output->push_back(out);
        }

        for (const InputLine &inputLine : input) {
            p.writeLine(inputLine.text);
            out.inputLine = inputLine.text;
            out.outputLines = p.readStdoutLines(inputLine.timeoutMs);

            if (output != nullptr) {
                output->push_back(out);
            }
        }

        auto end = std::chrono::steady_clock::now();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return (durationMs < std::chrono::milliseconds(timeoutMs / 2));
    }

    bool runProcess(std::vector<std::string> cmd, int timeoutMs, std::vector<std::string> inputLines,
                    std::vector<Output> *output) {
        std::vector<InputLine> input;
        for (const std::string &text : inputLines) {
            InputLine il;
            il.text = text;
            input.push_back(il);
        }
        return runProcessEx(cmd, timeoutMs, input, output);
    }
};

TEST_F(ProcessTest, LS) {
    std::vector<Output> output;
    ASSERT_TRUE(runProcess({"ls", "/"}, 5000, {}, &output));

    // Check that bin is included in the folders
    std::vector<std::string> lines = output[0].outputLines;
    ASSERT_NE(0, lines.size());
    std::set<std::string> lsResult(lines.begin(), lines.end());
    ASSERT_EQ(lines.size(), lsResult.size());
    ASSERT_EQ(1, lsResult.count("bin"));
}

TEST_F(ProcessTest, ECHO) {
    std::vector<Output> output;
    ASSERT_TRUE(runProcess({"echo", "testecho"}, 5000, {}, &output));

    // Check that echo outputs its parameter
    std::vector<std::string> lines = output[0].outputLines;
    ASSERT_EQ(1, lines.size());
    ASSERT_EQ("testecho", lines[0]);
}

TEST_F(ProcessTest, BASH) {
    std::vector<Output> output;
    ASSERT_TRUE(runProcess({"/bin/bash"}, 5000, {"ls -la /bin", "exit"}, &output));

    ASSERT_EQ(3, output.size());
    // 0: empty
    ASSERT_EQ("", output[0].inputLine);
    ASSERT_EQ(0, output[0].outputLines.size());

    // 1: ls -la /bin
    ASSERT_EQ("ls -la /bin", output[1].inputLine);
    ASSERT_EQ(1, output[1].outputLines.size());
    std::string outputLine = output[1].outputLines[0];
    ASSERT_TRUE(outputLine.find("/bin") > 0);

    // 2: exit
    ASSERT_EQ("exit", output[2].inputLine);
    ASSERT_EQ(0, output[2].outputLines.size());
}

TEST_F(ProcessTest, DISABLED_SLEEP_KILL) {
    std::vector<std::string> cmd{"sleep", "5"};
    Process p(cmd);
    p.kill();
    p.kill();
}

TEST_F(ProcessTest, DISABLED_EXIT_JOIN) {
    std::vector<std::string> cmd{"bash"};
    Process p(cmd);
    p.writeLine("sleep 0");
    p.writeLine("exit 3");
    ASSERT_EQ(3, p.join());
}

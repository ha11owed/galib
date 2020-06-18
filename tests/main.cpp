#include "gtest/gtest.h"

int RunAllTests(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

int main(int argc, char **argv) {
    // The program just runs all the tests.
    return RunAllTests(argc, argv);
}

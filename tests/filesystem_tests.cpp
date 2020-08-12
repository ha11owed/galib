#include "file_system.h"
#include "gtest/gtest.h"
#include <chrono>

using namespace ga;

TEST(FileSystemTest, absolutePathUnix) {
    std::vector<std::string> absPaths{
        "/root/abc/def", "/.//root/abc/def", "/../root/abc/def", "///root/abc/def", "/", "//", "/."};
    std::vector<std::string> relPaths{"root/abc/def", "..///root/abc/def", ".///root/abc/def", "", ".", ".."};

    for (std::string path : absPaths) {
        ASSERT_TRUE(isAbsolutePath(path));
    }
    for (std::string path : relPaths) {
        ASSERT_FALSE(isAbsolutePath(path));
    }
}

TEST(FileSystemTest, absolutePathWindows) {
    std::vector<std::string> absPaths{"C:\\", "Z:\\dir\\", "Z:\\dir\\file.txt", "//C:/file.txt", "C:\\..\\"};
    std::vector<std::string> relPaths{"dir\\file.txt", "dir", "file.txt", "", ".", "..", "..\\dir"};

    for (std::string path : absPaths) {
        ASSERT_TRUE(isAbsolutePath(path));
    }
    for (std::string path : relPaths) {
        ASSERT_FALSE(isAbsolutePath(path));
    }
}

TEST(FileSystemTest, combine) {
    std::string expected;
    std::string actual;

    expected = "a/b";
    actual = combine("a", "b");
    ASSERT_EQ(expected, actual);

    expected = "a/b/c";
    actual = combine("a/b", "/c");
    ASSERT_EQ(expected, actual);

    expected = "a/b/c/d";
    actual = combine("a/b/c/", "/d");
    ASSERT_EQ(expected, actual);
}

TEST(FileSystemTest, split) {
    std::vector<std::string> expected;
    std::vector<std::string> actual;

    expected = {"C:", "abc", "file.txt"};
    actual = splitPath("\\\\C:\\\\\\abc////////\\\\////file.txt\\\\////\\\\");
    ASSERT_EQ(expected, actual);

    expected = {"C:", "abc", "file.txt"};
    actual = splitPath("//C:/abc/file.txt");
    ASSERT_EQ(expected, actual);

    expected = {"C:", "abc", "file.txt"};
    actual = splitPath("/////C:///abc///file.txt//");
    ASSERT_EQ(expected, actual);

    expected = {"C:", "abc", "file.txt"};
    actual = splitPath("C:\\abc\\file.txt");
    ASSERT_EQ(expected, actual);

    expected = {"C:", "abc", "file.txt"};
    actual = splitPath("\\C:\\abc\\file.txt\\");
    ASSERT_EQ(expected, actual);

    expected = {"dir"};
    actual = splitPath("/dir");
    ASSERT_EQ(expected, actual);

    expected = {"dir"};
    actual = splitPath("dir");
    ASSERT_EQ(expected, actual);

    expected = {"dir"};
    actual = splitPath("dir/");
    ASSERT_EQ(expected, actual);

    expected = {"dir1", "dir2"};
    actual = splitPath("/dir1/dir2/");
    ASSERT_EQ(expected, actual);

    expected = {};
    actual = splitPath("\\\\\\///\\\\");
    ASSERT_EQ(expected, actual);

    expected = {};
    actual = splitPath("");
    ASSERT_EQ(expected, actual);

    expected = {".."};
    actual = splitPath("//..\\//");
    ASSERT_EQ(expected, actual);
}

TEST(FileSystemTest, fileName) {
    ASSERT_EQ("file.txt", getFilename("/d1/file.txt"));
    ASSERT_EQ("", getFilename("/d1/d2/"));
    ASSERT_EQ("file.txt", getFilename("C:\\d1\\file.txt"));
    ASSERT_EQ("", getFilename("C:\\d1\\d2\\"));
}

TEST(FileSystemTest, parentPath) {
    ASSERT_EQ("/d1/", getParent("/d1/file.txt"));
    ASSERT_EQ("/d1/", getParent("/d1/d2/"));
    ASSERT_EQ("/d1/", getParent("/d1/d2/."));
    ASSERT_EQ("/d1/", getParent("/d1/d2/././"));
    ASSERT_EQ("C:\\d1\\", getParent("C:\\d1\\file.txt"));
    ASSERT_EQ("C:\\d1\\", getParent("C:\\d1\\d2\\"));
    ASSERT_EQ("C:\\d1\\", getParent("C:\\d1\\d2\\."));
    ASSERT_EQ("C:\\d1\\", getParent("C:\\d1\\d2\\.\\.\\"));

    ASSERT_EQ("/../", getParent("/../../"));
    ASSERT_EQ("/../", getParent("/../.."));
    ASSERT_EQ("/", getParent("/../."));

    ASSERT_EQ("", getParent("file"));
    ASSERT_EQ("/../", getParent("/../file.txt"));
    ASSERT_EQ("/../", getParent("/../file.txt"));
    ASSERT_EQ("", getParent(""));
}

TEST(FileSystemTest, simplePathUnixAbs) {
    std::string expected;
    std::string actual;

    expected = "/file.txt";
    ASSERT_EQ(true, getSimplePath("/././root/./.././file.txt", actual));
    ASSERT_EQ(expected, actual);

    expected = "/file.txt";
    ASSERT_EQ(true, getSimplePath("/./root/./.././.././file.txt", actual));
    ASSERT_EQ(expected, actual);

    expected = "/d1/file.txt";
    ASSERT_EQ(true, getSimplePath("/root/../../d1/d2/d3/../../file.txt", actual));
    ASSERT_EQ(expected, actual);
}

TEST(FileSystemTest, simplePathUnixRel) {
    std::string expected;
    std::string actual;

    expected = "../file.txt";
    ASSERT_EQ(true, getSimplePath("root/../../file.txt", actual));
    ASSERT_EQ(expected, actual);

    expected = "../d1/file.txt";
    ASSERT_EQ(true, getSimplePath(".././d1/./d2/./d3/.././../file.txt", actual));
    ASSERT_EQ(expected, actual);
}

TEST(FileSystemTest, relativePathOk) {
    std::string expected;
    std::string actual;

    expected = "../file.txt";
    ASSERT_EQ(true, getRelativePath("/root/abc/def", "/root/abc/file.txt", actual));
    ASSERT_EQ(expected, actual);

    expected = "../../../jkl/file.txt";
    ASSERT_EQ(true, getRelativePath("/root/abc/def/ghi/jkl/", "/root/abc/jkl/file.txt", actual));
    ASSERT_EQ(expected, actual);

    expected = "../../../jkl/mno/";
    ASSERT_EQ(true, getRelativePath("/root/abc/def/ghi/jkl/", "/root/abc/jkl/mno/", actual));
    ASSERT_EQ(expected, actual);
}

TEST(FileSystemTest, relativePathMOk) {
    std::string expected;
    std::string actual;

    ASSERT_EQ(false, getRelativePath("root/abc/def", "root/abc/file.txt", actual));
    ASSERT_EQ(expected, actual);

    ASSERT_EQ(false, getRelativePath("/root/abc/def", "root/abc/file.txt", actual));
    ASSERT_EQ(expected, actual);

    ASSERT_EQ(false, getRelativePath("root/abc/def", "/root/abc/file.txt", actual));
    ASSERT_EQ(expected, actual);

    ASSERT_EQ(false, getRelativePath(".", ".", actual));
    ASSERT_EQ(expected, actual);

    ASSERT_EQ(false, getRelativePath("a", "b", actual));
    ASSERT_EQ(expected, actual);

    ASSERT_EQ(false, getRelativePath("a/", "b/", actual));
    ASSERT_EQ(expected, actual);

    ASSERT_EQ(false, getRelativePath("a\\", "b\\", actual));
    ASSERT_EQ(expected, actual);

    ASSERT_EQ(false, getRelativePath("C:\\abc\\", "b", actual));
    ASSERT_EQ(expected, actual);

    ASSERT_EQ(false, getRelativePath(".", "C:\\abc\\", actual));
    ASSERT_EQ(expected, actual);
}

TEST(FileSystemTest, DISABLED_exists) {
    ASSERT_TRUE(pathExists("/usr/bin/"));
    ASSERT_TRUE(pathExists("/usr/bin/bash"));

    ASSERT_FALSE(pathExists("/usr/bin32242/"));
    ASSERT_FALSE(pathExists("/usr/bin/bash34235"));
}

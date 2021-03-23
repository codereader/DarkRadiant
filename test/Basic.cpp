/**
 * Tests for basic library functions (such as string comparison) which do not
 * require an actual Radiant environment.
 */

#include "gtest/gtest.h"

#include "string/string.h"
#include "os/path.h"

namespace test
{

TEST(BasicTest, StringCompareNoCase)
{
    EXPECT_EQ(string::icmp("blah", "blah"), 0);
    EXPECT_EQ(string::icmp("blah", "BLAH"), 0);
    EXPECT_EQ(string::icmp("MiXeD", "mIxED"), 0);

    EXPECT_EQ(string::icmp("a", "b"), -1);
    EXPECT_EQ(string::icmp("b", "a"), 1);
    EXPECT_EQ(string::icmp("baaaaa", "aaaaa"), 1);
}

TEST(BasicTest, StringILessFunctor)
{
    string::ILess less;

    EXPECT_TRUE(!less("blah", "BLAH"));
    EXPECT_TRUE(!less("BLAH", "blah"));

    EXPECT_TRUE(less("blah", "BLEH"));
    EXPECT_TRUE(!less("BLEH", "blah"));
}

TEST(PathTests, GetFileExtension)
{
    EXPECT_EQ(os::getExtension(""), "");
    EXPECT_EQ(os::getExtension("file55"), "");
    EXPECT_EQ(os::getExtension("file55."), "");
    EXPECT_EQ(os::getExtension("file.extension"), "extension");
    EXPECT_EQ(os::getExtension("File.TGA"), "TGA");
    EXPECT_EQ(os::getExtension("File.tga"), "tga");
    EXPECT_EQ(os::getExtension("file.tga.bak"), "bak");
    EXPECT_EQ(os::getExtension("relativefolder/file.tga"), "tga");
    EXPECT_EQ(os::getExtension("c:\\absolutepath\\tork.bak"), "bak");
    EXPECT_EQ(os::getExtension("\\absolutepath\\tork.doc"), "doc");
    EXPECT_EQ(os::getExtension("dds/textures/darkmod/test.dds"), "dds");
}

TEST(PathTests, RemoveFileExtension)
{
    EXPECT_EQ(os::removeExtension(""), "");
    EXPECT_EQ(os::removeExtension("file55"), "file55");
    EXPECT_EQ(os::removeExtension("file55."), "file55");
    EXPECT_EQ(os::removeExtension("file.extension"), "file");
    EXPECT_EQ(os::removeExtension("File.tga"), "File");
    EXPECT_EQ(os::removeExtension("file.tga.bak"), "file.tga");
    EXPECT_EQ(os::removeExtension("relativefolder/file.tga"), "relativefolder/file");
    EXPECT_EQ(os::removeExtension("c:\\absolutepath\\tork.bak"), "c:\\absolutepath\\tork");
    EXPECT_EQ(os::removeExtension("\\absolutepath\\tork.doc"), "\\absolutepath\\tork");
    EXPECT_EQ(os::removeExtension("dds/textures/darkmod/test.dds"), "dds/textures/darkmod/test");
}

}
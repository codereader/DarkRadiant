/**
 * Tests for basic library functions (such as string comparison) which do not
 * require an actual Radiant environment.
 */

#include "gtest/gtest.h"

#include "string/string.h"

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

}
#include "RadiantTest.h"

namespace test
{

using CommandSystemTest = RadiantTest;

TEST_F(CommandSystemTest, GetCommandSystem)
{
    const auto& mod = GlobalCommandSystem();
    EXPECT_EQ(mod.getName(), "CommandSystem");
}

TEST_F(CommandSystemTest, AddAndRunCommand)
{
    int runCount = 0;

    // Add a command which just logs the number of times it is called
    EXPECT_FALSE(GlobalCommandSystem().commandExists("testRunCount"));
    GlobalCommandSystem().addCommand("testRunCount",
                                     [&](const cmd::ArgumentList&) { ++runCount; });
    EXPECT_TRUE(GlobalCommandSystem().commandExists("testRunCount"));

    // Ensure that the call happens when we run the command
    GlobalCommandSystem().executeCommand("testRunCount");
    EXPECT_EQ(runCount, 1);
    GlobalCommandSystem().executeCommand("testRunCount");
    EXPECT_EQ(runCount, 2);
}

}
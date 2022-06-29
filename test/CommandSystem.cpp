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
    ASSERT_FALSE(GlobalCommandSystem().commandExists("testRunCount"));
    GlobalCommandSystem().addCommand("testRunCount",
                                     [&](const cmd::ArgumentList&) { ++runCount; });
    EXPECT_TRUE(GlobalCommandSystem().commandExists("testRunCount"));

    // Ensure that the call happens when we run the command
    GlobalCommandSystem().executeCommand("testRunCount");
    EXPECT_EQ(runCount, 1);
    GlobalCommandSystem().executeCommand("testRunCount");
    EXPECT_EQ(runCount, 2);
}

TEST_F(CommandSystemTest, AddCheckedCommand)
{
    const char* COMMAND_NAME = "testCheckedCommand";
    bool commandEnabled = false;

    // Add a command which is conditionally enabled based on our variable flag
    ASSERT_FALSE(GlobalCommandSystem().commandExists(COMMAND_NAME));
    GlobalCommandSystem().addWithCheck(
        COMMAND_NAME, [](const cmd::ArgumentList&) {}, [&]() { return commandEnabled; }
    );

    // The flag should control the executability of the command
    EXPECT_FALSE(GlobalCommandSystem().canExecute(COMMAND_NAME));
    commandEnabled = true;
    EXPECT_TRUE(GlobalCommandSystem().canExecute(COMMAND_NAME));
    commandEnabled = false;
    EXPECT_FALSE(GlobalCommandSystem().canExecute(COMMAND_NAME));
}

}
#include "RadiantTest.h"

namespace test
{

using CommandSystemTest = RadiantTest;

TEST_F(CommandSystemTest, GetCommandSystem)
{
    const auto& mod = GlobalCommandSystem();
    EXPECT_EQ(mod.getName(), "CommandSystem");
}

namespace
{
    // Command receiver object. Keeps track of its received arguments and the number of times it
    // was called.
    struct TestCommandReceiver
    {
        // Name of the command to call
        const std::string name;

        // Number of times called
        int runCount = 0;

        // Last received arguments
        cmd::ArgumentList args;

        // Construct with name
        TestCommandReceiver(std::string n)
        : name(std::move(n))
        {}

        // Executor method
        void operator() (cmd::ArgumentList argList) {
            ++runCount;
            args = std::move(argList);
        }
    };
}

TEST_F(CommandSystemTest, AddAndRunCommand)
{
    TestCommandReceiver rec("testRunCount");

    // Add a command which just logs the number of times it is called
    ASSERT_FALSE(GlobalCommandSystem().commandExists(rec.name));
    GlobalCommandSystem().addCommand(rec.name,
                                     [&](const cmd::ArgumentList& args) { rec(args); });
    EXPECT_TRUE(GlobalCommandSystem().commandExists(rec.name));

    // Ensure that the call happens when we run the command
    GlobalCommandSystem().executeCommand(rec.name);
    EXPECT_EQ(rec.runCount, 1);
    GlobalCommandSystem().executeCommand(rec.name);
    EXPECT_EQ(rec.runCount, 2);
}

TEST_F(CommandSystemTest, AddAndRunCommandWithArgs)
{
    TestCommandReceiver rec("testCmdWithArgs");
    ASSERT_FALSE(GlobalCommandSystem().commandExists(rec.name));

    // Create a test command which stores its args
    GlobalCommandSystem().addCommand(rec.name, [&](const cmd::ArgumentList& a) { rec(a); },
                                     {cmd::ARGTYPE_INT, cmd::ARGTYPE_STRING});

    // Call the command and check the args
    GlobalCommandSystem().executeCommand(rec.name, 27, std::string("balls"));
    EXPECT_EQ(rec.runCount, 1);
    ASSERT_EQ(rec.args.size(), 2);
    EXPECT_EQ(rec.args.at(0).getInt(), 27);
    EXPECT_EQ(rec.args.at(1).getString(), "balls");

    // Calling the command with incorrect args does nothing (the command is not
    // called, but there is not currently a way to signal this to the caller
    // except via the message bus)
    GlobalCommandSystem().executeCommand(rec.name, std::string("wrong"));
    EXPECT_EQ(rec.runCount, 1);

    // Call the command with an argument list
    GlobalCommandSystem().executeCommand(rec.name, {{45}, {"blah"}});
    EXPECT_EQ(rec.runCount, 2);
    EXPECT_EQ(rec.args.size(), 2);
    EXPECT_EQ(rec.args.at(0).getInt(), 45);
    EXPECT_EQ(rec.args.at(1).getString(), "blah");

    // Call the command with an argument list containing incorrect types (should be a NOP)
    GlobalCommandSystem().executeCommand(rec.name, {{"wrong"}, 4.5f});
    EXPECT_EQ(rec.runCount, 2);

    // Call the command as a string
    GlobalCommandSystem().execute("testCmdWithArgs 96 \"string_arg\"");
    EXPECT_EQ(rec.runCount, 3);
    EXPECT_EQ(rec.args.at(0).getInt(), 96);
    EXPECT_EQ(rec.args.at(1).getString(), "string_arg");
}

TEST_F(CommandSystemTest, RunCommandSequence)
{
    TestCommandReceiver first("firstRunCountCommand");
    TestCommandReceiver second("secondRunCountCommand");

    // Register a command for each run count
    ASSERT_FALSE(GlobalCommandSystem().commandExists(first.name));
    ASSERT_FALSE(GlobalCommandSystem().commandExists(second.name));
    GlobalCommandSystem().addCommand(first.name,
                                     [&](const cmd::ArgumentList& args) { first(args); });
    GlobalCommandSystem().addCommand(second.name,
                                     [&](const cmd::ArgumentList& args) { second(args); });

    // Run a semicolon-separated sequence of both commands
    GlobalCommandSystem().execute("firstRunCountCommand; secondRunCountCommand");
    EXPECT_EQ(first.runCount, 1);
    EXPECT_EQ(second.runCount, 1);
    GlobalCommandSystem().execute("  secondRunCountCommand  ; firstRunCountCommand  ");
    EXPECT_EQ(first.runCount, 2);
    EXPECT_EQ(second.runCount, 2);
    GlobalCommandSystem().execute("secondRunCountCommand ;secondRunCountCommand");
    EXPECT_EQ(second.runCount, 4);
}

TEST_F(CommandSystemTest, RunCommandSequenceWithArgs)
{
    TestCommandReceiver first("firstCommandWithArgs");
    TestCommandReceiver second("secondCommandWithArgs");

    // Register a command for each run count
    ASSERT_FALSE(GlobalCommandSystem().commandExists(first.name));
    ASSERT_FALSE(GlobalCommandSystem().commandExists(second.name));
    GlobalCommandSystem().addCommand(first.name, [&](const cmd::ArgumentList& a) { first(a); },
                                     {cmd::ARGTYPE_STRING});
    GlobalCommandSystem().addCommand(second.name, [&](const cmd::ArgumentList& a) { second(a); },
                                     {cmd::ARGTYPE_DOUBLE});

    // Run a semicolon-separated sequence of both commands
    GlobalCommandSystem().execute("firstCommandWithArgs \"blah\"; secondCommandWithArgs 1.25");
    EXPECT_EQ(first.runCount, 1);
    EXPECT_EQ(first.args.at(0).getString(), "blah");
    EXPECT_EQ(second.runCount, 1);
    EXPECT_EQ(second.args.at(0).getDouble(), 1.25);

    // Calling the first command with incorrect args should not prevent the second command from
    // running
    GlobalCommandSystem().execute("firstCommandWithArgs ; secondCommandWithArgs -16.9");
    EXPECT_EQ(first.runCount, 1);
    EXPECT_EQ(second.runCount, 2);
    EXPECT_EQ(second.args.at(0).getDouble(), -16.9);
}

TEST_F(CommandSystemTest, PassVectorArgs)
{
    // Add a command which receives vectors
    TestCommandReceiver rec("vectorCommand");
    GlobalCommandSystem().addCommand(rec.name, [&](const cmd::ArgumentList& a) { rec(a); },
                                     {cmd::ARGTYPE_VECTOR3, cmd::ARGTYPE_VECTOR2});

    // Ensure that vector arguments are passed through correctly
    GlobalCommandSystem().executeCommand(rec.name, {Vector3{1, 4, -25}, Vector2{-5, 56.5}});
    EXPECT_EQ(rec.runCount, 1);
    ASSERT_EQ(rec.args.size(), 2);
    EXPECT_EQ(rec.args.at(0).getVector3(), Vector3(1, 4, -25));
    EXPECT_EQ(rec.args.at(1).getVector2(), Vector2(-5, 56.5));
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
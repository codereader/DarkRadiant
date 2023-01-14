#include "RadiantTest.h"

namespace test
{

using CommandSystemTest = RadiantTest;

TEST_F(CommandSystemTest, ConstructVoidArg)
{
    cmd::Argument voidArg;

    EXPECT_EQ(voidArg.getType(), cmd::ARGTYPE_VOID);
    EXPECT_EQ(voidArg.getInt(), 0);
    EXPECT_EQ(voidArg.getDouble(), 0);
    EXPECT_EQ(voidArg.getString(), "");
}

TEST_F(CommandSystemTest, ConstructIntArg)
{
    cmd::Argument intArg(357);

    EXPECT_EQ(intArg.getType(), cmd::ARGTYPE_INT | cmd::ARGTYPE_DOUBLE);
    EXPECT_EQ(intArg.getInt(), 357);
    EXPECT_EQ(intArg.getDouble(), 357.0);
    EXPECT_EQ(intArg.getString(), "357");
}

TEST_F(CommandSystemTest, ConstructStringArg)
{
    cmd::Argument stringArg("arbitrary string");

    EXPECT_EQ(stringArg.getType(), cmd::ARGTYPE_STRING);
    EXPECT_EQ(stringArg.getString(), "arbitrary string");

    // String should be interpreted as numeric if possible
    cmd::Argument intStringArg("81924");
    EXPECT_EQ(intStringArg.getType(),
              cmd::ARGTYPE_STRING | cmd::ARGTYPE_INT | cmd::ARGTYPE_DOUBLE);
    EXPECT_EQ(intStringArg.getDouble(), 81924.0);
    EXPECT_EQ(intStringArg.getInt(), 81924);
    EXPECT_EQ(intStringArg.getString(), "81924");

    // Integers are interpreted with stoi(), which does not fail on floating point values but
    // ignores everything after the decimal point.
    cmd::Argument fltStringArg("34.2570");
    EXPECT_EQ(fltStringArg.getType(),
              cmd::ARGTYPE_STRING | cmd::ARGTYPE_INT | cmd::ARGTYPE_DOUBLE);
    EXPECT_EQ(fltStringArg.getDouble(), 34.257);
    EXPECT_EQ(fltStringArg.getInt(), 34);
    EXPECT_EQ(fltStringArg.getString(), "34.2570");
}

TEST_F(CommandSystemTest, ConstructVectorArg)
{
    // Vector2
    cmd::Argument v2Arg(Vector2(123, -8.6));
    EXPECT_EQ(v2Arg.getType(), cmd::ARGTYPE_VECTOR2);
    EXPECT_EQ(v2Arg.getVector2(), Vector2(123, -8.6));

    // Vector3
    cmd::Argument v3Arg(Vector3(-18, 0.56, 1.25));
    EXPECT_EQ(v3Arg.getType(), cmd::ARGTYPE_VECTOR3);
    EXPECT_EQ(v3Arg.getVector3(), Vector3(-18, 0.56, 1.25));

    // Vectors can also be constructed from strings
    cmd::Argument v2Str("18 -20");
    EXPECT_EQ(v2Str.getType(),
              cmd::ARGTYPE_STRING | cmd::ARGTYPE_VECTOR2 | cmd::ARGTYPE_INT | cmd::ARGTYPE_DOUBLE);
    EXPECT_EQ(v2Str.getVector2(), Vector2(18, -20));

    cmd::Argument v3Str("6 8 0.12");
    EXPECT_EQ(
        v3Str.getType(),
        cmd::ARGTYPE_STRING | cmd::ARGTYPE_VECTOR3 | cmd::ARGTYPE_VECTOR2 | cmd::ARGTYPE_INT
                            | cmd::ARGTYPE_DOUBLE
    );
    EXPECT_EQ(v3Str.getVector3(), Vector3(6, 8, 0.12));
}

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

    // Parse a vector from a string
    GlobalCommandSystem().execute("vectorCommand \"24 -8.5 0.0246\" \"18 256\"");
    EXPECT_EQ(rec.runCount, 2);
    ASSERT_EQ(rec.args.size(), 2);
    EXPECT_EQ(rec.args.at(0).getVector3(), Vector3(24, -8.5, 0.0246));
    EXPECT_EQ(rec.args.at(1).getVector2(), Vector2(18, 256));
}

TEST_F(CommandSystemTest, AcceptDoubleAsIntArg)
{
    TestCommandReceiver rec("intCmd");
    ASSERT_FALSE(GlobalCommandSystem().commandExists(rec.name));
    GlobalCommandSystem().addCommand(rec.name, [&](const cmd::ArgumentList& a) { rec(a); },
                                     {cmd::ARGTYPE_INT});

    // Double is accepted as an int if it is rounded to an int already
    GlobalCommandSystem().executeCommand(rec.name, 2.0);
    EXPECT_EQ(rec.runCount, 1);
    ASSERT_EQ(rec.args.size(), 1);
    EXPECT_EQ(rec.args.at(0).getDouble(), 2.0);
    EXPECT_EQ(rec.args.at(0).getInt(), 2);

    // Double which is not an integer value does not trigger the command
    GlobalCommandSystem().executeCommand(rec.name, 2.9);
    EXPECT_EQ(rec.runCount, 1);
    GlobalCommandSystem().executeCommand(rec.name, 2.6);
    EXPECT_EQ(rec.runCount, 1);
    GlobalCommandSystem().executeCommand(rec.name, 26.0);
    EXPECT_EQ(rec.runCount, 2);
}

TEST_F(CommandSystemTest, AcceptIntAsDoubleArg)
{
    TestCommandReceiver rec("doubleCmd");
    ASSERT_FALSE(GlobalCommandSystem().commandExists(rec.name));
    GlobalCommandSystem().addCommand(rec.name, [&](const cmd::ArgumentList& a) { rec(a); },
                                     {cmd::ARGTYPE_DOUBLE});

    // Every int is a valid double
    GlobalCommandSystem().executeCommand(rec.name, 3829);
    EXPECT_EQ(rec.runCount, 1);
    EXPECT_EQ(rec.args.at(0).getDouble(), 3829);
    EXPECT_EQ(rec.args.at(0).getInt(), 3829);

    GlobalCommandSystem().executeCommand(rec.name, -2);
    EXPECT_EQ(rec.args.at(0).getDouble(), -2);
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
#include "RadiantTest.h"
#include "string/split.h"

namespace test
{

using GameTest = RadiantTest;

TEST_F(GameTest, GetCurrentGameConfig)
{
    // Check that we can get the game manager and current game without crashing or anything
    auto& mgr = GlobalGameManager();
    auto game = mgr.currentGame();
    ASSERT_TRUE(game);

    // RadiantTest sets up a test game type which should be exposed via the GameManager
    auto conf = mgr.getConfig();
    EXPECT_EQ(conf.gameType, RadiantTest::DEFAULT_GAME_TYPE);

    // The start of the engine path could be anywhere (depending on where the test binaries are
    // being run from), but it should end with "resources/tdm"
    auto pathComps = string::splitToVec(conf.enginePath, "/");
    EXPECT_GE(pathComps.size(), 2);
    EXPECT_EQ(pathComps.at(pathComps.size() - 1), "tdm");
    EXPECT_EQ(pathComps.at(pathComps.size() - 2), "resources");
}

}
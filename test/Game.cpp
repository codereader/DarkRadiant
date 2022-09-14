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

TEST_F(GameTest, GetGameList)
{
    auto games = GlobalGameManager().getSortedGameList();
    ASSERT_TRUE(games.size() > 2);

    // The games are sorted by the "index" value in the .game XML file, placing TDM at the top,
    // followed by Doom 3 and its demo.
    EXPECT_EQ(games[0]->getName(), "The Dark Mod 2.0 (Standalone)");
    EXPECT_EQ(games[1]->getName(), "Doom 3");
    EXPECT_EQ(games[2]->getName(), "Doom 3 Demo");
}

TEST_F(GameTest, GetGameKeyValues)
{
    auto game = GlobalGameManager().currentGame();

    // Default game is darkmod.game
    EXPECT_EQ(game->getKeyValue("type"), "doom3");
    EXPECT_EQ(game->getKeyValue("name"), "The Dark Mod 2.0 (Standalone)");
    EXPECT_EQ(game->getKeyValue("index"), "10");
    EXPECT_EQ(game->getKeyValue("maptypes"), "mapdoom3");
}

}
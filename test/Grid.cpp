#include "RadiantTest.h"

#include "igrid.h"
#include "iselectable.h"
#include "icommandsystem.h"
#include "iradiant.h"
#include <sigc++/connection.h>

#include "algorithm/Scene.h"
#include "messages/GridSnapRequest.h"

namespace test
{

using GridTest = RadiantTest;

inline void checkGridSize(GridSize size, grid::Space space, float expectedSize)
{
    GlobalGrid().setGridSize(size);
    EXPECT_EQ(GlobalGrid().getGridSize(space), expectedSize);
}

inline void checkGridPower(GridSize size, grid::Space space, int expectedPower)
{
    GlobalGrid().setGridSize(size);
    EXPECT_EQ(GlobalGrid().getGridPower(space), expectedPower);
}

TEST_F(GridTest, GridBase)
{
    EXPECT_EQ(GlobalGrid().getGridBase(grid::Space::World), 2);
    EXPECT_EQ(GlobalGrid().getGridBase(grid::Space::Texture), 2);
}

TEST_F(GridTest, WorldGridSize)
{
    checkGridSize(GRID_0125, grid::Space::World, 0.125f);
    checkGridSize(GRID_025, grid::Space::World, 0.25f);
    checkGridSize(GRID_05, grid::Space::World, 0.5f);
    checkGridSize(GRID_1, grid::Space::World, 1.0f);
    checkGridSize(GRID_2, grid::Space::World, 2.0f);
    checkGridSize(GRID_4, grid::Space::World, 4.0f);
    checkGridSize(GRID_8, grid::Space::World, 8.0f);
    checkGridSize(GRID_16, grid::Space::World, 16.0f);
    checkGridSize(GRID_32, grid::Space::World, 32.0f);
    checkGridSize(GRID_64, grid::Space::World, 64.0f);
    checkGridSize(GRID_128, grid::Space::World, 128.0f);
    checkGridSize(GRID_256, grid::Space::World, 256.0f);
}

TEST_F(GridTest, WorldGridPower)
{
    checkGridPower(GRID_0125, grid::Space::World, -3);
    checkGridPower(GRID_025, grid::Space::World, -2);
    checkGridPower(GRID_05, grid::Space::World, -1);
    checkGridPower(GRID_1, grid::Space::World, 0);
    checkGridPower(GRID_2, grid::Space::World, 1);
    checkGridPower(GRID_4, grid::Space::World, 2);
    checkGridPower(GRID_8, grid::Space::World, 3);
    checkGridPower(GRID_16, grid::Space::World, 4);
    checkGridPower(GRID_32, grid::Space::World, 5);
    checkGridPower(GRID_64, grid::Space::World, 6);
    checkGridPower(GRID_128, grid::Space::World, 7);
    checkGridPower(GRID_256, grid::Space::World, 8);
}

TEST_F(GridTest, TextureGridSize)
{
    checkGridSize(GRID_0125, grid::Space::Texture, 0.0009765625f); // lower bound
    checkGridSize(GRID_025, grid::Space::Texture, 0.001953125f);
    checkGridSize(GRID_05, grid::Space::Texture, 0.00390625f);
    checkGridSize(GRID_1, grid::Space::Texture, 0.0078125f); 
    checkGridSize(GRID_2, grid::Space::Texture, 0.015625f);
    checkGridSize(GRID_4, grid::Space::Texture, 0.03125f);
    checkGridSize(GRID_8, grid::Space::Texture, 0.0625f);
    checkGridSize(GRID_16, grid::Space::Texture, 0.125f);
    checkGridSize(GRID_32, grid::Space::Texture, 0.25f);
    checkGridSize(GRID_64, grid::Space::Texture, 0.5f);
    checkGridSize(GRID_128, grid::Space::Texture, 1.0f); // upper bound
    checkGridSize(GRID_256, grid::Space::Texture, 1.0f);
}

TEST_F(GridTest, TextureGridPower)
{
    checkGridPower(GRID_0125, grid::Space::Texture, -10); // lower bound
    checkGridPower(GRID_025, grid::Space::Texture, -9);
    checkGridPower(GRID_05, grid::Space::Texture, -8);
    checkGridPower(GRID_1, grid::Space::Texture, -7); 
    checkGridPower(GRID_2, grid::Space::Texture, -6);
    checkGridPower(GRID_4, grid::Space::Texture, -5);
    checkGridPower(GRID_8, grid::Space::Texture, -4);
    checkGridPower(GRID_16, grid::Space::Texture, -3);
    checkGridPower(GRID_32, grid::Space::Texture, -2);
    checkGridPower(GRID_64, grid::Space::Texture, -1);
    checkGridPower(GRID_128, grid::Space::Texture, 0); // upper bound
    checkGridPower(GRID_256, grid::Space::Texture, 0);
}

TEST_F(GridTest, ChangedSignal)
{
    // Set to the default
    GlobalGrid().setGridSize(GRID_1);

    bool signalFired = false;
    sigc::connection conn = GlobalGrid().signal_gridChanged().connect([&]()
    {
        signalFired = true;
    });

    GlobalGrid().setGridSize(GRID_64);
    EXPECT_TRUE(signalFired) << "Signal didn't fire after changing grid size";

    signalFired = false;

    // Set the same grid size again, the signal shouldn't fire since we don't change anything
    GlobalGrid().setGridSize(GRID_64);
    EXPECT_FALSE(signalFired) << "Signal fired even when grid size didn't change";

    conn.disconnect();
}

inline void checkGridSizeByCmd(const std::string& size, grid::Space space, float expectedSize)
{
    GlobalCommandSystem().executeCommand("SetGrid", size);
    EXPECT_EQ(GlobalGrid().getGridSize(space), expectedSize);
}

TEST_F(GridTest, SetGridSizeByCmd)
{
    checkGridSizeByCmd(grid::getStringForSize(GRID_0125), grid::Space::World, 0.125f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_025), grid::Space::World, 0.25f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_05), grid::Space::World, 0.5f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_1), grid::Space::World, 1.0f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_2), grid::Space::World, 2.0f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_4), grid::Space::World, 4.0f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_8), grid::Space::World, 8.0f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_16), grid::Space::World, 16.0f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_32), grid::Space::World, 32.0f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_64), grid::Space::World, 64.0f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_128), grid::Space::World, 128.0f);
    checkGridSizeByCmd(grid::getStringForSize(GRID_256), grid::Space::World, 256.0f);
}

TEST_F(GridTest, GridSnapMessageIsSent)
{
    bool messageReceived = false;

    auto handler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::GridSnapRequest,
        radiant::TypeListener<selection::GridSnapRequest>(
            [&](selection::GridSnapRequest& msg) { messageReceived = true; }));

    // Fire the command, this should send out the request
    GlobalCommandSystem().executeCommand("SnapToGrid");

    EXPECT_TRUE(messageReceived) << "No message received during SnapToGrid";

    GlobalRadiantCore().getMessageBus().removeListener(handler);
}

/*
 * After grid-snapping some brushes might be degenerate, and before solving #6120
 * these were not immediately removed the scene. Next time the selection system had
 * been finishing an operation (which in this scenario is a null operation), the
 * degenerate brushes had been removed outside any undoable transaction.
 * Hitting undo crashed the app due to dangling references.
 */
TEST_F(GridTest, DegenerateBrushesAreRemovedAfterGridsnap)
{
    loadMap("degenerate_brushes_after_gridsnap.map");

    // Select all world brushes
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
    worldspawn->foreachNode([](const auto& node)
    {
        Node_setSelected(node, true);
        return true;
    });

    EXPECT_GT(GlobalSelectionSystem().countSelected(), 0) << "Need to have at least one brush selected";

    // Set a large grid size to let brushes disappear on snap
    GlobalGrid().setGridSize(GRID_8);
    GlobalCommandSystem().executeCommand("SnapToGrid");

    auto childCount = algorithm::getChildCount(worldspawn);
    EXPECT_GT(childCount, 0) << "Must have some brushes after grid snapping";

    // Make all brushes calculate their faces
    worldspawn->foreachNode([](const auto& node)
    {
        if (auto brush = Node_getIBrush(node); brush)
        {
            brush->evaluateBRep();
        }
        return true;
    });

    // Start and end a manipulation (without moving anything)
    GlobalSelectionSystem().onManipulationStart();
    GlobalSelectionSystem().onManipulationEnd();

    EXPECT_EQ(algorithm::getChildCount(worldspawn), childCount) << "Manipulation should not have altered the child count";

    // Fire undo, which caused a crash as described in #6120
    GlobalCommandSystem().executeCommand("Undo");
}

}

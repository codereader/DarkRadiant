#include "RadiantTest.h"

#include "igrid.h"
#include "icommandsystem.h"
#include "iradiant.h"
#include <sigc++/connection.h>
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
    checkGridSize(GRID_0125, grid::Space::Texture, 0.0078125f);
    checkGridSize(GRID_025, grid::Space::Texture, 0.0078125f);
    checkGridSize(GRID_05, grid::Space::Texture, 0.0078125f);
    checkGridSize(GRID_1, grid::Space::Texture, 0.0078125f); // lower bound
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
    checkGridPower(GRID_0125, grid::Space::Texture, -7);
    checkGridPower(GRID_025, grid::Space::Texture, -7);
    checkGridPower(GRID_05, grid::Space::Texture, -7);
    checkGridPower(GRID_1, grid::Space::Texture, -7); // lower bound
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

}
